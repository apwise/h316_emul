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
#ifndef _PTR_HH_
#define _PTR_HH_

class Proc;
class STDTTY;

#include "tty_file.hh"

class PTR : public IODEV
{
public:
  PTR(Proc *p, STDTTY *stdtty);
  STATUS ina(unsigned short instr, signed short &data);
  STATUS ocp(unsigned short instr);
  STATUS sks(unsigned short instr);
  STATUS ota(unsigned short instr, signed short data);
  STATUS smk(unsigned short mask);

  void event(int reason);
  void set_filename(char *filename);

private:
  
  void master_clear(void);
  void open_file(void);
  void start_reader(void);

  Proc *p;
  STDTTY *stdtty;

  TTY_file tty_file;
  bool pending_filename;
  char *filename;

  bool eot; // End Of Tape
  unsigned int eot_counter;

  unsigned short mask;
  
  bool tape_running;
  bool ready;
  unsigned char data_buf;

  bool ignore_event;
  int events_queued;

  int data_count;
};

#endif // _PTR_HH_
