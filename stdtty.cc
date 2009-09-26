/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005, 2006  Adrian Wise
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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <fcntl.h>
#include <termios.h>

#include <signal.h>

#include <iostream>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif

#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif

#include "stdtty.hh"

#define LF 012
#define CR 015
#define ESCAPE 0x1b

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

/* Use this variable to remember original terminal attributes. */
struct termios *STDTTY::saved_attributes = NULL;
bool STDTTY::cannonical;

Proc *STDTTY::p;
bool (*STDTTY::call_special_chars)(Proc *p, int k);

STDTTY::STDTTY()
{
  p = NULL;
  if (!saved_attributes)
    {
      STDTTY::save_input_mode();
      STDTTY::install_handler();
    }
  
  tty_input = 0;
  pending = 0;
  escape = 0;

  last_was_cr = false;
  last_was_lf = false;

  cannonical = 1;
  set_non_cannonical();

}

void STDTTY::set_proc(Proc *p, bool (*call_special_chars)(Proc *p, int k))
{
  this->p = p;
  this->call_special_chars = call_special_chars;
}

/*
 * The input mode is saved in order that it can be reinstated
 * after leaving the program. Otherwise they remain in force
 * and this is very likely to confuse the shell!
 */
void STDTTY::save_input_mode()
{
  saved_attributes = new termios;
 
  /* Make sure stdin is a terminal. */
  if (!isatty (STDIN_FILENO))
    {
      fprintf (stderr, "Not a terminal.\n");
      exit (1);
    }

  /* Save the terminal attributes so we can restore them later. */
  tcgetattr (STDIN_FILENO, saved_attributes);
  atexit (STDTTY::reset_input_mode);
}

void STDTTY::reset_input_mode (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, saved_attributes);
}

/*
 * set the non-cannonical mode.
 */

void STDTTY::set_non_cannonical()
{
  struct termios tattr;

  //cout << __PRETTY_FUNCTION__ << "\n";

  if (cannonical)
    {
      tty_input = 0;
      pending = 0;

      tcgetattr (STDIN_FILENO, &tattr);
      tattr.c_lflag &= ~(ICANON|ECHO); //Clear ICANON and ECHO.
      tattr.c_iflag |= INLCR;          //Allow CR characters
      tattr.c_iflag &= ~IGNCR;
      tattr.c_iflag &= ~ICRNL;
      tattr.c_iflag &= ~INLCR;

      //tattr.c_oflag &= ~ONLCR;
      tattr.c_oflag |= ONLCR;
      tattr.c_cc[VMIN] = 0;
      tattr.c_cc[VTIME] = 0;
      tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
    
      cannonical = 0;
    }
}

/*
 * set the cannonical mode.
 */

void STDTTY::set_cannonical()
{
  struct termios tattr;

  //cout << __PRETTY_FUNCTION__ << "\n";

  if (!cannonical)
    {
      tty_input = 0;
      pending = 0;

      /* Set the funny terminal modes. */
      tcgetattr (STDIN_FILENO, &tattr);
      tattr.c_lflag |= ICANON|ECHO; /* Set ICANON and ECHO. */
      tattr.c_iflag &= ~INLCR;      // Disallow CR charcters
      tattr.c_iflag |= ICRNL;
      tattr.c_oflag |= ONLCR;
      tattr.c_cc[VERASE] = '\010';  // Make backspace delete
      tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);

      cannonical = 1;
    }

}

/*
 * got_char() and putch() are used by the TTY to get
 * and send individual characters from keyboard and
 * to printer.
 */
bool STDTTY::got_char(char &c)
{
  bool r;

  if (pending)
    {
      r = 1;
      pending = 0;
      c = pending_char;
    }
  else
    {
      r = (read(STDIN_FILENO, &c, 1) == 1);
      
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
 * cout) don't behave properly beacause the CR is missing
 * (/n is LF).
 * So instead this routine replaces CR-LF or LF-CR sequences
 * by the newline (\n) character. Isolated CR of LF characters
 * are passed unchanged.
 */
void STDTTY::putch(const char &c)
{
  bool send = true;
  char ch = c;
  bool send_cr_or_lf = false;
  int n;

  if (last_was_cr)
    {
      if ((ch & 0x7f) == LF)
        ch = '\n'; // CR-LF sequence
      else
        send_cr_or_lf = true;

      last_was_cr = false;
    }
  else if (last_was_lf)
    {
      if ((ch & 0x7f) == CR)
        ch = '\n'; // LF-CR sequence
      else
        send_cr_or_lf = true;

      last_was_lf = false;
    }
  else if ((ch & 0x7f) == CR)
    {
      last_was_cr = true;
      send = false;
      cr_or_lf = ch;
    }
  else if ((ch & 0x7f) == LF)
    {
      last_was_lf = true;
      send = false;
      cr_or_lf = ch;
    }

  if (send_cr_or_lf)
    n = write(STDOUT_FILENO, &cr_or_lf, 1);

  if (send)
    n = write(STDOUT_FILENO, &ch, 1);
}

/*
 * get user input
 */

#ifndef HAVE_LIBREADLINE

#define NRL_BUFLEN 256

static char *no_readline(const char *prompt)
{
  static char buf[NRL_BUFLEN];
  int i, j, n;
  bool reading;

  fputs(prompt, stdout);
  fflush(stdout);

  reading = true;
  i=0;

  while (reading)
    {
      n = read(STDIN_FILENO, &buf[i], NRL_BUFLEN-1);
      if (n == -1)
        {
          if (errno != EAGAIN)
            {
              std::cerr << "read() returned -1: errno => "
                        << strerror(errno) << " (" << errno << ")" << std::endl;
              exit(1);
            }
        }
      else
        {
          for (j=i; j<(i+n); j++)
            if (buf[j] == '\n')
              {
                buf[j] = '\0';
                reading = false;
              }
          i+=n;
        }
    };
  return buf;
}
#endif

void STDTTY::get_input(const char *prompt, char *str, int len, bool more)
{
  int n;
  char *ptr;

  set_cannonical();

#ifdef HAVE_LIBREADLINE
  ptr = readline(prompt);
#else
  ptr = no_readline(prompt);
#endif

  if (ptr)
    {
      n = strlen(ptr);
      // Nibble any spaces off the end
      // (They're likely due to filename completion)
      while ((n>1) && (ptr[n-1] == ' '))
        n--;
            
      if (n>(len - 1))
        n = len - 1;
      int i;
      for (i=0; i<n; i++)
        str[i]=ptr[i];

#ifdef HAVE_LIBREADLINE
      if (str[0])
        add_history(ptr);//Add including the trailing space
#endif
    }
  else
    n = 0;

  str[n] = '\0';
#ifdef HAVE_LIBREADLINE
  free(ptr);
#endif

  if (!more)
    set_non_cannonical();
}

/*
 * Terminal signal handler stuff...
 */

int STDTTY::fcntl_perror(char *prefix, int fildes, int cmd, int arg)
{
  int res;

  res = fcntl(fildes, cmd, arg);
  if (res == -1)
    {
      perror(prefix);
      exit(1);
    }
  return res;
}

int STDTTY::saved_flags;

void STDTTY::install_handler()
{
  int flags;

  signal (SIGIO, catch_sigio);

  flags = fcntl_perror(const_cast<char *>("STDTTY::install_handler() F_GETFL"),
                       STDIN_FILENO, F_GETFL, 0);
  
  saved_flags = flags; // Save to restore at exit

  flags |= O_NONBLOCK;

  (void) fcntl_perror(const_cast<char *>("STDTTY::install_handler() F_SETFL"),
                      STDIN_FILENO, F_SETFL, flags);

  // Don't think this SETOWN is needed...
  // flags = fcntl (STDIN_FILENO, F_SETOWN, getpid());

  tty_input = 0;
  atexit(STDTTY::uninstall_handler);
}

void STDTTY::uninstall_handler()
{
  (void) fcntl_perror(const_cast<char *>("STDTTY::uninstall_handler() F_SETFL"),
                      STDIN_FILENO, F_SETFL, saved_flags);
}

char STDTTY::pending_char;
bool STDTTY::pending;
bool STDTTY::tty_input;
bool STDTTY::escape;

void STDTTY::catch_sigio(int sig)
{
  tty_input = 1;
  signal (sig, catch_sigio);
}

void STDTTY::service_tty_input()
{
  bool r;

  tty_input = 0;

  if (!cannonical)
    {
      while (r = (read (STDIN_FILENO, &pending_char, 1) == 1))
        {
          /*
           * if pending is already set then at this point
           * we drop data. Unfortunate, but we have to move
           * on else the special processing will not happen for
           * the queued characters.
           */
          if (pending)
            {
              putc(0x07, stdout);
              fflush(stdout);
            }
          if (r)
            pending = !special_action(pending_char);
        }
    }
}

bool STDTTY::special_action(char c)
{
  // cout << __PRETTY_FUNCTION__ << "\n";
  bool r=0;
  int k = ((int) c) & 0xff;

  /* In xterm ALT-<c> causes the MSB of the byte received to
     be set. However, not all terminals do this. In particular,
     the gnome-terminal sends ESC folowed by <c>. */
  
  if (k == ESCAPE)
    {
      escape = true;
      return true; // Don't want ESC to become pending
    }

  if (escape)
    k |= 0x80; // try putting on the top bit

  if ( k & 0x80 )
    r = (*call_special_chars)(p, k);

  escape = false;

  return r;
}
