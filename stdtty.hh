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

class Proc;
struct termios;

class STDTTY
{
public:
  STDTTY();
  void set_proc(Proc *p, bool (*call_special_chars)(Proc *p, int k));

  void set_cannonical();
  void set_non_cannonical();
  bool got_char(char &c);
  void putch(const char &c);

  void get_input(const char *prompt, char *str, unsigned len, bool more);

  static void service_tty_input();
  static bool get_tty_input(){return tty_input;};
  static bool get_cannonical(){return cannonical;};

private:
  bool last_was_cr, last_was_lf;
  char cr_or_lf;

  /* The following are al static.
     This relects the fact that there is only one
     controlling terminal. If the mode changes from
     cannonical to non-cannonical it occurs for
     the common controlling terminal, not the
     individual (if there were more than one;
     which is silly) stdtty objects */

  static Proc *p;
  static bool (*call_special_chars)(Proc *p, int k);

  static struct termios *saved_attributes;
  static void save_input_mode();
  static void reset_input_mode();

  static void catch_sigio(int sig);

  static int fcntl_perror(char *prefix, int fildes, int cmd, int arg);
  static int saved_flags;
  static void install_handler();
  static void uninstall_handler();

  static bool special_action(char c);

  static bool cannonical;
  static char pending_char;
  static bool pending;
  static bool escape;

  static bool tty_input;
};

