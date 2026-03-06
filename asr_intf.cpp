/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 2005, 2026  Adrian Wise
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
 */

#include "asr_intf.hpp"

#include <cstdlib>
#include <cstdio>
#include "stdtty.hpp"

#include "proc.hpp"
#include "iodev.hpp"
#include "asr.hpp"

#define BAUD 9600 // 110
#define SMK_MASK (1 << (16-11))

#define STOP_CODE 0023

#define iEvent(x) static_cast<int>(Event::x)
#define MILLISECONDS_CHAR ((1000000*11)/ BAUD)
#define MILLISECONDS_DUMMY ((1000000*1)/ BAUD)

AsrIntf::AsrIntf(IoToPIntf &p)
  : IoDev(p)
  , asr(nullptr)
{
  asr = new ASR();

  master_clear();
}

AsrIntf::~AsrIntf() {
  if (asr) delete asr;
}

const char *AsrIntf::name() {
  return "ASR";
}

void AsrIntf::master_clear()
{
  mask = 0;

  data_buf = 0;
  ready = false;
  input_pending = false;

  output_mode = false;
  output_pending = false;
  activity = Activity::NONE;
}

AsrIntf::Status AsrIntf::ina(uint16_t instr, int16_t &data)
{
  char c;
  bool r = ready;

  if (ready)
    {
      switch(instr & 0777)
        {
        case 0004: data = data_buf; break;
        case 0204: data = data_buf & 0x3f; break;
        default:
          p.anomaly(IoToPIntf::Level::ERROR, message(instr));
        }
                        
      input_pending = false;
      ready = false;
      p.clear_interrupt(SMK_MASK);
    }
  else
    {
      if (!input_pending)
        {
          if (asr->get_asrch(c))
            {
              data_buf = c & 0xff;
                                                        
              activity = Activity::INPUT;
              input_pending = true;

              p.queue(MILLISECONDS_CHAR, *this, iEvent(INPUT));
            }
        }
    }

  return status(r);
}

void AsrIntf::ocp(uint16_t instr)
{
  switch(instr & 0700)
    {
    case 0000:
                                                
      output_mode = 0;
      activity = Activity::NONE;
      ready = false;
      p.clear_interrupt(SMK_MASK);
      break;

    case 0100:
                                                
      output_mode = 1;
      activity = Activity::DUMMY;
      ready = 1;
      p.set_interrupt(mask);

      p.queue(MILLISECONDS_DUMMY, *this, iEvent(DUMMY_CYCLE) );
                        
      break;
                        
    default:
      p.anomaly(IoToPIntf::Level::ERROR, message(instr));
    }
}

AsrIntf::Status AsrIntf::sks(uint16_t instr)
{
  bool r = false;

  switch(instr & 0700)
    {
    case 0000: r = ready; break;
    case 0100: r = (activity == Activity::NONE); break;
    case 0400: r = !(ready && mask); break;
    case 0500: r = ((!output_mode) && ready &&
                    ((data_buf & 0x7f) == STOP_CODE)); break;
    default:
      p.anomaly(IoToPIntf::Level::ERROR, message(instr));
    }

  return status(r);
}

AsrIntf::Status AsrIntf::ota(uint16_t instr, int16_t data)
{
  bool r = ready;

  if (ready)
    {
      switch(instr & 0700)
        {
        case 0000: data_buf = data; break;
        case 0200:
          data_buf = (data & 0x80) |
            ((~data & 0x20) << 1) |
            (data & 0x3f);
          break;
        default:
          p.anomaly(IoToPIntf::Level::ERROR, message(instr));
        }
                        
      ready = 0;
      p.clear_interrupt(SMK_MASK);

      if (activity == Activity::DUMMY)
        output_pending = true;
      else
        {
          activity = Activity::OUTPUT;
          p.queue(MILLISECONDS_CHAR, *this, iEvent(OUTPUT) );
        }
    }

  return status(r);
}

void AsrIntf::smk(uint16_t mask)
{
  this->mask = mask & SMK_MASK;

  if (ready && this->mask)
    p.set_interrupt(this->mask);
  else
    p.clear_interrupt(SMK_MASK);
}

void AsrIntf::event(int reason)
{
  const Event event {static_cast<Event>(reason)};

  switch (event) {
    
  case Event::MASTER_CLEAR:
    master_clear();
    break;
    
  case Event::DUMMY_CYCLE:
    if (activity == Activity::DUMMY) {
      if (output_pending) {
        activity = Activity::OUTPUT;
        p.queue(MILLISECONDS_CHAR, *this, iEvent(OUTPUT) );
        output_pending = false;
      } else {
        activity = Activity::NONE;
      }
    }
    break;
    
  case Event::OUTPUT:
    if (activity == Activity::OUTPUT) {
      activity = Activity::NONE;
      ready = 1;
      p.set_interrupt(mask);
      
      asr->put_asrch(data_buf);
    }
    break;
    
  case Event::INPUT:
    if (activity == Activity::INPUT) {
      activity = Activity::NONE;
      input_pending = false;
      ready = true;
        p.set_interrupt(mask);
    }
    break;
    
  case Event::PTR_ON:
    asr->ptr_on();
    break;
    
  case Event::PTP_ON:
    asr->ptp_on();
    break;
    
  default:
    p.anomaly(IoToPIntf::Level::FATAL, uxReason(reason));
  }
}

void AsrIntf::set_filename(const std::string &filename, unsigned subdevice) {
  // Forward to the ASR
  p.anomaly(IoToPIntf::Level::FATAL, "Unexpected subdevice");
  asr->set_filename(filename, subdevice);
}

bool AsrIntf::special(char c)
{
  return asr->special(c);
}

DEFINE_UNEXPECTED_DMC(AsrIntf)
