/* Honeywell Series 16 emulator
 * Copyright (C) 1998, 2004, 2005  Adrian Wise
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

class STDTTY;
class Proc;

struct CmdTab;

class Monitor
{
public:
  Monitor(Proc *p, STDTTY *stdtty, int argc, char **argv);
  void do_commands(bool &run, FILE **fp);
  
private:
  Proc *p;
  STDTTY *stdtty;
  int argc;
  char **argv;
  
  bool first_time;
  bool doing_commands;
  
  static struct CmdTab commands[];
  
  void get_line(char *prompt, FILE **fp, char *buffer, int buf_size);
  char **break_command(char *buf, int &word);
  void free_command(char **q);
  void do_command(bool &run, int words, char **cmd);
  long parse_number(char *str, bool &ok);
  bool parse_bool(char *str, bool &ok);
  char *binary16(unsigned short n);
  bool reg(bool &run, int words, char **cmd, int n);
  
  bool quit(bool &run, int words, char **cmd);
  bool cont(bool &run, int words, char **cmd);
  bool stop(bool &run, int words, char **cmd);
  bool go(bool &run, int words, char **cmd);
  bool ss(bool &run, int words, char **cmd);
  bool ptr(bool &run, int words, char **cmd);
  bool ptp(bool &run, int words, char **cmd);
  bool lpt(bool &run, int words, char **cmd);
  bool asr_ptr(bool &run, int words, char **cmd);
  bool asr_ptp(bool &run, int words, char **cmd);
  bool asr_ptp_on(bool &run, int words, char **cmd);
  bool a(bool &run, int words, char **cmd);
  bool b(bool &run, int words, char **cmd);
  bool x(bool &run, int words, char **cmd);
  bool m(bool &run, int words, char **cmd);
  bool clear(bool &run, int words, char **cmd);
  bool help(bool &run, int words, char **cmd);
  bool trace(bool &run, int words, char **cmd);
  bool disassemble(bool &run, int words, char **cmd);
  bool license(bool &run, int words, char **cmd);
  bool warranty(bool &run, int words, char **cmd);
};

