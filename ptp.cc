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
#include "ptp.hh"

#define SPEED 110 // charcters per second
#define SMK_MASK (1 << (16-10))

enum PTP_REASON
  {
    PTP_REASON_CHARACTER,

    PTP_REASON_NUM
  };

static char *ptp_reason[PTP_REASON_NUM] __attribute__ ((unused)) =
{
  "Character ready"

};

PTP::PTP(Proc *p, STDTTY *stdtty)
{
  this->p = p;
  this->stdtty = stdtty;

  fp = NULL;

  master_clear();
}

void PTP::master_clear()
{
  mask = 0;

  if (fp)
    fclose(fp);
  fp = NULL;
  pending_filename = 0;

  ascii_file = 0;

  power_on = false;
  ready = false;
};

bool PTP::ina(unsigned short instr, signed short &data)
{
  fprintf(stderr, "%s Input from punch\n", __PRETTY_FUNCTION__);
  exit(1);
  return 0;
}

bool PTP::ota(unsigned short instr, signed short data)
{
  bool r = false;
  int c;
  
  switch(instr & 0700)
    {
    case 0000:
      if (!power_on)
	turn_power_on();
      
      r = ready;
      
      if (ready)
	{
	  c = data & 0xff;
	  
	  if (ascii_file)
	    {
              c &= 0x7f;

	      if (c != 015) // ignore Carriage Return
		{
		  if (c == 012) // LF is newline
		    c = '\n';
		  putc(c, fp); // loose the top bit
		}
	    }
	  else
	    putc(c, fp);
	  
	  ready = false;
	  
	  Event::queue(p, (1000000 / SPEED), this, PTP_REASON_CHARACTER );
	}
      break;
      
    default:
      fprintf(stderr, "PTP: OTA '%04o\n", instr&0x3ff);
      exit(1);
    }
  
  return r;
}

void PTP::turn_power_on()
{
  char buf[256];
  char *cp;
  
  if (!fp)
    {
      while (!fp)
	{
	  if (pending_filename)
	    strcpy(buf, filename);
	  else
	    stdtty->get_input("PTP: Filename>", buf, 256, 0);
	  
	  cp = buf;
	  if (buf[0]=='&')
	    {
	      ascii_file = 1;
	      cp++;
	    }
	  
	  fp = fopen(cp, "wb");
	  if (!fp)
	    {
	      fprintf(((pending_filename) ? stderr : stdout),
		      "Could not open <%s> for writing\n", cp);
	      if (pending_filename)
		exit(1);
	    }
	  
	  if (pending_filename)
	    delete filename;
	  
	  pending_filename = 0;
	}
    }
  
  if (!power_on)
    {
      // 5 second turn-on delay
      Event::queue(p, 5000000, this, PTP_REASON_CHARACTER );
      power_on = true;
    }	
}

void PTP::ocp(unsigned short instr)
{
  
  switch(instr & 0700)
    {
    case 0000:
      turn_power_on();
      break;
      
    case 0100:
      power_on = false;
      
      /*
       * Don't close the file; some programs (like the
       * assembler) stop the punch, and then restart
       * it. However, fflush() it to disk.
       */

      if (fp)
	fflush(fp);
      
      break;
      
    default:
      fprintf(stderr, "PTP: OCP '%04o\n", instr&0x3ff);
      exit(1);
    }
  
}

bool PTP::sks(unsigned short instr)
{
  bool r = 0;
  
  switch(instr & 0700)
    {
    case 0000: r = ready; break;
    case 0100: r = power_on; break;
    case 0400: r = !(ready && mask); break;
      
    default:
      fprintf(stderr, "PTP: SKS '%04o\n", instr&0x3ff);
      exit(1);
    }
  
  return r;
}

void PTP::smk(unsigned short mask)
{
  this->mask = mask & SMK_MASK;
  
  if (ready && this->mask)
    p->set_interrupt(this->mask);
  else
    p->clear_interrupt(SMK_MASK);
}

void PTP::event(int reason)
{
  switch(reason)
    {
    case REASON_MASTER_CLEAR:
      master_clear();
      break;
      
    case PTP_REASON_CHARACTER:
      ready = true;
      break;
			
    default:
      fprintf(stderr, "%s %d\n", __PRETTY_FUNCTION__, reason);
      exit(1);
      break;
    }
}

void PTP::set_filename(char *filename)
{
  master_clear();
  this->filename = strdup(filename);
  pending_filename = 1;
}
