/* Honeywell Series 16 emulator
 * Copyright (C) 2011, 2026  Adrian Wise
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
#include "rtc.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "iodev.hpp"
#include "stdtty.hpp"

#include "proc.hpp"

#define SMK_MASK (1 << (16-16))

#define RATE 60 /* Ticks per second */
#define iEvent(x) static_cast<int>(Event::x)

const char *RTC::name() {
  return "RTC";
}

RTC::RTC(IoToPIntf &p)
  : IoDev(p)
{
  this->p = p;

  master_clear();
}

void RTC::master_clear()
{
  interrupting = false;
  mask = 0;
};

RTC::Status RTC::ina(uint16_t instr, int16_t &data)
{
  bool r = false;

  p.anomaly(IoToPIntf::Level::ERROR, message(instr));
 
  return status(r);
}

void RTC::ocp(uint16_t instr)
{
  switch(instr & 0700) {
  case 0000: /* Reset interrupt request and start clock */
    interrupting = false;
    p.clear_interrupt(SMK_MASK);
    if (!running)
      p.queue((1000000 / RATE), *this, static_cast<int>(RTC::Event::TICK));
    running = true;
    break;
    
  case 0200: /* Reset interrupt request and stop clock */
    interrupting = false;
    p.clear_interrupt(SMK_MASK);
    running = false;
    break;
    
  default:
    p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  }
}

RTC::Status RTC::sks(uint16_t instr)
{
  bool r = false;

  switch(instr & 0700) {
  case 0000: r = !(interrupting && mask); break;
    
  default:
    p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  }
  
  return status(r);
}

RTC::Status RTC::ota(uint16_t instr, int16_t data)
{
  p.anomaly(IoToPIntf::Level::ERROR, message(instr));

  return Status::WAIT;
}

void RTC::smk(uint16_t mask)
{
  this->mask = mask & SMK_MASK;

  if (interrupting && this->mask)
    p.set_interrupt(this->mask);
  else
    p.clear_interrupt(SMK_MASK);
}

void RTC::event(int reason)
{
  Event event {static_cast<Event>(reason)};

  switch(event) {
  case Event::MASTER_CLEAR:
    master_clear();
    break;
    
  case Event::TICK:
    if (running) {
      p.queue((1000000 / RATE), *this, iEvent(TICK) );
      /* Request a single execution cycle on CPU */
      p.set_break(0);
    }
    break;
    
  case Event::ROLLOVER:
    interrupting = true;
    
    if (interrupting && this->mask)
      p.set_interrupt(this->mask);
    break;
    
  default:
    p.anomaly(IoToPIntf::Level::FATAL, uxReason(reason));
    break;
  }
}

DEF_NULL_SET_FILENAME(RTC)
