/* Honeywell Series 16 emulator $Id: asr_intf.hh,v 1.1 1999/02/20 00:06:35 adrian Exp $
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
 * $Log: asr_intf.hh,v $
 * Revision 1.1  1999/02/20 00:06:35  adrian
 * Initial revision
 *
 */

class Proc;
class ASR;
class STDTTY;

enum ASR_ACTIVITY
{
	ASR_ACTIVITY_NONE,
	ASR_ACTIVITY_OUTPUT,
	ASR_ACTIVITY_INPUT,
	ASR_ACTIVITY_DUMMY
};

class ASR_INTF : public IODEV
{
 public:
  ASR_INTF(Proc *p, STDTTY *stdtty);
  bool ina(unsigned short instr, signed short &data);
  void ocp(unsigned short instr);
  bool sks(unsigned short instr);
  bool ota(unsigned short instr, signed short data);
  void smk(unsigned short mask);

	void event(int reason);
	void set_filename(char *filename);
	bool special(char c);
	
 private:
	void master_clear(void);

	Proc *p;

	unsigned short mask; // just set the one bit for this device

	int data_buf;
	bool ready;
	bool input_pending;

	bool output_mode;
	bool output_pending;
	enum ASR_ACTIVITY activity;

  ASR *asr;
};
