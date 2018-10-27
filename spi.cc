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
     (state == SPI::STATE_DUMM) ? "DUMM" :
     (state == SPI::STATE_DATA) ? "DATA" :
     (state == SPI::STATE_WLST) ? "WLST" :
     (state == SPI::STATE_DONE) ? "DONE" :
     (state == SPI::STATE_DISC) ? "DISC" : "????");
  os << s;  
  return os;  
}

SPI::SPI(Proc *p,
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
  
  reset_time = p->get_half_cycles();
  state = STATE_POWR;

  wprot_ll  = true;
  boot      = false;
  data_reg = 0;
  ctrl = 0;
  addr = 0;
  data_l = false;
  data_h = false;
  cmnd = 0;
  data_last = false;
  read_paused = true;
  do_abort = false;
}

bool SPI::check_power()
{
  bool r = true;

  //cout << __PRETTY_FUNCTION__ << endl;
  
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
    if (!mode_out) {
      data_l = data_h = false;
      if (last) {
        data_last = true;
        last = false;
      }
      addr += (mode_16) ? 2 : 1;
      if (read_paused) {
        unsigned int cycles = inner.write(Inner::CMND_SPEC | Inner::CMND_READ);
        p->queue_hc(cycles, this, SPI_REASON_STATE);
        read_paused = false;
      }
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
  case OCP_CWPR: wprot    = false; break;
  case OCP_SWPR: wprot    = true;  break;
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
        (state != STATE_WLST) &&
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
    data_reg = data;
    data_l = true;
    if (mode_16) {
      data_h = true;
      addr += 2;
    } else {
      addr += 1;
    }
    break;
  case OTA_CMND:
    skip = readyc();
    cmnd = data;
    if (skip) {
      state_machine(true);
    }
    break;
  case OTA_CTRL: skip = true; ctrl = data; break;
  case OTA_ARGH: skip = true; addr = ((uint32_t) data << 15) | (addr & 0x7fff); break;
  case OTA_ARGL: skip = true; addr = (addr & 0x7fff0000) | data; break;
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
  //cout << __PRETTY_FUNCTION__ << endl;
  
  if (mode_out) {
    if (erl) {
      last = true;
      // set interrupt
    } else {
      data_reg = data;
      data_h = true;
      data_l = true;
      //state_machine();
    }
  } else {
    if (erl) {
      last = true;
      // set interrupt
    } else {
      data = data_reg;
      data_h = false;
      data_l = false;
      if (read_paused) {
        state_machine();
      }
    }
  }
}

void SPI::event(int reason)
{
  //cout << "SPI::event(" << reason << ")" << endl;
  
  switch (reason) {

  case REASON_MASTER_CLEAR:
    master_clear();
    break;
    
  case SPI_REASON_STATE:
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
  return ((state == STATE_IDLE) && (wprot == wprot_ll) && (!boot));
}

void SPI::state_machine(bool start)
{
  unsigned int cycles = 0;
  //SPI_STATE prev_state = state;
  
  if (do_abort) {
    cycles = inner.write(Inner::CMND_SPEC | Inner::CMND_IDLE);
    state  = STATE_DONE;
    do_abort = false;
  } else {
    switch (state) {
    case STATE_POWR: // Should never happen
      state = STATE_IDLE;
      break;
    
    case STATE_IDLE:
      if (wprot != wprot_ll) {
        // Send the write-protect status to the low-level controller
        cycles   = inner.write(Inner::CMND_SPEC   |
                               ((wprot & 1) << 7) |
                               Inner::CMND_WPROT);
        state    = STATE_WPRT;
        wprot_ll = wprot;
        break;
      } else if (start) {
        if ((cmnd & 0xfe00) == 0) {
          // This is a raw (low-level) command - just pass it on
          cycles = inner.write((((cmnd & 0x0100) != 0) ?
                                Inner::CMND_SPEC : 0) |
                               (cmnd & 0x00ff));
          state  = STATE_LOWL;
          break;
        } else {
          if ((cmnd & 0xc000) != 0xc000) {
            // i.e. Command is *not* omitted
            cycles = inner.write(cmnd & 0x00ff);
            state  = STATE_CMND;
            break;
          }
          // else - fall through to STATE_CMND
        }
      } else {
        break;
      }
      // Fall through
    
    case STATE_CMND:
      if ((cmnd & 0x3000) == 0x2000) {
        // Change shift mode
        cycles = inner.write(Inner::CMND_SPEC          |
                             ((((ctrl & 0x0800) != 0) ?
                               Inner::SM_FOUR :
                               Inner::SM_TWO) << 6)    |
                             Inner::CMND_SMODE);
        state  = STATE_SHF1;
        break;
      }
      // Fall through
    
    case STATE_SHF1:
      if ((cmnd & 0x8000) != 0) {
        // Go to 1st byte of 32-bit address
        cycles = inner.write((addr >> 24) & 0x00ff);
        state  = STATE_ADR1;
        break;
      } else if ((cmnd & 0xc000) == 0x4000) {
        cycles = inner.write((addr >> 16) & 0x00ff);
        state  = STATE_ADR2;
        break;
      }
      // Fall through
    
    case STATE_ADR4:
      if ((state == STATE_ADR4) && /* i.e. didn't fall through */
          ((cmnd & 0x3000) == 0x3000)) {
        // Change shift mode
        cycles = inner.write(Inner::CMND_SPEC          |
                             ((((ctrl & 0x0800) != 0) ?
                               Inner::SM_FOUR :
                               Inner::SM_TWO) << 6)    |
                             Inner::CMND_SMODE);
        state  = STATE_SHF2;      
      }
      // Fall through
    
    case STATE_SHF2:
      if ((cmnd & 0x3000) == 0) {
        // No mode, dummy, or data
        cycles = inner.write(Inner::CMND_SPEC | Inner::CMND_IDLE);
        state  = STATE_DONE;
      } else if ((cmnd & 0x0800) != 0) {
        // Send mode bits
        cycles = inner.write(Inner::CMND_SPEC          |
                             (((cmnd & 0x0400) != 0) ?
                              ((ctrl << 4) & 0xf0)   :
                              ( ctrl       & 0xf0))    |
                             Inner::CMND_HALF);
        state  = STATE_MODE;
      } else if ((cmnd & 0x0c00) == 0x0400) {
        // Send dummy bits
        cycles = inner.write(Inner::CMND_SPEC          |
                             ((ctrl >> (8-5)) & 0xe0)  |
                             Inner::CMND_DUMMY);
        state  = STATE_DUMM;
      } else {
        // Go to data
        state  = STATE_DATA;
      }
      break;
    
    case STATE_LOWL:
    case STATE_WPRT:
      state = STATE_IDLE;
      break;
    
    case STATE_ADR1:
      cycles = inner.write((addr >> 16) & 0xff);
      state  = STATE_ADR2;
      break;
      
    case STATE_ADR2:
      cycles = inner.write((addr >> 8) & 0xff);
      state  = STATE_ADR3;
      break;

    case STATE_ADR3:
      cycles = inner.write(addr & 0xff);
      state  = STATE_ADR4;
      break;

    case STATE_MODE:
      if ((cmnd & 0x0c00) == 0x0400) {
        // Send dummy bits
        cycles = inner.write(Inner::CMND_SPEC          |
                             ((ctrl >> (8-5)) & 0xe0)  |
                             Inner::CMND_DUMMY);
        state  = STATE_DUMM;
        break;
      }
      // Fall through
    
    case STATE_DUMM:
      state = STATE_DATA;
      cycles = 1;
      break;
        
    case STATE_DATA: {
      bool write = true;
      unsigned int write_data = 0;
    
      if ((cmnd & 0x0200) != 0) {
        // Read

        write_data |= Inner::CMND_SPEC;
      
        if ((mode_dmc && last) || data_last) {
          write_data |= Inner::CMND_IDLE;
          state      = STATE_DONE;
          last       = false;
          data_h     = false;
          data_l     = false;
        } else {
          write_data |= Inner::CMND_READ;

          if (inner.accept()) {
            uint8_t read_data;
            bool rd_valid = inner.read(read_data);
            bool rd_accept = ((!data_l) || ((!data_h) && mode_16) || data_last);
          
            if (rd_valid && rd_accept) {
              if (mode_16 && (!data_h)) {
                data_reg = (static_cast<uint16_t>(read_data) << 8) | (data_reg & 0x00ff);
                data_h   = true;
              } else if (!data_l) {
                data_reg = (data_reg * 0xff00) | read_data;
                data_l   = true;
              }

              if (mode_dmc && data_l && ((!mode_16) || data_h)) {
                //cout << "Request DMC chan=" << dmc_chn << endl;
                p->set_dmcreq(1 << dmc_chn);
              }
            }
          } else {
            write = false;
            read_paused = true;
          }
        }
        data_last = false;
      } else {
        // Write
        if (mode_16 && data_h) {
          write_data = (data_reg >> 8) & 0xff;
          data_h     = false;
        } else if (data_l) {
          write_data = data_reg & 0xff;
          data_l     = false;
          if (last) {
            state = STATE_WLST;
            last  = false;
          }
        }
      }
      if (write) {
        cycles = inner.write(write_data);
      }
      break;
    }
  
    case STATE_WLST:
      cycles = inner.write(Inner::CMND_SPEC | Inner::CMND_IDLE);
      state  = STATE_DONE;
      break;
    
    case STATE_DONE:
      // If the IDLE command has been accepted move on
      state = STATE_DISC;
      break;
    
    case STATE_DISC: {
      // Discard any return data and wait for response to IDLE
      data_h    = false;
      data_l    = false;

      uint8_t read_data;
      bool rd_valid = inner.read(read_data);

      if (rd_valid && inner.resp_idle()) {
        state = STATE_IDLE;
      }
    } break;
    
    default:
      state = STATE_IDLE;
      break;
    }
  }

#if 0
  if (state != prev_state) {
    cout << __PRETTY_FUNCTION__ << ": "
         << p->get_half_cycles() << ": "
         << prev_state << " to "
         << state << endl;
  }
#endif
  
  if (cycles != 0) {
    p->queue_hc(cycles, this, SPI_REASON_STATE);
  }
}

/***********************************************************************
 * Inner controller
 **********************************************************************/

SPI::Inner::Inner()
  : sm(SM_ONE)
  , wprot(true)
{
}

SPI::Inner::~Inner()
{
}

unsigned int SPI::Inner::write(unsigned int data)
{
  unsigned int cycles = ((sm==SM_FOUR) ? 2 : (sm==SM_TWO) ? 4 : 8);
  
  if ((data & CMND_SPEC) != 0) {
    // Special command
    switch(data & 0x0f) {
    case CMND_READ:
      read_data.push(flash.read());
      break;
    case CMND_IDLE: /* IDLE */
      flash.abort();
      cycles = 2;
      idle = true;
      break;
    case CMND_DUMMY: /* DUMMY */
      cycles = 8 - ((data >> 5) & 0x07);
      flash.dummy(cycles);
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
      flash.write((data >> 4) & 0xf, Flash::HALF);
      cycles /= 2;
      break;
    default:
      cerr << "Unexpected inner command = " << (data & 0x0f) << endl;
    }
  } else {
    // Regular write data
    flash.write(data & 0x00ff);
  }
  return cycles;
}

bool SPI::Inner::rd_valid()
{
  // Always keep one in the queue to represent the one byte
  // delay between starting to shift the data, and it being
  // read by the controller.
  
  return (read_data.size() >= 2);
}

bool SPI::Inner::read(std::uint8_t &data)
{
  bool r = false;
  if (rd_valid()) {
    data = read_data.front();
    read_data.pop();
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
