/* Honeywell Series 16 emulator
 * Copyright (C) 2004, 2005  Adrian Wise
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
#include "stdtty.hh"

#include "proc.hh"
#include "lpt.hh"

#include <iostream>

#define SMK_MASK (1 << (16-14))

enum LPT_REASON
  {
    LPT_REASON_DUMMY,
    LPT_REASON_NUM
  };

static const char *ptp_reason[LPT_REASON_NUM] __attribute__ ((unused)) =
{
  "Dummy"
};

LPT::LPT(Proc *p, STDTTY *stdtty)
  : IODEV(p)
{
  this->p = p;
  this->stdtty = stdtty;
  
  fp = NULL;
  
  line[120] = '\0';

  master_clear();
}

void LPT::master_clear()
{
  mask = 0;
  
  if (fp)
    fclose(fp);
  fp = NULL;
  pending_filename = 0;
  line_number = 0;
  pending_nl = false;
}

LPT::STATUS LPT::ina(unsigned short instr, signed short &data)
{
  fprintf(stderr, "%s Input from line printer\n", __PRETTY_FUNCTION__);
  exit(1);
  return STATUS_READY;
}

void LPT::open_file()
{
  char buf[256];
  char *cp;

  while (!fp)
    {
      if (pending_filename)
        strcpy(buf, filename);
      else
        stdtty->get_input("LPT: Filename>", buf, 256, 0);
      
      cp = buf;
      if (buf[0]=='&')
        cp++;
      
      fp = fopen(cp, "w");
      if (!fp)
        {
          fprintf(((pending_filename) ? stderr : stdout),
                  "Could not open <%s> for writing\n", cp);
          if (pending_filename)
            exit(1);
        }
      line_number = 0;

      if (pending_filename)
        delete filename;
      
      pending_filename = 0;
    }
}

LPT::STATUS LPT::ota(unsigned short instr, signed short data)
{
  bool r = false;
  int d;

  switch(instr & 01700)
    {
    case 00000:

      open_file();

      if (scan_counter < 119)
        {
          d = (data >> 8) & 0x7f;
          if (d == 0) d = ' ';
          line[scan_counter] = d;
          d = data & 0x7f;
          if (d == 0) d = ' ';
          line[scan_counter + 1] = d;
          scan_counter += 2;
        }
      
      if (scan_counter >= 120)
        {
          for (d = 119; d >= 0; d--)
            if (line[d] != ' ')
              break;
          d += 1;
          line[d] = '\0';
          fprintf(fp, "%s", line);
          pending_nl = true;
          line[d] = ' ';
        }
       
      r = true;
      break;

    default:
      fprintf(stderr, "LPT: OTA '%04o\n", instr&0x3ff);
      exit(1);
    }
  
  return status(r);
}

#define LINES 66
#define VTAB 3

void LPT::next_line()
{
  line_number++;
  if (line_number >= LINES)
    {
      line_number = 0;
      fprintf(fp, "\f");
    }
}

void LPT::deal_pending_nl()
{
  if (pending_nl)
    {
      fprintf(fp, "\n");
      pending_nl = false;
      next_line();
    }
}

LPT::STATUS LPT::ocp(unsigned short instr)
{
  switch(instr & 01700)
    {
    case 01300:
      /* Form feed */
      open_file();
      deal_pending_nl();
      if (line_number != 0)
        {
          /*
            while (line_number != 0)
            {
            fprintf(fp, "\n");
            next_line();
            }
          */
          line_number = 0;
          fprintf(fp, "\f");
        }
      break;

    case 01600:
      /* Skip to vertical tab */
      open_file();
      deal_pending_nl();
      while ((line_number % VTAB) != 0)
        {
          fprintf(fp, "\n");
          next_line();
        }
      break;

    case 01700:
      /* Step one line */
      open_file();
      fprintf(fp, "\n");
      next_line();
      pending_nl = false;
      break;

    case 00100: /* Start scan */
      scan_counter = 0;
      break;

    case 00200: /* stop scan */

      break;

    default:
      fprintf(stderr, "LPT: OCP '%04o\n", instr&0x3ff);
      exit(1);
    }

  return STATUS_READY;
}

LPT::STATUS LPT::sks(unsigned short instr)
{
  bool r = 0;

  switch(instr & 01700)
    {
    case 00000:
      /* skip if ready */
      r = false;
      break;

    case 00200:
      /* skip if not busy */
      r = true;
      break;

    case 00300:
      /* skip if no alarm */
      r = true;
      break;

    case 00400:
      /* skip if not shifting paper */
      r = true;
      break;

    default:
      fprintf(stderr, "LPT: SKS '%04o\n", instr&0x3ff);
      exit(1);
    }
  
  return status(r);
}

LPT::STATUS LPT::smk(unsigned short mask)
{
  bool ready = false;

  this->mask = mask & SMK_MASK;
  
  if (ready && this->mask)
    p->set_interrupt(this->mask);
  else
    p->clear_interrupt(SMK_MASK);

  return STATUS_READY;
}

void LPT::event(int reason)
{
  switch(reason)
    {
    case REASON_MASTER_CLEAR:
      master_clear();
      break;
      
    default:
      fprintf(stderr, "%s %d\n", __PRETTY_FUNCTION__, reason);
      exit(1);
      break;
    }
}

void LPT::set_filename(char *filename)
{
  master_clear();
  this->filename = strdup(filename);
  pending_filename = 1;
}
