/* Honeywell Series 16 emulator
 * Copyright (C) 2018, 2024  Adrian Wise
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
#include <cassert>
#include <iostream>
#include <iomanip>

#include "iodev.hh"
#include "stdtty.hh"

#include "proc.hh"
#include "spi.hh"

using namespace std;

ostream &operator<<(ostream &os, const SPI::SPI_STATE &state)  
{
  const char *const s = 
    ((state == SPI::STATE_POWR) ? "POWR" :
     (state == SPI::STATE_IDLE) ? "IDLE" :
     (state == SPI::STATE_LOWL) ? "LOWL" :
     (state == SPI::STATE_WPRT) ? "WPRT" :
     (state == SPI::STATE_CMND) ? "CMND" :
     (state == SPI::STATE_SHF1) ? "SHF1" :
     (state == SPI::STATE_ADR1) ? "ADR1" :
     (state == SPI::STATE_ADR2) ? "ADR2" :
     (state == SPI::STATE_ADR3) ? "ADR3" :
     (state == SPI::STATE_ADR4) ? "ADR4" :
     (state == SPI::STATE_SHF2) ? "SHF2" :
     (state == SPI::STATE_MODE) ? "MODE" :
     (state == SPI::STATE_SHF3) ? "SHF3" :
     (state == SPI::STATE_HMOD) ? "HMOD" :
     (state == SPI::STATE_SHF4) ? "SHF4" :
     (state == SPI::STATE_DUM2) ? "DUM2" :
     (state == SPI::STATE_DUMM) ? "DUMM" :
     (state == SPI::STATE_DATA) ? "DATA" :
     (state == SPI::STATE_DONE) ? "DONE" :
     (state == SPI::STATE_DISC) ? "DISC" : "????");
  os << s;  
  return os;  
}

SPI::SPI(Proc *p, SpiDev *devices[CHIP_SELECTS],
         unsigned int smk_bit,
         unsigned int pwr_dly,
         unsigned int dmc_chn,
         unsigned int boot_cmnd,
         unsigned int boot_ctrl,
         unsigned int boot_addr
         )
  : IODEV(p)
  , smk_mask(1 << (16-smk_bit))
  , pwr_dly(pwr_dly)
  , dmc_chn(dmc_chn)
  , boot_cmnd(boot_cmnd)
  , boot_ctrl(boot_ctrl)
  , boot_addr(boot_addr)
  , inner(devices)
{
  master_clear();
}

SPI::~SPI()
{
}

void SPI::master_clear()
{
  intr_mask = false;
  intr_dmc  = false;
  bt_trig   = true;
  mode_out  = false;
  mode_16   = false;
  mode_dmc  = false;
  wprot     = true;
  laddrbt   = true;
  last      = false;
  last_w    = false;
  
  reset_time = p->get_half_cycles();
  state = STATE_POWR;

  wprot_ll  = true;
  boot      = false;
  data_reg  = 0;
  ctrl      = 0;
  addr      = 0;
  data_l    = false;
  data_h    = false;
  cmnd      = 0;
  data_last = false;
  do_abort  = false;
  event_pending = false;
  dmc_pending = false;
}

bool SPI::check_power()
{
  bool r = true;
  
  if (state == STATE_POWR) {
    long long time = p->get_half_cycles();
    if ((time - reset_time) >= pwr_dly) {
      state = STATE_IDLE;
      r = false;
    }
  } else {
    r = false;
  }
  return r;
}

SPI::STATUS SPI::ina(unsigned short instr, signed short &data)
{
  bool skip = false;
  
  if (check_power()) return STATUS_WAIT;
  
  switch(function_code(instr)) {
  case INA_RDAT: case INA_CDAT: 
    skip = (ready() && (!mode_dmc));
    data = data_reg;
    if ((!mode_out) && (skip)) {
      data_l = data_h = false;
      if (last) {
        data_last = true;
        last = false;
      }
      addr += (mode_16) ? 2 : 1;
    }
    break;
  case INA_RLRD: case INA_CLRD: {
    uint8_t read_data = 0;
    skip = ((state == STATE_IDLE) && inner.read(read_data));
    data = (((short) inner.resp_idle()) << 8) | read_data;
  } break;
  case INA_CTRL: skip = true; data = ctrl; break;
  case INA_ARGH: skip = true; data = addr >> 15; break;
  case INA_ARGL: skip = true; data = addr & 0x7fff; break;
  default: skip = false; break;
  }

  return (skip) ? STATUS_READY : STATUS_WAIT;
}

SPI::STATUS SPI::ocp(unsigned short instr)
{
  switch(function_code(instr)) {
  case OCP_BOOT:
    if (bt_trig) {
      // Trigger boot
      if (laddrbt) {
        cmnd = boot_cmnd;
        ctrl = boot_ctrl;
        addr = boot_addr;
      }
      mode_out = false; // Boot is an input operation
      mode_16  = false; // in 8-bit mode
      mode_dmc = false; // using programmed I/O
      bt_trig  = false; // disable the boot trigger

      state_machine(true);
    }
    break;
  case OCP_OUTP: mode_out = true;  break;
  case OCP_INP:  mode_out = false; break;
  case OCP_MD16: mode_16  = true;  break;
  case OCP_MD8:  mode_16  = false; break;
  case OCP_DMC:  mode_dmc = true;  intr_dmc = false; break;
  case OCP_PIO:  mode_dmc = false; intr_dmc = false; break;
  case OCP_CWPR: wprot    = false; idle_state_machine(); break;
  case OCP_SWPR: wprot    = true;  idle_state_machine(); break;
  case OCP_DLAB: laddrbt  = false; break;
  case OCP_ELAB: laddrbt  = true;  break;
  case OCP_DBMT: bt_trig  = false; break;
  case OCP_ABMT: bt_trig  = true;  break;
  case OCP_LAST: last     = true;  break;
  case OCP_FCCI: intr_dmc = false;

    if ((state != STATE_POWR) &&
        (state != STATE_IDLE) &&
        (state != STATE_LOWL) &&
        (state != STATE_WPRT) &&
        (state != STATE_DONE) &&
        (state != STATE_DISC)) {
      do_abort = true;
      state_machine();
    }
    break;
  default: /* Do nothing */ break;
  }
  
  return STATUS_READY;
}

SPI::STATUS SPI::sks(unsigned short instr)
{
  bool skip = false;
  
  if (check_power()) return STATUS_WAIT;

  switch(function_code(instr)) {
  case SKS_RDY:   skip = (ready() && (!mode_dmc)); break;
  case SKS_RDYC:  skip = readyc(); break;
  case SKS_RLRD:  skip = ((state == STATE_IDLE) && inner.rd_valid()); break;
  case SKS_NBUSY: skip = ( state == STATE_IDLE); break;
  case SKS_NINTR: skip = !spi_pilXX(); break;
  default:        skip = false; break;
  }
  return (skip) ? STATUS_READY : STATUS_WAIT;
}

SPI::STATUS SPI::ota(unsigned short instr, signed short data)
{
  bool skip = false;

  if (check_power()) return STATUS_WAIT;

  switch(function_code(instr)) {
  case OTA_WDAT:
    skip = (ready() && (!mode_dmc));
    if (skip) {
      data_reg = data;
      data_l = true;
      last_w = last;
      last = false;
      if (mode_16) {
        data_h = true;
        addr += 2;
      } else {
        data_h = false;
        addr += 1;
      }
    }
    break;
  case OTA_CMND:
    skip = readyc();
    if (skip) {
      cmnd = data;
      state_machine(true);
    }
    break;
  case OTA_CTRL: skip = true; ctrl = data; inner.chip_select(ctrl_cs_addr());   break;
  case OTA_ARGH: skip = true; addr = ((uint32_t) data << 15) | (addr & 0x7fff); break;
  case OTA_ARGL: skip = true; addr = (addr & 0x7fff8000) | (data & 0x7fff);     break;
  default: skip = false; break;
  }
  
  return (skip) ? STATUS_READY : STATUS_WAIT;
}

SPI::STATUS SPI::smk(unsigned short mask)
{
  return STATUS_WAIT;
}

void SPI::dmc(signed short &data, bool erl)
{
  dmc_pending = false;
  if (mode_out) {
    data_reg = data;
    data_h = true;
    data_l = true;
    addr += 2;
    if (erl) {
      last_w = true;
    }
  } else {
    data = data_reg;
    data_h = false;
    data_l = false;
    addr += 2;
    if (erl) {
      last = true;
    }
  }
}

void SPI::event(int reason)
{
  switch (reason) {

  case REASON_MASTER_CLEAR:
    master_clear();
    break;
    
  case SPI_REASON_STATE:
    event_pending = false;
    if (inner.rd_valid()) {
      service_read_data();
    }
    state_machine();
    break;

  default:
    cerr << "Unexpected event = " << reason << endl;
  }
}

bool SPI::spi_pilXX()
{
  return false;
}

bool SPI::ready()
{
  return ((state == STATE_DATA) &&
          ((  mode_out  && (!data_l) && (!data_h)) ||
           ((!mode_out) &&   data_l  && ( data_h   || (!mode_16)))));
}

bool SPI::readyc()
{
  return ((state == STATE_IDLE) /*&& (wprot == wprot_ll)*/ && (!boot));
}

void SPI::service_read_data()
{
  bool rd_accept = ((!data_l) || ((!data_h) && mode_16));
  bool rd_valid = false;
  uint8_t read_data;
  
  if (rd_accept) {
    rd_valid = inner.read(read_data);
  }
  
  if (rd_valid) {
    if (mode_16 && (!data_h)) {
      data_reg = (static_cast<uint16_t>(read_data) << 8) | (data_reg & 0x00ff);
      data_h   = true;
    } else if (!data_l) {
      data_reg = mode_16 ? ((data_reg & 0xff00) | read_data) : read_data;
      data_l   = true;
    }
    if (data_l && (data_h || (!mode_16)) && mode_dmc && (!dmc_pending)) {
      p->set_dmcreq(1 << dmc_chn);
      dmc_pending = true;
    }
  }
}





void SPI::state_machine(bool strt_cmnd)
{
  bool stepping = true;
  unsigned int clocks = 0;
  unsigned int cycles = 0;
  //SPI_STATE prev_state = state;

  bool first = true; // First time around the stepping loop
  
  while (stepping) {

    clocks += 1;
    
    if (do_abort) {
      cycles = inner.write(Inner::CMND_SPEC | Inner::CMND_IDLE);
      state  = STATE_DONE;
      do_abort = false;
    } else {
      switch (state) {
      case STATE_POWR: // Should never happen
        assert(! "STATE_POWR");
        state = STATE_IDLE;
        break;
    
      case STATE_IDLE:
        if (wprot != wprot_ll) {
          // Send the write-protect status to the low-level controller
          cycles  += inner.write(Inner::CMND_SPEC   |
                                 ((wprot & 1) << 7) |
                                 Inner::CMND_WPROT);
          state    = STATE_WPRT;
        } else {

          if (boot) {
            // TODO - boot
            assert(! "boot");
          }
          
          if (strt_cmnd) {
            strt_cmnd = false;
            
            if (cmnd_addr() == CMND_ADDR_NONE) {
              // This is a raw (low-level) command - just pass it on
              cycles += inner.write(((cmnd_special()) ? Inner::CMND_SPEC : 0) |
                                    (cmnd_cmnd_byte()));
              state  = STATE_LOWL;
            } else {
              if (cmnd_addr() == CMND_ADDR_ADDR) {
                // Omit sending the command - what's next?
                if (cmnd_ch_smode() == CH_SMODE_ACMND) {
                  // Change shift mode
                  cycles += inner.write(Inner::CMND_SPEC          |
                                        (((ctrl_alt_smode()) ?
                                          Inner::SM_FOUR :
                                          Inner::SM_TWO) << 6)    |
                                        Inner::CMND_SMODE);
                  state   = STATE_SHF1;
                } else {
                  if (ctrl_addr_size()) {
                    // Go to 1st byte of 32-bit address
                    cycles += inner.write((addr >> 24) & 0x00ff);
                    state   = STATE_ADR1;
                  } else {
                    cycles += inner.write((addr >> 16) & 0x00ff);
                    state   = STATE_ADR2;
                  }
                }
              } else {
                // Send the command
                cycles += inner.write(cmnd_cmnd_byte());
                state   = STATE_CMND;
              }
            }
          } else {
            // Should not really reach here, but just in case
            stepping = false;
          }
        }
        break;
        
      case STATE_WPRT:
        if (first) {
          wprot_ll = wprot;
        }
        // Fall through
        
      case STATE_LOWL:
        if (first) {
          state = STATE_IDLE;
        }
        stepping = false;
        break;

      case STATE_CMND:
        if (cmnd_ch_smode() == CH_SMODE_ACMND) {
          // Change shift mode
          cycles += inner.write(Inner::CMND_SPEC          |
                                (((ctrl_alt_smode()) ?
                                  Inner::SM_FOUR :
                                  Inner::SM_TWO) << 6)    |
                                Inner::CMND_SMODE);
          state   = STATE_SHF1;
        } else if (cmnd_addr() == CMND_ADDR_BOTH) {
          if (ctrl_addr_size()) {
            // Go to 1st byte of 32-bit address
            cycles += inner.write((addr >> 24) & 0x00ff);
            state   = STATE_ADR1;
          } else {
            cycles += inner.write((addr >> 16) & 0x00ff);
            state   = STATE_ADR2;
          }
        } else if (cmnd_ch_smode() == CH_SMODE_NONE) {
          // No mode, dummy, or data
          cycles += inner.write(Inner::CMND_SPEC | Inner::CMND_IDLE);
          state   = STATE_DONE;
        } else if ((cmnd_md_dummy() == MD_DUMMY_ENTER_DUM) ||
                   (cmnd_md_dummy() == MD_DUMMY_EXIT_DUM )) {
          // Send mode bits
          cycles += inner.write((half_byte_mode()) ?
                                (Inner::CMND_SPEC | (xip() & 0xf0) | Inner::CMND_HALF) :
                                xip());
          state   = (half_byte_mode()) ? STATE_HMOD : STATE_MODE;
        } else if (cmnd_md_dummy() == MD_DUMMY_N_DUM) {
          // Send dummy bits
          cycles += inner.write(Inner::CMND_SPEC          |
                                (first_d_cyc() << 5)      |
                                ((mode_out) ?  Inner::CMND_DUMYW : Inner::CMND_DUMMY));
          state  = (second_dummy()) ? STATE_DUM2 : STATE_DUMM;
        } else {
          state = STATE_DATA;
        }
        break;
        
      case STATE_SHF1:
        if (cmnd_addr() == CMND_ADDR_BOTH) {
          if (ctrl_addr_size()) {
            // Go to 1st byte of 32-bit address
            cycles += inner.write((addr >> 24) & 0x00ff);
            state   = STATE_ADR1;
          } else {
            cycles += inner.write((addr >> 16) & 0x00ff);
            state   = STATE_ADR2;
          }
        } else if (cmnd_ch_smode() == CH_SMODE_NONE) {
          // No mode, dummy, or data
          cycles += inner.write(Inner::CMND_SPEC | Inner::CMND_IDLE);
          state   = STATE_DONE;
        } else if ((cmnd_md_dummy() == MD_DUMMY_ENTER_DUM) ||
                   (cmnd_md_dummy() == MD_DUMMY_EXIT_DUM )) {
          // Send mode bits
          cycles += inner.write((half_byte_mode()) ?
                                (Inner::CMND_SPEC | (xip() & 0xf0) | Inner::CMND_HALF) :
                                xip());
          state   = (half_byte_mode()) ? STATE_HMOD : STATE_MODE;
        } else if (cmnd_md_dummy() == MD_DUMMY_N_DUM) {
          // Send dummy bits
          cycles += inner.write(Inner::CMND_SPEC          |
                                (first_d_cyc() << 5)      |
                                ((mode_out) ?  Inner::CMND_DUMYW : Inner::CMND_DUMMY));
          state  = (second_dummy()) ? STATE_DUM2 : STATE_DUMM;
        } else {
          state = STATE_DATA;
        }
        break;
    
      case STATE_ADR1:
        cycles += inner.write((addr >> 16) & 0xff);
        state   = STATE_ADR2;
        break;
      
      case STATE_ADR2:
        cycles += inner.write((addr >> 8) & 0xff);
        state   = STATE_ADR3;
        break;

      case STATE_ADR3:
        cycles += inner.write(addr & 0xff);
        state   = STATE_ADR4;
        break;

      case STATE_ADR4:
        if (cmnd_ch_smode() == CH_SMODE_NONE) {
          // No mode, dummy, or data
          cycles += inner.write(Inner::CMND_SPEC | Inner::CMND_IDLE);
          state   = STATE_DONE;
        } else if ((cmnd_md_dummy() == MD_DUMMY_ENTER_DUM) ||
                   (cmnd_md_dummy() == MD_DUMMY_EXIT_DUM )) {
          // Send mode bits
          cycles += inner.write((half_byte_mode()) ?
                                (Inner::CMND_SPEC | (xip() & 0xf0) | Inner::CMND_HALF) :
                                xip());
          state   = (half_byte_mode()) ? STATE_HMOD : STATE_MODE;
        } else if (cmnd_ch_smode() == CH_SMODE_AADDR) {
          cycles += inner.write(Inner::CMND_SPEC          |
                                (((ctrl_alt_smode()) ?
                                  Inner::SM_FOUR :
                                  Inner::SM_TWO) << 6)    |
                                Inner::CMND_SMODE);
          state  = STATE_SHF2;      
        } else if (cmnd_md_dummy() == MD_DUMMY_N_DUM) {
          // Send dummy bits
          cycles += inner.write(Inner::CMND_SPEC          |
                                (first_d_cyc() << 5)      |
                                ((mode_out) ?  Inner::CMND_DUMYW : Inner::CMND_DUMMY));
          state  = (second_dummy()) ? STATE_DUM2 : STATE_DUMM;
        } else {
          state = STATE_DATA;
        }
        break;
        
      case STATE_SHF2:
        if (cmnd_ch_smode() == CH_SMODE_NONE) {
          // No mode, dummy, or data
          cycles += inner.write(Inner::CMND_SPEC | Inner::CMND_IDLE);
          state   = STATE_DONE;
        } else if ((cmnd_md_dummy() == MD_DUMMY_ENTER_DUM) ||
                   (cmnd_md_dummy() == MD_DUMMY_EXIT_DUM )) {
          // Send mode bits
          cycles += inner.write((half_byte_mode()) ?
                                (Inner::CMND_SPEC | (xip() & 0xf0) | Inner::CMND_HALF) :
                                xip());
          state   = (half_byte_mode()) ? STATE_HMOD : STATE_MODE;
        } else if (cmnd_md_dummy() == MD_DUMMY_N_DUM) {
          // Send dummy bits
          cycles += inner.write(Inner::CMND_SPEC          |
                                (first_d_cyc() << 5)      |
                                ((mode_out) ?  Inner::CMND_DUMYW : Inner::CMND_DUMMY));
          state  = (second_dummy()) ? STATE_DUM2 : STATE_DUMM;
        } else {
          state = STATE_DATA;
        }
        break;
        
      case STATE_MODE:
        if (cmnd_ch_smode() == CH_SMODE_AADDR) {
          // Change shift mode
          cycles += inner.write(Inner::CMND_SPEC          |
                                (((ctrl_alt_smode()) ?
                                  Inner::SM_FOUR :
                                  Inner::SM_TWO) << 6)    |
                                Inner::CMND_SMODE);
          state  = STATE_SHF3;
        } else if (any_dummy()) {
          // Send dummy bits
          cycles += inner.write(Inner::CMND_SPEC          |
                                (first_d_cyc() << 5)      |
                                ((mode_out) ?  Inner::CMND_DUMYW : Inner::CMND_DUMMY));
          state  = (second_dummy()) ? STATE_DUM2 : STATE_DUMM;
        } else {
          state = STATE_DATA;
        }
        break;
        
      case STATE_SHF3:
        if (any_dummy()) {
          // Send dummy bits
          cycles += inner.write(Inner::CMND_SPEC          |
                                (first_d_cyc() << 5)      |
                                ((mode_out) ?  Inner::CMND_DUMYW : Inner::CMND_DUMMY));
          state  = (second_dummy()) ? STATE_DUM2 : STATE_DUMM;
        } else {
          state = STATE_DATA;
        }
        break;

      case STATE_HMOD:
        if (cmnd_ch_smode() == CH_SMODE_AADDR) {
          // Change shift mode
          cycles += inner.write(Inner::CMND_SPEC          |
                                (((ctrl_alt_smode()) ?
                                  Inner::SM_FOUR :
                                  Inner::SM_TWO) << 6)    |
                                Inner::CMND_SMODE);
          state  = STATE_SHF4;
        } else {
          // Send half a byte's worth of dummy cycles
          cycles += inner.write(Inner::CMND_SPEC          |
                                (half_d_cyc() << 5)       |
                                Inner::CMND_DUMMY);
          state  = STATE_DUMM;
        }
        break;
        
    
      case STATE_SHF4:
        // Send half a byte's worth of dummy cycles
        cycles += inner.write(Inner::CMND_SPEC          |
                              (half_d_cyc() << 5)       |
                              Inner::CMND_DUMMY);
        state  = STATE_DUMM;
        break;

      case STATE_DUM2:
        cycles += inner.write(Inner::CMND_SPEC          |
                              (second_d_cyc() << 5)      |
                              ((mode_out) ?  Inner::CMND_DUMYW : Inner::CMND_DUMMY));
        state  = STATE_DUMM;
        break;
        
      case STATE_DUMM:
        state = STATE_DATA;
        cycles = 1;
        break;
        
      case STATE_DATA: {
        if (mode_out) {
          if (data_l || data_h) {
            cycles += inner.write(((mode_16 && data_h) ? (data_reg >> 8) : data_reg) & 0xff);
            data_l = (data_l && mode_16 && data_h);
            data_h = false;
            stepping = false;
          } else if (last_w) {
              last_w = false;
              cycles += inner.write(Inner::CMND_SPEC | Inner::CMND_IDLE);
              state = STATE_DONE;
              stepping = false;
          } else {
            stepping = false;
            if (mode_dmc && (!dmc_pending)) {
              p->set_dmcreq(1 << dmc_chn);
              dmc_pending = true;
            }
          }
        } else {
          // Read
          
          if ((mode_dmc && last) || data_last) {
            last = data_last = false;
            cycles += inner.write(Inner::CMND_SPEC | Inner::CMND_IDLE);
            state = STATE_DONE;
            stepping = false;
            data_h = false;
            data_l = false;
          } else {
            if (inner.accept()) {
              cycles += inner.write(Inner::CMND_SPEC | Inner::CMND_READ);
              stepping = false;
            }
            stepping = false;
          }
        }
      } break;
        
      case STATE_DONE:
        // If the IDLE command has been accepted move on
        state = STATE_DISC;
        break;
    
      case STATE_DISC:
        // Discard any return data and wait for response to IDLE
        data_h    = false;
        data_l    = false;

        if (first) {
          // Called from event call-back
          state = (wprot != wprot_ll) ? STATE_WPRT : STATE_IDLE;
          stepping = false;
        } else if (inner.rd_empty() && inner.resp_idle()) {
          // Will go to STATE_IDLE in call-back
          stepping = false;
        } else {
          inner.rd_discard();
        }
        break;
        
      default:
        state = STATE_IDLE;
        break;
      }
      first = false;
    }

#if 0
    cout << __PRETTY_FUNCTION__ << ": " << std::dec
         << p->get_half_cycles() << ": "
         << prev_state << " to "
         << state
         << " stepping = " << stepping
         << endl;
    prev_state = state;
#endif
  }

  unsigned long long half_cycles = clocks + cycles * 3; // TODO - magic numbers...
  
  if ((half_cycles != 0) && (state != STATE_IDLE)) {
    assert(!event_pending);
    p->queue_hc(half_cycles, this, SPI_REASON_STATE);
    event_pending = true;
  }
}

void SPI::idle_state_machine()
{
  if (state == STATE_IDLE) {
    state_machine();
  }
}

/***********************************************************************
 * Inner controller
 **********************************************************************/

SPI::Inner::Inner(SpiDev *_devices[CHIP_SELECTS])
  : sm(SM_ONE)
  , wprot(true)
  , select(0)
{
  for (unsigned i = 0; i < CHIP_SELECTS; i++)
    devices[i] = _devices[i];
}

SPI::Inner::~Inner()
{
}

void SPI::Inner::chip_select(unsigned n)
{
  assert(n < CHIP_SELECTS);
  select = n;
}

unsigned int SPI::Inner::write(unsigned int data)
{
  unsigned int cycles = ((sm==SM_FOUR) ? 2 : (sm==SM_TWO) ? 4 : 8);
  
  if ((data & CMND_SPEC) != 0) {
    // Special command
    switch(data & 0x0f) {
    case CMND_READ:
      if (!write_data.empty()) {
         devices[select]->write(wprot, write_data);
         assert(write_data.empty());
      }
      read_data.push_back(devices[select]->read());
      break;
    case CMND_IDLE: /* IDLE */
      if (!write_data.empty()) {
         devices[select]->write(wprot, write_data);
         assert(write_data.empty());
      }
      //flash.deselect();
      cycles = 2;
      idle = true;
      break;
    case CMND_DUMMY: /* DUMMY */
      cycles = 8 - ((data >> 5) & 0x07);
      //flash.dummy(cycles);
      break;
    case CMND_SMODE:
      sm = static_cast<SM>((data >> 6) & 0x3);
      cycles = 1;
      break;
    case CMND_WPROT:
      wprot = (((data >> 7) & 0x1) != 0);
      cycles = 1;
      break;
    case CMND_HALF:
      write_data.push_back(data & 0x00f0); // Push a whole byte?
      cycles /= 2;
      break;
    default:
      cerr << "Unexpected inner command = " << (data & 0x0f) << endl;
    }
  } else {
    // Regular write data
    write_data.push_back(data & 0x00ff);
  }
  return cycles;
}

bool SPI::Inner::rd_valid()
{
  return (!read_data.empty());
}

bool SPI::Inner::rd_empty()
{
  return read_data.empty();
}

void SPI::Inner::rd_discard()
{
  if (!read_data.empty()) {
    read_data.pop_front();
  }
}

bool SPI::Inner::read(std::uint8_t &data)
{
  bool r = false;
  if (rd_valid()) {
    data = read_data.front();
    read_data.pop_front();
    r = true;
  }
  return r;
}

bool SPI::Inner::accept()
{
  return read_data.size() <= 4;
}
 
bool SPI::Inner::resp_idle()
{
  bool r = idle;
  idle = false;
  return r;
}
