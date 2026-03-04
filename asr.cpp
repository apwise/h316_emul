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

#include "asr.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <iostream>

#include "stdtty.hpp"

#define WRU  005
#define BS   010
#define SO   016 // ^N shift into black
#define SI   017 // ^O shift into red
#define XON  021
#define TAPE 022
#define XOFF 023
#define RUBOUT 0377

Asr::Asr(IoToPIntf &p)
  : IoDev(p)
  , stdTty(StdTty::getInstance())
{
  int i;

  stdTty.set_cannonical(false);

  for (i=0; i<2; i++) {
    pending_filename[i] = false;
    running[i] = false;
    ascii_file[i] = false;
    silent_file[i] = false;
    char_count[i] = 0;
  }
  
  clear_ptr_flags();
  clear_ptp_flags();
}

const char *Asr::name() {
  return "ASR";
}

void Asr::clear_ptp_flags()
{
  tape_char_received = false;
  xoff_received = false;
}

void Asr::clear_ptr_flags()
{
  stop_after_next = false;
  xoff_read = false;
}

bool Asr::get_asrch(char &c, bool local_echo)
{
  char k;
  bool r = false;

  //std::cout << __PRETTY_FUNCTION__ << std::endl;

  if (running[ASR_PTR]) {
    stdTty.service_tty_input();

    c = tty_file[ASR_PTR].getc();
    r = true;

    if (stop_after_next) {
      running[ASR_PTR] = false;
      stop_after_next = false;
    } else if (xoff_read) {
      xoff_read = false;
      if ((c & 0xff) == RUBOUT) {
        running[ASR_PTR] = false;
      } else {
        stop_after_next = true;
      }
      
    } else if ((c & 0x7f) == XOFF) {
      xoff_read = true;
    }
  } else {
    if (stdTty.got_char(k)) {
      k &= 0x7f;
      if ( (k == 012) || (k == 015) || (k == 007) )
        r = true;
      else if (k >= 040) {
        if ((k >= 0140) && (k < 0177))
          k -= 040;
        r = true;
      }
    }
    
    if (r)
      c = k | ((k != 0) ? 0x80 : 0);
  }
  
  if ((r) && (local_echo) &&
      ((!running[ASR_PTR]) || (!silent_file[ASR_PTR])))
    echo_asrch(c, false);
  
  return r;
}

void Asr::put_asrch(char c)
{
  echo_asrch(c, true);
}

void Asr::echo_asrch(char c, bool from_serial)
{
  int k;

  if (from_serial) {
    if (tape_char_received) {
      tape_char_received = false;
      //open_punch_file();
      if ((c & 0xff) == RUBOUT)
        return; // RUBOUT is not punched
    } else if ( (c & 0x7f) == TAPE )
      tape_char_received = true;

    if ((c & 0x7f) == XON) {
      if (tty_file[ASR_PTR].is_open()) {
        running[ASR_PTR] = true;
      }
    }
  }
  
  if (running[ASR_PTP]) {

    tty_file[ASR_PTP].putc(c);

    if (xoff_received) {
      xoff_received = false;
      running[ASR_PTP] = false;
      tty_file[ASR_PTP].flush();
    } else if ( (c & 0x7f) == XOFF ) {
      xoff_received = true;
    }
  }
  
  if ((!running[ASR_PTP]) || (!silent_file[ASR_PTP])) {
    k = c & 0x7f;

    if ((c & 0x80) && (!from_serial) && running[ASR_PTR])
      return; // Don't echo reader data with MSB set
    
    if ((k == 007) || (k == 012) || (k == 015) ||
        ((k >= 040) && (k < 0174)))
      stdTty.putch(k);  
  }
}

void Asr::set_filename(const std::string &filename, unsigned subdevice) {
  if (subdevice > 1) {
    p.anomaly(IoToPIntf::Level::FATAL, "Unexpected subdevice");
  }
  
  int f=0;
  ascii_file[subdevice] = silent_file[subdevice] = false;

  if (filename[f] == '&') {
    ascii_file[subdevice] = true;
    f++;
  }
  if (filename[f] == '@') {
    silent_file[subdevice] = true;
    f++;
  }
  
  this->filename[subdevice] = strdup(&filename[f]);
  pending_filename[subdevice] = true;
}

void Asr::ptp_on() {
  open_punch_file();
}

void Asr::ptr_on() {
  open_reader_file();
}

#define BUFLEN 256

void Asr::get_filename(bool asr_ptp)
{
  char temp[BUFLEN];
  stdTty.get_input( const_cast<char *>((asr_ptp) ? "AsrPTP: " : "AsrPTR: "),
                    temp, BUFLEN, false);
  set_filename(temp, asr_ptp);
}

void Asr::close_file(bool asr_ptp)
{
  tty_file[asr_ptp].close();
  running[asr_ptp] = false;
}

void Asr::open_reader_file()
{
  while (!tty_file[ASR_PTR].is_open()) {
    if (!pending_filename[ASR_PTR])
      get_filename(ASR_PTR);
    tty_file[ASR_PTR].open(filename[ASR_PTR],
                           (ascii_file[ASR_PTR] ? TTY_file::READ_ASCII :
                            TTY_file::READ_BINARY));
    pending_filename[ASR_PTR] = false;
    if (!tty_file[ASR_PTR].is_open())
      fprintf(stderr, "Failed to open <%s> for reading\r\n",
              filename[ASR_PTR]);
    
    char_count[ASR_PTR] = 0;
  }
  clear_ptr_flags();
  running[ASR_PTR] = true;
}

void Asr::open_punch_file()
{
  while (!tty_file[ASR_PTP].is_open()) {
    if (!pending_filename[ASR_PTP])
      get_filename(ASR_PTP);
    tty_file[ASR_PTP].open(filename[ASR_PTP],
                           (ascii_file[ASR_PTP] ? TTY_file::WRITE_ASCII :
                            TTY_file::WRITE_BINARY));
    pending_filename[ASR_PTP] = false;
    if (!tty_file[ASR_PTP].is_open())
      fprintf(stderr, "Failed to open <%s> for writing\n",
              filename[ASR_PTP]);
  }
  
  running[ASR_PTP] = true;
}

bool Asr::special(char c)
{
  bool r=false;

  switch(c & 0x7f) {
  case 'h': /* help */
    printf("\n");
    printf("ALT-h Print this help\n");
    printf("ALT-p Select file for punch output\n");
    printf("ALT-u Start punch\n");
    printf("ALT-r Select file for reader input\n");
    printf("ALT-t Start reader\n");
    printf("      \'&\' before filename specifies ASCII file\n");
    printf("      \'@\' before filename specifies silent\n");
    printf("ALT-c Close files\n");
    printf("ALT-q Quit\n");
    r = 1;
    break;
    
  case 'c': /* close files */
    close_file(ASR_PTP);
    close_file(ASR_PTR);
    clear_ptr_flags();
    clear_ptp_flags();
    r = 1;
    break;
    
  case 'q': /* quit */
    close_file(ASR_PTP);
    close_file(ASR_PTR);
    r = 1;
    exit(0);
    break;
    
  case 'p': /* punch */
    close_file(ASR_PTP);
    clear_ptp_flags();
    get_filename(ASR_PTP);
    r = 1;
    break;
    
  case 'r': /* reader */
    close_file(ASR_PTR);
    clear_ptr_flags();
    get_filename(ASR_PTR);
    r = 1;
    break;
    
  case 't': /* start reader */
    open_reader_file();
    r = 1;
    break;

  case 'u': /* start punch */
    open_punch_file();
    r = 1;
    break;
    
  case 'z': /* Print some status */
    printf("\nAsrPTR:\n");
    printf("running = %d\n", ((int)running[ASR_PTR]) );
    printf("char_count = %d\n", ((int) char_count[ASR_PTR]) );
    
    r = 1;
    break;
  }
  return r;
}
