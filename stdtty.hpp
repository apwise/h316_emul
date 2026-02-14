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

#include <string>

namespace h16 {
  
  struct SavedState;

  class StdTty final {
  private:
    static StdTty *pStdTty;
    class StdTtyDestructor {
    public:
      ~StdTtyDestructor();
    };
    static StdTtyDestructor destructor;

    StdTty();
    ~StdTty();

  public:
    typedef bool callback_t(void *callback_arg, int k);
    
    StdTty(StdTty &) = delete; // Can't copy
    void operator=(const StdTty &) = delete; // Can't assign

    static StdTty &getInstance();

    static void service() {
      if (!pStdTty) {
        (void) getInstance();
      }
      if ((pStdTty) && (pStdTty->tty_input)) {
        pStdTty->service_tty_input();
      }
    }

    void register_callback(void *p, bool (*call_back)(void *p, int k));
  
    bool got_char(char &c);
    void putch(char c);
    void write_char(char c);

    void get_input(const std::string &prompt, std::string &str, bool more);

    void set_canonical(bool c);
    bool get_canonical(){return canonical;};
  
    void catch_sigio() {
      tty_input = true;
    };
    bool get_tty_input(){return tty_input;};
    void service_tty_input();

  private:
    struct SavedState *savedState;
    bool canonical;
    bool tty_input;
    bool pending;
    char pending_char;
    bool escape;
    bool line_ending;
    bool lf_sent;

    // TODO - should this be an interface?
    void *callback_arg;
    callback_t *callback;
  
    char cr_or_lf;

    static void catch_sigio(int sig);
  
    void perror(int res, const std::string &prefix);
              
    bool special_action(char c);
  };
}
#endif // _STDTTY_HPP_
