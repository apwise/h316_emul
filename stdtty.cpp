/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005, 2006, 2012, 2024, 2026  Adrian Wise
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

#include "config.h"
#include "stdtty.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <csignal>
#include <cerrno>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif

#include <fcntl.h>
#include <termios.h>

#include <iostream>

#define LF 012
#define CR 015
#define ESCAPE 0x1b

#define PERROR_EXIT 2
#define EOF_EXIT   2

/*
 * This file provides the interface to the terminal.
 *
 * Most of the complication involved here stems from the
 * fact that one terminal is used for two purposes; to be
 * the "teletype" connected to the computer, and to provide
 * a human interface for various aspects of the emulation
 * (such as asking what filename to use as input to the
 * paper tape reader.)
 *
 * To this end the terminal switches between two modes;
 * cannonical and non-cannonical.
 * When in an "emulation-control" situation (asking about the
 * filename for the paper taper reader, or input to the
 * "monitor", the cannonical input mode is used and the
 * processing behaves like a normal UNIX program. Line editing
 * works as one would expect and characters like ^C and ^Z
 * have their normal function.
 * When emulating a teletype the non-cannonical mode is used
 * and the characters are read one-by-one as and when they are
 * ready. The normal UNIX processing of special signal-generating
 * characters is not performed. Instead this is handled
 * explicitly by code in this file. This allows for a range of
 * additional "asynchronous" events to be generated from the
 * keyboard, including generating start button interrupts and
 * breaking into the monitor while the machine is running.
 * Since these events may happen when the program running on
 * the Honeywell has no need for teletype input there is a need
 * to check whether characters have been typed at all times
 * and in fact a signal (interrupt) is installed for this
 * purpose.
 *
 */
#include <mutex>

/*
 * Saved state is stored via an opaque pointer to avoid having
 * headers defining these structures included everywhere.
 */
struct SavedState {
  struct termios t;
  struct sigaction sa;
  int flags;
};

StdTty *StdTty::pStdTty{nullptr};
StdTty::StdTtyDestructor StdTty::destructor;

StdTty &StdTty::getInstance()
{
  static std::mutex mutex;
  if (pStdTty == nullptr) {
    std::lock_guard<std::mutex> lock(mutex);
    if (pStdTty == nullptr) {
      pStdTty = new StdTty();
    }
  }
  return *pStdTty;
}

StdTty::StdTtyDestructor::~StdTtyDestructor() {
  if (StdTty::pStdTty) {
    delete StdTty::pStdTty;
    StdTty::pStdTty = nullptr;
  }
};

void StdTty::catch_sigio(int sig) {
  pStdTty->tty_input = true;
};


StdTty::StdTty()
  : savedState(nullptr)
  , cannonical(true)
  , tty_input(false)
  , pending(false)
  , escape(false)
  , last_was_cr(false)
  , last_was_lf(false)
  , callback_arg(nullptr)
  , callback(nullptr)
{
  savedState = new SavedState;
  
  if (isatty(STDIN_FILENO)) {
    // Install a signal handler to catch SIGIO
    struct sigaction sa {};
    sa.sa_handler = catch_sigio;

    tty_input = false;
    
    int res = sigaction(SIGIO, &sa, &savedState->sa);
    perror(res, "StdTty: sigaction()");

    // Save current stdin flags
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    perror(flags, "StdTty: sigaction()");
    
    savedState->flags = flags; // Save to restore at exit
    
    flags |= O_NONBLOCK;
    
    res = fcntl(STDIN_FILENO, F_SETFL, flags);
    perror(res, "StdTty: fcntl(F_SETFL)");

    // Save the current terminal attributes
    res = tcgetattr(STDIN_FILENO, &savedState->t);
    perror(res, "StdTty: tcgetattr()");
  }
  
  set_cannonical(false);
}

StdTty::~StdTty() {
  set_cannonical(true); // Restore terminal characteristics
  if (isatty(STDIN_FILENO)) {
    
    // Restore original stdin flags 
    int res = fcntl(STDIN_FILENO, F_SETFL, saved_flags);
    perror(res, "~StdTty: fcntl(F_SETFL)");

    // Restore the original action on SIGIO
    res = sigaction(SIGIO, &savedState->sa, nullptr);
    perror(res, "~StdTty: sigaction()");
  }
  if (savedState) {
    delete savedState;
  }
}

void StdTty::register_callback(void *callback_arg, callback_t *callback) {
  this->callback_arg = callback_arg;
  this->callback = callback;
}

void StdTty::set_cannonical(bool c)
{
  int res;
  tty_input = false;
  pending = false;

  if ((c != cannonical) && isatty(STDIN_FILENO)) {

    if (c) {
      // non-cannonical to cannonical
      
      res = tcsetattr(STDIN_FILENO, TCSAFLUSH, &savedState->t);
      perror(res, "StdTty::set_cannonical(true): tcsetattr()");
    } else {
      // cannonical to non-cannonical

      struct termios tattr;
      res = tcgetattr (STDIN_FILENO, &tattr);
      perror(res, "StdTty::set_cannonical(false): tcgetattr()");
      
      tattr.c_lflag &= ~(ICANON|ECHO);
      tattr.c_iflag &= ~(IGNCR | ICRNL | INLCR);
      tattr.c_oflag |= ONLCR;
      tattr.c_cc[VMIN] = 0;
      tattr.c_cc[VTIME] = 0;
      
      res = tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
      perror(res, "StdTty::set_cannonical(false): tcsetattr()");
    }
    
    cannonical = c;
  }
}

/*
 * got_char() and putch() are used by the TTY to get
 * and send individual characters from keyboard and
 * to printer.
 */
bool StdTty::got_char(char &c)
{
  bool r;
  
  if (pending) {
    r = 1;
    pending = 0;
    c = pending_char;
  } else {
    r = (read(STDIN_FILENO, &c, 1) == 1);
    
    if ((! isatty(STDIN_FILENO)) && (c == '\n')) {
      // If input redirected from a file (or pipe) then translate
      // a line-end to a carriage-return, as if typed on keyboard.
      c = '\r';
    }
    
    if (r)
      r = !special_action(c);
  }
  
  return r;
}

/* putch()
 *
 * The point of this routine is that code written for the
 * H316 assumes that both CR and LF needs to be sent to
 * the ASR. This is actually true of the terminal emulators
 * on a Unix system (e.g. xterm) but Unix replaces the
 * newline character (\n) by CR-LF in the low-level
 * terminal interface.
 * It is possible to turn this behaviour off (and an earlier
 * version of this emulator did that) however this means
 * that messages from the emulator (generated with printf() or
 * cout) don't behave properly because the CR is missing
 * (\n is LF).
 * So instead, this routine replaces CR-LF or LF-CR sequences
 * by the newline (\n) character. Isolated CR or LF characters
 * are passed unchanged.
 *
 * TODO: Look at the cr-xoff-rubout-lf sequence
 */
void StdTty::putch(const char c)
{
  bool send = true;
  char ch = c;
  bool send_cr_or_lf = false;

  if (last_was_cr) {
    if ((ch & 0x7f) == LF)
      ch = '\n'; // CR-LF sequence
    else
      send_cr_or_lf = true;
    
    last_was_cr = false;
  } else if (last_was_lf) {
    if ((ch & 0x7f) == CR)
      ch = '\n'; // LF-CR sequence
    else
      send_cr_or_lf = true;
    
    last_was_lf = false;
  } else if ((ch & 0x7f) == CR) {
    last_was_cr = true;
    send = false;
    cr_or_lf = ch;
  } else if ((ch & 0x7f) == LF) {
    last_was_lf = true;
    send = false;
    cr_or_lf = ch;
  }
  
  if (send_cr_or_lf || send) {
    ssize_t n;
    do {
      n = write(STDOUT_FILENO, ((send) ? &ch : &cr_or_lf), 1);
      if (n == -1) {
        if ((errno != EAGAIN) && (errno != EINTR))
          abort();
      }
    } while (n == 0);
  }
}

/*
 * get user input
 */

#ifndef HAVE_LIBREADLINE

//#define NRL_BUFLEN 256

static char *no_readline(const char *prompt)
{
  char *ptr = NULL;
  size_t n = 0;
  ssize_t r;
  
  fputs(prompt, stdout);
  fflush(stdout);

  r = getline(&ptr, &n, stdin);

  if (r < 0) {
    if (ptr) free(ptr);
    ptr = 0;
  }

  return ptr;
}
#endif

void StdTty::get_input(const char *prompt, char *str, unsigned len, bool more)
{
  unsigned n;
  char *ptr;

  set_cannonical(true);

#ifdef HAVE_LIBREADLINE
  ptr = readline(prompt);
#else
  ptr = no_readline(prompt);
#endif

  if (ptr) {
    n = strlen(ptr);
    // Nibble any spaces off the end
    // (They're likely due to filename completion)
    while ((n>1) && (ptr[n-1] == ' '))
      n--;
    
    if (n>(len - 1))
      n = len - 1;

    strncpy(str, ptr, n);
    
#ifdef HAVE_LIBHISTORY
    if (n > 0)
      add_history(ptr); // Add including the trailing space
#endif
  } else {
    n = 0;
    
    if (! isatty(STDIN_FILENO)) {
      // Input is not from a terminal (file or pipe?)
      // and something failed - probably EOF.
      // Returning as normal will be interpreted as a blank
      // line, but this probably isn't what's expected.
      // Exit instead.
      fprintf(stderr, "EOF\n");
      exit(EOF_EXIT);
    }
  }

  str[n] = '\0';
#ifdef HAVE_LIBREADLINE
  free(ptr);
#endif

  if (!more)
    set_cannonical(false);
}

/*
 * Terminal signal handler stuff...
 */

void StdTty::perror(int res, const char *prefix) {
  if (res == -1) {
    ::perror(prefix);
    exit(PERROR_EXIT);
  }
}

void StdTty::service_tty_input()
{
  bool r;

  tty_input = 0;

  if ((!cannonical) && isatty(0)) {
      while ((r = read (STDIN_FILENO, &pending_char, 1)) == 1) {
        /*
         * if pending is already set then at this point
         * we drop data. Unfortunate, but we have to move
         * on else the special processing will not happen for
         * the queued characters.
         */
        if (pending) {
          putc(0x07, stdout);
          fflush(stdout);
        }
        if (r)
          pending = !special_action(pending_char);
      }
  }
}

bool StdTty::special_action(char c)
{
  // cout << __PRETTY_FUNCTION__ << "\n";
  bool r = false;
  int k = ((int) c) & 0xff;

  /* In xterm ALT-<c> causes the MSB of the byte received to
     be set. However, not all terminals do this. In particular,
     the gnome-terminal sends ESC folowed by <c>. */

  if (k == ESCAPE) {
    escape = true;
    return true; // Don't want ESC to become pending
  }

  if (escape) {
    k |= 0x80; // try putting on the top bit
  }

  if (( k & 0x80 ) && (callback)) {
    r = (*callback)(callback_arg, k);
  }
  
  escape = false;
  
  return r;
}


