/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 1999, 2004, 2005  Adrian Wise
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
#include <string.h>

#include "iodev.hh"
#include "event.hh"
#include "stdtty.hh"

#include "proc.hh"
#include "ptr.hh"

#define SPEED 1000 // charcters per second
#define SMK_MASK (1 << (16-9))

#define EOT_LIMIT 1000

/*
 * The file is opened by the first time (since master clear)
 * that the reader is started.
 *
 * Another filename will not be requested until either a
 * master-clear occurs, or that the reader is started and the
 * tape is at EOT (i.e. at EOF).
 *
 *
 * In the case of an ASCII file all non-zero characters read
 * from the file have the MSB forced to one. A '\n' read from
 * the file is translated into a CR-LF sequence.
 *
 */

enum PTR_REASON
  {
    PTR_REASON_CHARACTER,

    PTR_REASON_NUM
  };

static const char *ptr_reason[PTR_REASON_NUM] __attribute__ ((unused)) =
{
  "Character ready"

};

PTR::PTR(Proc *p, STDTTY *stdtty)
  : IODEV(p)
{
  this->p = p;
  this->stdtty = stdtty;

  fp = NULL;

  master_clear();
}

void PTR::master_clear()
{
  mask = 0;

  if (fp)
    fclose(fp);
  fp = NULL;
  pending_filename = false;

  ascii_file = false;
  insert_lf = false;

  eot = false;
  eot_counter = 0;

  tape_running = false;
  ready = false;
  
  ignore_event = false;
  events_queued = 0;

  data_count = 0;
};

PTR::STATUS PTR::ina(unsigned short instr, signed short &data)
{
  bool r = ready;

  if (ready)
    {
      data = data_buf;
      ready = false;
      p->clear_interrupt(SMK_MASK);
      data_count ++;
      //printf("\nPTR: Read character (%d) %d\n", data_buf, data_count);
    }
  else if (eot)
    {
      eot_counter++;

      if (eot_counter > EOT_LIMIT)
        {
          /*
           * This is a cludge, however...
           *
           * Some programs (well the assembler in particular)
           * don't read to the end of the tape. This is quite
           * normal; most programs should be able to tell they
           * are at the end of valid input without actually
           * running out of tape. However, the assembler needs
           * to get back to the start of the souce code in order
           * to do the second pass. On the real machine, this
           * is simply done by taking the tape out of the
           * reader, rewinding it, and putting it back at the
           * start of the tape. Master clear would cause the
           * PTR to ask for the name of the file again, but then
           * the PC is lost, and the second pass cannot be easily
           * started.
           *
           * In order to deal with this, after an arbitrary
           * number of attempts to read after the EOT, close
           * the file and ask for the name of a new file.
           */
          printf("PTR: EOT\n");
          if (fp)
            fclose(fp);
          fp = NULL;
          open_file();
          start_reader();
        }
    }
  
  return status(r);
}

void PTR::open_file(void)
{
  char buf[256];
  char *cp;

  while (!fp)
    {
      if (pending_filename)
        strcpy(buf, filename);
      else
        stdtty->get_input("PTR: Filename>", buf, 256, 0);
      
      cp = buf;
      if (buf[0]=='&')
        {
          ascii_file = 1;
          cp++;
        }
      
      fp = fopen(cp, "rb");
      if (!fp)
        {
          fprintf(((pending_filename) ? stderr : stdout),
                  "Could not open <%s> for reading\n", cp);
          if (pending_filename)
            exit(1);
        }
      
      if (pending_filename)
        delete filename;
      
      pending_filename = 0;
    }
  
  eot = false;
  eot_counter = 0;
}

void PTR::start_reader(void)
{
  Event::queue(p, (1000000 / SPEED), this, PTR_REASON_CHARACTER );
  events_queued++;
  tape_running = true;
}

PTR::STATUS PTR::ocp(unsigned short instr)
{
  switch(instr & 0700)
    {
    case 0000:

      if ((fp) && (eot))
        {
          fclose(fp);
          fp = NULL;
        }

      if (!fp)
        open_file();
      
      if (!tape_running)
        start_reader();

      break;

    case 0100:

      tape_running = false;
      if (events_queued > 0)
        ignore_event = true;
      /*
       * The first event should be ignored
       * because the tape has been stopped, it is assumed
       * that the tape is stopped early enough to avoid
       * losing characters.
       */
      break;
      
    default:
      fprintf(stderr, "PTR: OCP '%04o\n", instr&0x3ff);
      exit(1);
    }
  return STATUS_READY;
}

PTR::STATUS PTR::sks(unsigned short instr)
{
  bool r = 0;

  switch(instr & 0700)
    {
    case 0000: r = ready; break;
    case 0400: r = !(ready && mask); break;

    default:
      fprintf(stderr, "PTR: SKS '%04o\n", instr&0x3ff);
      exit(1);
    }

  return status(r);
}

PTR::STATUS PTR::ota(unsigned short instr, signed short data)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  exit(1);

  return STATUS_READY;
}

PTR::STATUS PTR::smk(unsigned short mask)
{
  this->mask = mask & SMK_MASK;

  if (ready && this->mask)
    p->set_interrupt(this->mask);
  else
    p->clear_interrupt(SMK_MASK);

  return STATUS_READY;
}

void PTR::event(int reason)
{
  int c;

  switch(reason)
    {
    case REASON_MASTER_CLEAR:
      master_clear();
      break;

    case PTR_REASON_CHARACTER:

      events_queued--;

      if (!ignore_event)
        {
          if (ready)
            fprintf(stderr,
                    "%s %lld Character overrun\n",
                    __PRETTY_FUNCTION__, p->get_half_cycles());

          if (insert_lf)
            {
              c = 0212;
              insert_lf = false;
            }
          else
            {
              c = getc(fp);
              if (ascii_file)
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
          
          if (c == EOF)
            {
              eot = true;
              eot_counter = 0;
            }
          else
            {
              data_buf = c & 0xff;
              ready = true;
              p->set_interrupt(mask);
              Event::queue(p, (1000000 / SPEED), this, PTR_REASON_CHARACTER );
              events_queued++;
            }
        }

      ignore_event = false;
      break;

    default:
      fprintf(stderr, "%s %d\n", __PRETTY_FUNCTION__, reason);
      exit(1);
      break;
    }
}

void PTR::set_filename(char *filename)
{
  if (fp)
    fclose(fp);
  fp = NULL;
  pending_filename = false;

  ascii_file = false;
  insert_lf = false;

  eot = false;
  eot_counter = 0;

  tape_running = false;
  ready = false;

  this->filename = strdup(filename);
  pending_filename = 1;
}
