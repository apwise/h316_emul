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
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

#include "stdtty.hh"
#include "asr.hh"

#define WRU  005
#define SO   016 // ^N shift into black
#define SI   017 // ^O shift into red
#define XON  021
#define TAPE 022
#define XOFF 023
#define RUBOUT 0377

ASR::ASR(STDTTY *stdtty)
{
  int i;
  this->stdtty = stdtty;

  stdtty->set_non_cannonical();

  for (i=0; i<2; i++)
    {
      pending_filename[i] = false;
      fp[i] = NULL;
      running[i] = false;
      ascii_file[i] = false;
      silent_file[i] = false;
      char_count[i] = 0;
    }

  clear_ptr_flags();
  clear_ptp_flags();
}

void ASR::clear_ptp_flags()
{
  tape_char_received = false;
  xoff_received = false;
}

void ASR::clear_ptr_flags()
{
  insert_lf = false;
  stop_after_next = false;
  xoff_read = false;
}

bool ASR::get_asrch(char &c)
{
  char k;
  bool r = false;
  int ch;

  //cout << __PRETTY_FUNCTION__ << "\n";

  if (running[ASR_PTR])
    {
      stdtty->service_tty_input();

      if (insert_lf)
        {
          c = 0212;
          insert_lf = false;
          r = true;
        }
      else
        {
          ch = getc(fp[ASR_PTR]);
          char_count[ASR_PTR]++;
          if (ch == EOF)
            {
              fclose(fp[ASR_PTR]);
              fp[ASR_PTR] = NULL;
              printf("\nASRPTR: EOF\n");
              running[ASR_PTR] = false;
            }
          else
            {
              c = ch;
              r = true;
              
              if (ascii_file[ASR_PTR])
                {
                  if (c=='\n')
                    {
                      c = 0215;
                      insert_lf = true;
                    }
                  else if (c != 0 )
                    c |= 0x80;
                }
            }
        }

      if (stop_after_next)
        {
          running[ASR_PTR] = false;
          //Printf("ASRPTR: Stopped\n");
        }
      else if (xoff_read)
        {
          //Printf("ASRPTR: About to stop\n");
          xoff_read = false;
          if ((c&0xff) == RUBOUT)
            {
              running[ASR_PTR] = false;
              //Printf("ASRPTR: Stopped\n");
            }
          else
            stop_after_next = true;
        }
      else if ((c&0x7f) == XOFF)
        {
          xoff_read = true;
          //Printf("ASRPTR: XOFF read\n");
        }
    }
  else
    {
      if (stdtty->got_char(k))
        {
          k &= 0x7f;
          if ( (k == 012) || (k == 015) || (k == 007) )
            r = 1;
          else if (k >= 040)
            {
              if ((k >= 0140) && (k < 0177))
                k -= 040;
              r = 1;
            }
        }
      
      if (r)
        c = k | ((k != 0) ? 0x80 : 0);
    }
  
  if ((r) &&
      ((!running[ASR_PTR]) || (!silent_file[ASR_PTR])))
    echo_asrch(c, false);

  return r;
}

void ASR::put_asrch(char c)
{
  echo_asrch(c, true);
}

void ASR::echo_asrch(char c, bool from_serial)
{
  int k;

  if (from_serial)
    {
      if (tape_char_received)
        {
          tape_char_received = false;
          //open_punch_file();
          if ((c & 0xff) == RUBOUT)
            return; // RUBOUT is not punched
        }
      else if ( (c&0x7f) == TAPE )
        tape_char_received = true;


      if ((c & 0x7f) == XON)
        {
          //Printf("ASR: XON\n");
          open_reader_file();
        }
    }

  if (running[ASR_PTP])
    {
      if (ascii_file[ASR_PTP])
        {
          k = c & 0x7f;
          if ((k != 015) && // ignore Carriage Return
              (k != 0))     // ignore null
            {
              if (k == 012) // LF is newline
                k = '\n';
              putc(k, fp[ASR_PTP]);
            }
        }
      else
        putc(c, fp[ASR_PTP]);

      if (xoff_received)
        {
          //printf("XOFF\n");
          xoff_received = false;
          running[ASR_PTP] = false;
          fflush(fp[ASR_PTP]);
        }
      else if ( (c & 0x7f) == XOFF )
        xoff_received = true;
    }

  if ((!running[ASR_PTP]) || (!silent_file[ASR_PTP]))
    {
      k = c & 0x7f;

      //if (((c & 0xff) > 0240) && (!from_serial) && running[ASR_PTR])
      if ((c & 0x80) && (!from_serial) && running[ASR_PTR])
        return; // Don't echo reader data with MSB set

      if ((k == 007) || (k == 012) || (k == 015) ||
          ((k >= 040) && (k < 0174)) || (k == 0177))
        stdtty->putch(k);

    }
}

void ASR::set_filename(char *filename, bool asr_ptp)
{
  int f=0;
  ascii_file[asr_ptp] = silent_file[asr_ptp] = false;

  if (filename[f] == '&')
    {
      ascii_file[asr_ptp] = true;
      f++;
    }
  if (filename[f] == '@')
    {
      silent_file[asr_ptp] = true;
      f++;
    }
  
  this->filename[asr_ptp] = strdup(&filename[f]);
  pending_filename[asr_ptp] = true;
}

void ASR::asr_ptp_on(char *filename)
{
  //printf ("ASR::asr_ptp_on(%s)\n", (filename) ? filename : "NULL");

  if (filename)
    set_filename(filename, ASR_PTP);

  open_punch_file();
}

#define BUFLEN 256

void ASR::get_filename(bool asr_ptp)
{
  char temp[BUFLEN];
  stdtty->get_input( const_cast<char *>((asr_ptp) ? "ASRPTP: " : "ASRPTR: "),
                     temp, BUFLEN, false);
  set_filename(temp, asr_ptp);
}

void ASR::close_file(bool asr_ptp)
{
  if (fp[asr_ptp])
    {
      fclose(fp[asr_ptp]);
      fp[asr_ptp] = NULL;
      running[asr_ptp] = false;
    }
}

void ASR::open_reader_file()
{
  while (!fp[ASR_PTR])
    {
      clear_ptr_flags();
      if (!pending_filename[ASR_PTR])
        get_filename(ASR_PTR);
      fp[ASR_PTR] = fopen(filename[ASR_PTR],
                          (ascii_file[ASR_PTR] ? "r" : "rb"));
      pending_filename[ASR_PTR] = false;
      if (!fp[ASR_PTR])
        fprintf(stderr, "Failed to open <%s> for reading\r\n",
                filename[ASR_PTR]);

      insert_lf = false;
      char_count[ASR_PTR] = 0;
    }

  running[ASR_PTR] = true;
  //Printf("ASRPTR: Started\n");
}

void ASR::open_punch_file()
{
  while (!fp[ASR_PTP])
    {
      if (!pending_filename[ASR_PTP])
        get_filename(ASR_PTP);
      fp[ASR_PTP] = fopen(filename[ASR_PTP],
                          (ascii_file[ASR_PTP] ? "w" : "wb"));
      pending_filename[ASR_PTP] = false;
      if (!fp[ASR_PTP])
        fprintf(stderr, "Failed to open <%s> for writing\n",
                filename[ASR_PTP]);
    }
  
  running[ASR_PTP] = true;
}

bool ASR::special(char c)
{
  bool r=false;

  switch(c & 0x7f)
    {
    case 'h': /* close files */
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
      printf("\nASRPTR:\n");
      printf("running = %d\n", ((int)running[ASR_PTR]) );
      printf("char_count = %d\n", ((int) char_count[ASR_PTR]) );

      r = 1;
      break;
    }
  return r;
}
