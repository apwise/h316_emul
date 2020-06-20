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
 * Provide a simple wrapper around stdio calls for file access,
 * allowing a file opened for reading in an "ASCII" mode to be
 * translated on-the-fly from the native O/S conventions to have
 * the correct line endings, etc. as if it were prepared for
 * reading on a Teletype (ASR 33 or 35). Forced parity is added,
 * line endings are CR-XOFF-RUBOUT-LF (which stops the ASR's
 * tape reader after each line to avoid inadvertently dropping
 * characters while the input-line is processed). Lower-case
 * letters are folded to upper case.
 * End of file is translated to EOM-XOFF-RUBOUT or, if it is
 * preceded by newline, as CR-XOFF-RUBOUT-EOM-XOFF-RUBOUT.
 *
 * Processing on writing is much simpler. CR is mapped to the
 * native O/S line ending, all other control characters are
 * discarded, and the forced-parity bit is dropped to get
 * 7-bit ASCII characters.
 */

#ifndef _TTY_FILE_HH_
#define _TTY_FILE_HH_

#include <cstdio>

class TTY_file
{
public:
  enum TTY_file_mode
    {
     READ_BINARY,
     READ_ASCII,
     WRITE_BINARY,
     WRITE_ASCII
    };

  TTY_file();
  ~TTY_file();

  void open(const char *filename, TTY_file_mode mode);
  void close();
  bool is_open() {return (fp);}
  
  int getc();
  void putc(int c);
  void flush();

private:
  enum Insert
    {
     INS_NONE,
     INS_XOFF,
     INS_RUBOUT,
     INS_XOFF_EOF,
     INS_RUBOUT_EOF,
     INS_EOF,
     INS_LF
    };

  enum
    {
     NUL    = 0000,

     // Control characters with forced-parity bit
     EOM    = 0203,
     LF     = 0212,
     CR     = 0215,
     XOFF   = 0223,
     RUBOUT = 0377,

     // 7-bit ASCII values
     CR7    = 0015,
     SP7    = 0040,
     RUB7   = 0177
    };

  TTY_file_mode mode;
  std::FILE *fp;
  bool have_pending_ch;
  int pending_ch;
  Insert insert;

  int simple_getc();
  void simple_ungetc(int c);
};

#endif // _TTY_FILE_HH_
