/* Honeywell Series 16 emulator $Id: iodev.hh,v 1.2 2004/04/21 21:20:27 adrian Exp $
 * Copyright (C) 1997, 1998  Adrian Wise
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
 * $Log: iodev.hh,v $
 * Revision 1.2  2004/04/21 21:20:27  adrian
 * Batch operation and line-printer
 *
 * Revision 1.1  1999/02/20 00:06:35  adrian
 * Initial revision
 *
 */

class Proc;
class STDTTY;

#define PTR_DEVICE 1
#define PTP_DEVICE 2
#define LPT_DEVICE 3
#define ASR_DEVICE 4

class IODEV
{
 public:
  virtual bool ina(unsigned short instr, signed short &data);
  virtual void ocp(unsigned short instr);
  virtual bool sks(unsigned short instr);
  virtual bool ota(unsigned short instr, signed short data);
  virtual void smk(unsigned short mask);
  
  virtual void event(int reason);
  
  static IODEV **dispatch_table(Proc *p, STDTTY *stdtty);
  static void master_clear_devices(IODEV **dt);
  static void set_mask(IODEV **dt, unsigned short mask);
};

#define REASON_MASTER_CLEAR -1
