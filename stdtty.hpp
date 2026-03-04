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

#ifndef _STDTTY_HPP_
#define _STDTTY_HPP_

class Proc;
struct SavedState;

class StdTty final
{
private:
  static StdTty *pStdTty;
  class StdTtyDestructor {
    ~StdTtyDestructor();
  };
  static StdTtyDestructor destructor;

  StdTty();
  ~StdTty();

public:
  StdTty(StdTty &) = delete; // Can't copy
  void operator=(const StdTty &) = delete; // Can't assign

  static StdTty &getInstance();
  
  void set_proc(Proc *p, bool (*call_special_chars)(Proc *p, int k));

  bool got_char(char &c);
  void putch(const char c);

  void get_input(const char *prompt, char *str, unsigned len, bool more);

  void set_cannonical(bool c);
  bool get_cannonical(){return cannonical;};
  
  void catch_sigio() {
    tty_input = true;
  };
  bool get_tty_input(){return tty_input;};
  void service_tty_input();

private:
  struct SavedState *savedState;
  bool cannonical;
  bool tty_input;
  bool pending;
  char pending_char;
  bool escape;
  bool last_was_cr;
  bool last_was_lf;

  // TODO - replace this with an interface...
  Proc *p;
  bool (*call_special_chars)(Proc *p, int k);
   
  char cr_or_lf;

  static void catch_sigio(int sig);
  
  void perror(int res, const char *prefix);
              
  int saved_flags;

  bool special_action(char c);


};

#endif // _STDTTY_HPP_
