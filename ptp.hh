/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 1999, 2004, 2005  Adrian Wise
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
class STDTTY;

class PTP : public IODEV
{
 public:
  PTP(Proc *p, STDTTY *stdtty);
  bool ina(unsigned short instr, signed short &data);
  void ocp(unsigned short instr);
  bool sks(unsigned short instr);
  bool ota(unsigned short instr, signed short data);
  void smk(unsigned short mask);

  void event(int reason);
  void set_filename(char *filename);

 private:
  void master_clear(void);
  void turn_power_on(void);

  Proc *p;
  STDTTY *stdtty;

  FILE *fp;
  bool pending_filename;
  char *filename;
  
  bool ascii_file;
  bool ready;
  bool power_on;
  
  unsigned short mask;
};
