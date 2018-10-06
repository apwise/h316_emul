/* Honeywell Series 16 emulator
 * Copyright (C) 2018  Adrian Wise
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307 USA
 *
 */

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "iodev.hh"
#include "stdtty.hh"

#include "proc.hh"
#include "vdmc.hh"

using namespace std;

VDMC::VDMC(Proc *p, unsigned int cc_addr)
  : IODEV(p)
  , cc_addr(cc_addr)
{
  master_clear();
}

void VDMC::master_clear()
{
  unsigned int i;
  
  intr_mask = false;
  channel = 0;
  busy.reset();
  chn_interrupt.reset();
  unexpected_trfr.reset();
  input_mode.reset();

  for (i=0; i<NUM_CHANNELS; i++) {
    trfr_size[i] = 0;
    data_prng[i] = 0;
    time_prng[i] = 0;
    time_ctrl[i] = 0;
    checksum[i] = 0;
    trfr_cntr[i] = 0;
    pending_transfers[i] = 0;
    last_pending[i] = false;
    error_bits[i].reset();
  }
}

void VDMC::reload_time_cntr(unsigned int chan)
{
  unsigned int time_cntr = time_ctrl[chan] & 0x0fff;

  unsigned int mask = (1 << ((time_ctrl[chan] >> 12) & 0x000f)) - 1;

  time_cntr += (time_prng[chan] & mask);

  if (mask != 0) {
    time_prng[chan] = (( time_prng[chan] >> 1) ^
                       ((time_prng[chan]  & 1) ? PRNG_TAPS : 0));
  }

  p->queue_hc(time_cntr, this, (VDMC_REASON_TIMER + chan));
}

bool VDMC::no_chan_error(unsigned int chan)
{
  bool r = true;
  if (unexpected_trfr[chan] ||
      error_bits[chan].any() ||
      (checksum[chan] != 0)) {
    r = false;
  }
  return r;
}

bool VDMC::interrupts(signed short *chan)
{
  bool r = false;
  unsigned int i;
  
  if (chan) {
    *chan = ~0;
  }
  
  for (i=0; i < NUM_CHANNELS; i++) {
    if (chn_interrupt[i]) {
      r = true;
      if (chan) {
        *chan = i;
      }
      break;
    }
  }

  r &= intr_mask;
  
  return r;
}

void VDMC::evaluate_interrupts()
{
  if (interrupts()) {
    p->set_interrupt(SMK_MASK);
  } else {
    p->clear_interrupt(SMK_MASK);
  }
}

IODEV::STATUS VDMC::ina(unsigned short instr, signed short &data)
{
  STATUS status = STATUS_READY;
  
  if (is_central(instr)) {
    switch(function_code(instr)) {
    case INA_CHAN_SLCT: data = channel; break;
    case INA_INTR_CHAN: (void) interrupts(&data); break;
    case INA_ERR_CHAN:
      data = ~0;
      for (unsigned int i = 0; i < NUM_CHANNELS; i++) {
        if (unexpected_trfr[i]) {
          data = i;
          break;
        }
      }
      break;
    default: status = STATUS_WAIT;
    }
  } else {
    switch(function_code(instr)) {
    case INA_TRFR_SIZE: data =  trfr_size[channel]; break;
    case INA_DATA_PRNG: data =  data_prng[channel]; break;
    case INA_TIME_PRNG: data =  time_prng[channel]; break;
    case INA_TIME_CTRL: data =  time_ctrl[channel]; break;
    case INA_CHAN_ERRS:
      data = (((unsigned int) unexpected_trfr[channel]) |
              (error_bits[channel].to_ulong()            <<   1) |
              (((unsigned int) (checksum[channel] != 0)) << (1+NUM_ERROR_BITS)));
      //cout << "INA_CHAN_ERRS: " << " data = " << data << endl;
      break;
    default: status = STATUS_WAIT;
    }
  }
  
  return status;
}

IODEV::STATUS VDMC::ota(unsigned short instr, signed short data)
{
  STATUS status = STATUS_READY;

  if (is_central(instr)) {
    switch(function_code(instr)) {
    case OTA_CHAN_SLCT: channel = data & 0xf; break;
    default: status = STATUS_WAIT;
    }
  } else {
    switch(function_code(instr)) {
    case OTA_TRFR_SIZE: trfr_size[channel] = data; break;
    case OTA_DATA_PRNG: data_prng[channel] = data; break;
    case OTA_TIME_PRNG: time_prng[channel] = data; break;
    case OTA_TIME_CTRL: time_ctrl[channel] = data; break;
    default: status = STATUS_WAIT;
    }
  }
  return status;
}

VDMC::STATUS VDMC::sks(unsigned short instr)
{
  STATUS status = STATUS_READY;

  if (is_central(instr)) {
    switch(function_code(instr)) {
    case SKS_NOT_BUSY:  if (busy.any())             status = STATUS_WAIT; break;
    case SKS_NOT_INTR:  if (interrupts())           status = STATUS_WAIT; break;
    case SKS_NTRFR_ERR: if (unexpected_trfr.any())  status = STATUS_WAIT; break;
    default:                                        status = STATUS_WAIT;
    }
  } else {
    switch(function_code(instr)) {
    case SKS_NOT_BUSY: if (busy[channel])           status = STATUS_WAIT; break;
    case SKS_NO_ERROR: if (no_chan_error(channel))  status = STATUS_WAIT; break;
    case SKS_NOT_INTR: if (!chn_interrupt[channel]) status = STATUS_WAIT; break;
    default:                                        status = STATUS_WAIT;
    }
  }
  return status;
}

VDMC::STATUS VDMC::ocp(unsigned short instr)
{
  if (is_central(instr)) {
    switch(function_code(instr)) {
    case OCP_CLEAR_ERR: unexpected_trfr.reset(); break;
    default: break; /* Do nothing */
    }
  } else {
    bool start = false;
    switch(function_code(instr)) {
    case OCP_STRT_OUTP: input_mode[channel] = false; start = true; break;
    case OCP_STRT_INPT: input_mode[channel] = true;  start = true; break;
    case OCP_CLEAR_INT: chn_interrupt[channel] = false; evaluate_interrupts(); break;
    case OCP_CLEAR_ERR: error_bits[channel].reset(); break;
    default: break; /* Do nothing */
    }

    if (start) {
      // cout << "Start channel " << channel << endl;
      trfr_cntr[channel] = trfr_size[channel] - 1;
      pending_transfers[channel] = 0;
      last_pending[channel] = false;
      checksum[channel] = 0;
      chn_interrupt[channel] = false;
      evaluate_interrupts();
      error_bits[channel].reset();
      reload_time_cntr(channel);
      busy[channel] = true;
    }
  }
  return STATUS_READY;
}

VDMC::STATUS VDMC::smk(unsigned short mask)
{
  intr_mask = (mask & SMK_MASK);

  evaluate_interrupts();
  
  return STATUS_READY;
}

void VDMC::event(int reason)
{
  if ((reason >= VDMC_REASON_TIMER                        ) &&
      (reason < (VDMC_REASON_TIMER + ((int) NUM_CHANNELS)))) {
    unsigned int chan = reason - VDMC_REASON_TIMER;
    pending_transfers[chan]++;
    p->set_dmcreq(1 << chan);
    if (trfr_cntr[chan] == 0) {
      last_pending[chan] = true;
    } else {
      trfr_cntr[chan]--;
      reload_time_cntr(chan);
    }
  } else {
    switch(reason) {
    case REASON_MASTER_CLEAR:
      master_clear();
      break;
      
    default:
      fprintf(stderr, "%s %d\n", __PRETTY_FUNCTION__, reason);
      exit(1);
      break;
    }
  }
}

void VDMC::dmc(signed short &data, bool erl)
{
  const unsigned int chan = p->get_dmc_dev();
  
  if (pending_transfers[chan] == 0) {
    unexpected_trfr[chan] = true;
  } else {
    const bool last_transfer = ((pending_transfers[chan] == 1) &&
                                last_pending[chan]);
    
    pending_transfers[chan]--;
    
    if (erl && (!last_transfer)) {
      error_bits[chan][UNEXPECTED_EOR] = true;
    } else if (last_transfer && (!erl)) {
      //cout << "Missing EOR" << endl;
      error_bits[chan][MISSING_EOR] = true;
    }
    
    if (input_mode[chan]) {
      if (last_transfer) {
        data = - (((checksum[chan] & 1) << 15) | (checksum[chan] >> 1));
      } else {
        data = data_prng[chan];
      }
    } else {
      if ((!last_transfer) &&
          ((data & 0xffff) != (data_prng[chan] & 0xffff))) {
        //cout << "Mismatch" << endl;
        error_bits[chan][DATA_MISMATCH] = true;
      }
    }
    
    checksum[chan] = ((((checksum[chan] & 1) << 15) | (checksum[chan] >> 1)) +
                      data);

    //cout << "DMC " << ((input_mode[chan]) ? "input  " : "output ")
    //     << "data = " << hex << ((unsigned) data) << " "
    //     << "data_prng = " << hex << data_prng[chan]
    //     << endl;

    if (last_transfer) {
      // cout << "Stop channel " << chan << endl;
      busy[chan] = false;
      chn_interrupt[chan] = true;
      evaluate_interrupts();
    } else {
      data_prng[chan] = (( data_prng[chan] >> 1) ^
                         ((data_prng[chan]  & 1) ? PRNG_TAPS : 0));
    }
  }

}
