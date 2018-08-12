/* Honeywell Series 16 emulator
 * Copyright (C) 2011  Adrian Wise
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iodev.hh"
#include "stdtty.hh"

#include "proc.hh"
#include "rtc.hh"

#define SMK_MASK (1 << (16-16))

#define RATE 60 /* Ticks per second */

enum RTC_REASON {
  RTC_REASON_TICK,  
  RTC_REASON_NUM
};

static const char *rtc_reason[RTC_REASON_NUM] __attribute__ ((unused)) =
{
  "tick"
};

RTC::RTC(Proc *p)
  : IODEV(p)
{
  this->p = p;

  master_clear();
}

void RTC::master_clear()
{
  interrupting = false;
  mask = 0;
};

RTC::STATUS RTC::ina(unsigned short instr, signed short &data)
{
  bool r = false;
 
  return status(r);
}

RTC::STATUS RTC::ocp(unsigned short instr)
{
  switch(instr & 0700) {
  case 0000: /* Reset interrupt request and start clock */
    interrupting = false;
    p->clear_interrupt(SMK_MASK);
    if (!running)
      p->queue((1000000 / RATE), this, RTC_REASON_TICK );
    running = true;
    break;
    
  case 0200: /* Reset interrupt request and stop clock */
    interrupting = false;
    p->clear_interrupt(SMK_MASK);
    running = false;
    break;
    
  default:
    fprintf(stderr, "RTC: OCP '%04o\n", instr&0x3ff);
    exit(1);
  }
  return STATUS_READY;
}

RTC::STATUS RTC::sks(unsigned short instr)
{
  bool r = false;

  switch(instr & 0700) {
  case 0000: r = !(interrupting && mask); break;
    
  default:
    fprintf(stderr, "RTC: SKS '%04o\n", instr&0x3ff);
    exit(1);
  }
  
  return status(r);
}

RTC::STATUS RTC::ota(unsigned short instr, signed short data)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  exit(1);

  return STATUS_READY;
}

RTC::STATUS RTC::smk(unsigned short mask)
{
  this->mask = mask & SMK_MASK;

  if (interrupting && this->mask)
    p->set_interrupt(this->mask);
  else
    p->clear_interrupt(SMK_MASK);

  return STATUS_READY;
}

void RTC::event(int reason)
{
  switch(reason) {
  case REASON_MASTER_CLEAR:
    master_clear();
    break;
    
  case RTC_REASON_TICK:
    if (running) {
      p->queue((1000000 / RATE), this, RTC_REASON_TICK );
      /* Request a single execution cycle on CPU */
      p->set_rtclk(true);
    }

    break;
    
  default:
    fprintf(stderr, "%s %d\n", __PRETTY_FUNCTION__, reason);
    exit(1);
    break;
  }
}

void RTC::rollover()
{
  interrupting = true;
 
  if (interrupting && this->mask)
    p->set_interrupt(this->mask);
}
