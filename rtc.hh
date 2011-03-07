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

class Proc;

class RTC : public IODEV
{
public:
  RTC(Proc *p);
  STATUS ina(unsigned short instr, signed short &data);
  STATUS ocp(unsigned short instr);
  STATUS sks(unsigned short instr);
  STATUS ota(unsigned short instr, signed short data);
  STATUS smk(unsigned short mask);

  void event(int reason);
  void rollover();

private:
  void master_clear(void);

  Proc *p;

  unsigned short mask;
  bool running;
  bool interrupting;
};
