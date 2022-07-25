/* Honeywell Series 16 emulator
 * Copyright (C) 2020  Adrian Wise
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
#include "tty_file.hh"

TTY_file::TTY_file()
  : fp(0)
  , have_pending_ch(false)
  , insert(INS_NONE)
{
}

TTY_file::~TTY_file()
{
  close();
}

void TTY_file::open(const char *filename, TTY_file_mode mode)
{
  const char *m;

  close();

  switch(mode) {
  case READ_BINARY:  m = "rb"; break;
  case READ_ASCII:   m = "r";  break;
  case WRITE_BINARY: m = "wb"; break;
  case WRITE_ASCII:  m = "w";  break;
  default: return; // is_open() will be false;
  }

  this->mode = mode;

  fp = std::fopen(filename, m);

  have_pending_ch = false;
  insert = INS_NONE;
}

void TTY_file::close()
{
  if (fp) {
    //if ((mode == WRITE_ASCII) && (have_pending_ch) && (ch != '\n')) {
    // }
    std::fclose(fp);
    fp = 0;
  }
}

int TTY_file::simple_getc()
{
  int c;
  
  if (have_pending_ch) {
    c = pending_ch;
    have_pending_ch = false;
  } else {
    c = std::fgetc(fp);
  }

  return c;
}

void TTY_file::simple_ungetc(int c)
{
  pending_ch = c;
  have_pending_ch = true;
}

int TTY_file::getc()
{
  int c;

  if ((!fp) || (mode == WRITE_BINARY) || (mode == WRITE_ASCII)) {
    return EOF;
  }
  
  if (mode == READ_ASCII) {
    switch (insert) {
    default:
    case INS_NONE:
      c = simple_getc();
      if (c=='\n') {
        c = CR;
        insert = INS_XOFF;
      } else if (c == EOF) {
        c = EOM;
        insert = INS_XOFF_EOF;
      } else if ((c >= 'a') && (c <= 'z'))
        c = (c - ('a'-'A')) | 0x80; // fold lower case to upper
      else if (c != NUL ) {
        c |= 0x80;
      }
      break;
      
    case INS_XOFF:       c = XOFF;   insert = INS_RUBOUT;     break;
    case INS_RUBOUT:     c = RUBOUT; insert = INS_LF;         break;
    case INS_XOFF_EOF:   c = XOFF;   insert = INS_RUBOUT_EOF; break;
    case INS_RUBOUT_EOF: c = RUBOUT; insert = INS_EOF;        break;
    case INS_EOF:        c = EOF;    insert = INS_NONE;       break;

    case INS_LF:
      // Take a look at the next character - are we at EOF?
      c = simple_getc();
      if (c == EOF) {
        c = EOM;
        insert = INS_XOFF_EOF;
      } else {
        simple_ungetc(c);
        c = LF;
        insert = INS_NONE;
      }
      break;
    }
  } else {
    c = simple_getc();
  }

  if (c == EOF) {
    close();
  }
  
  return c;
}

void TTY_file::putc(int c)
{
  if ((!fp) || (mode == READ_BINARY) || (mode == READ_ASCII)) {
    return;
  }

  if (mode == WRITE_ASCII) {
    c &= 0x7f; // Drop the forced-parity bit
    if (c == CR7) {
      c = '\n';
    } else if (((c < SP7) && (c != NUL)) ||
               (c == RUB7)) {
      return; // Discard control characters
    }
  }
  
  fputc(c, fp); 
}

void TTY_file::flush()
{
  if (fp) {
    fflush(fp);
  }
}
