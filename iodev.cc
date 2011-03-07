/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005  Adrian Wise
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

#include "stdtty.hh"
#include "iodev.hh"

#include "ptr.hh"
#include "ptp.hh"
#include "asr_intf.hh"
#include "lpt.hh"
#include "rtc.hh"
#include "plt.hh"
#include "proc.hh"

IODEV::IODEV(Proc *p)
  : p(p)
{
}

IODEV::~IODEV()
{
}

IODEV::STATUS IODEV::ina(unsigned short instr, signed short &data)
{
  fprintf(stderr, "INA with undefined device: '%02o\n", instr & 0x3f);
  p->abort();
}

IODEV::STATUS IODEV::ocp(unsigned short instr)
{
  fprintf(stderr, "OCP with undefined device: '%02o\n", instr & 0x3f);
  p->abort();
}

IODEV::STATUS IODEV::sks(unsigned short instr)
{
  fprintf(stderr, "SKS with undefined device: '%02o\n", instr & 0x3f);
  p->abort();
}

IODEV::STATUS IODEV::ota(unsigned short instr, signed short data)
{
  fprintf(stderr, "OTA with undefined device: '%02o\n", instr & 0x3f);
  p->abort();
}

IODEV::STATUS IODEV::smk(unsigned short mask)
{
  // quite likely to get here from set_mask();
}

void IODEV::event(int reason)
{
  if (reason != REASON_MASTER_CLEAR)
    {
      fprintf(stderr, "Event called for undefined device with reason %d\n",
              reason);
      p->abort();
    }
}

IODEV **IODEV::dispatch_table(Proc *p, STDTTY *stdtty)
{
  IODEV **dt = (IODEV **) malloc(sizeof(IODEV *) * 64);
  int i;
  IODEV *dummy = new IODEV(p);
  
  for(i=0; i<64; i++)
    dt[i] = dummy;
  
  dt[ASR_DEVICE] = new ASR_INTF(p, stdtty);
  dt[PTR_DEVICE] = new PTR(p, stdtty);
  dt[PTP_DEVICE] = new PTP(p, stdtty);
  dt[LPT_DEVICE] = new LPT(p, stdtty);
  dt[RTC_DEVICE] = new RTC(p);
  dt[PLT_DEVICE] = new PLT(p, stdtty);
  
  return dt;
} 

void IODEV::master_clear_devices(IODEV **dt)
{
  int i;
  
  for(i=0; i<64; i++)
    dt[i]->event(REASON_MASTER_CLEAR);
}

void IODEV::set_mask(IODEV **dt, unsigned short mask)
{
  int i;
  
  for(i=0; i<64; i++)
    dt[i]->smk(mask);
}
