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
#include "stdtty.hh"

#include "proc.hh"
#include "iodev.hh"
#include "asr.hh"
#include "asr_intf.hh"

#define BAUD 9600 // 110
#define SMK_MASK (1 << (16-11))

#define STOP_CODE 0023

enum ASR_REASON
  {
    ASR_REASON_DUMMY_CYCLE,
    ASR_REASON_OUTPUT,
    ASR_REASON_INPUT,

    ASR_REASON_NUM
  };

static const char *asr_reason[ASR_REASON_NUM] __attribute__ ((unused)) =
{
  "Dummy cycle",
  "Output",
  "Input"
};

ASR_INTF::ASR_INTF(Proc *p, STDTTY *stdtty)
  : IODEV(p)
{
  this->p = p;
  asr = new ASR(stdtty);

  master_clear();
}

void ASR_INTF::master_clear()
{
  mask = 0;

  data_buf = 0;
  ready = false;
  input_pending = false;

  output_mode = false;
  output_pending = false;
  activity = ASR_ACTIVITY_NONE;
}

ASR_INTF::STATUS ASR_INTF::ina(unsigned short instr, signed short &data)
{
  char c;
  bool r = ready;

  if (ready)
    {
      switch(instr & 0777)
        {
        case 0004: data = data_buf; break;
        case 0204: data = data_buf & 0x3f; break;
        default:
          fprintf(stderr, "ASR_INTF: INA '%04o\n", instr&0x3ff);
          p->abort();
        }
                        
      input_pending = false;
      ready = false;
      p->clear_interrupt(SMK_MASK);
    }
  else
    {
      if (!input_pending)
        {
          if (asr->get_asrch(c))
            {
              data_buf = c & 0xff;
                                                        
              activity = ASR_ACTIVITY_INPUT;
              input_pending = true;

              p->queue(((1000000*11)/ BAUD), this, ASR_REASON_INPUT );
            }
        }
    }

  return status(r);
}

ASR_INTF::STATUS ASR_INTF::ocp(unsigned short instr)
{
  switch(instr & 0700)
    {
    case 0000:
                                                
      output_mode = 0;
      activity = ASR_ACTIVITY_NONE;
      ready = false;
      p->clear_interrupt(SMK_MASK);
      break;

    case 0100:
                                                
      output_mode = 1;
      activity = ASR_ACTIVITY_DUMMY;
      ready = 1;
      p->set_interrupt(mask);

      p->queue((1000000 / BAUD), this, ASR_REASON_DUMMY_CYCLE );
                        
      break;
                        
    default:
      fprintf(stderr, "ASR_INTF: OCP '%04o\n", instr&0x3ff);
      p->abort();
    }
  return STATUS_READY;
}

ASR_INTF::STATUS ASR_INTF::sks(unsigned short instr)
{
  bool r = false;

  switch(instr & 0700)
    {
    case 0000: r = ready; break;
    case 0100: r = (activity == ASR_ACTIVITY_NONE); break;
    case 0400: r = !(ready && mask); break;
    case 0500: r = ((!output_mode) && ready &&
                    ((data_buf & 0x7f) == STOP_CODE)); break;
    default:
      fprintf(stderr, "ASR_INTF: SKS '%04o\n", instr&0x3ff);
      p->abort();
    }

  //if (( (instr & 0x03ff) == 0004) && r)
  //    Printf("%s %04o r=%d\n",  __PRETTY_FUNCTION__, instr & 0x03ff, r);

  return status(r);
}

ASR_INTF::STATUS ASR_INTF::ota(unsigned short instr, signed short data)
{
  bool r = ready;

  if (ready)
    {
      switch(instr & 0700)
        {
        case 0000: data_buf = data; break;
        case 0200:
          data_buf = (data & 0x80) |
            ((~data & 0x20) << 1) |
            (data & 0x3f);
          break;
        default:
          fprintf(stderr, "ASR_INTF: OTA '%04o\n", instr&0x3ff);
          p->abort();
        }
                        
      ready = 0;
      p->clear_interrupt(SMK_MASK);

      if (activity == ASR_ACTIVITY_DUMMY)
        output_pending = true;
      else
        {
          activity = ASR_ACTIVITY_OUTPUT;
          p->queue(((1000000*11) / BAUD), this, ASR_REASON_OUTPUT );
        }
    }

  return status(r);
}

ASR_INTF::STATUS ASR_INTF::smk(unsigned short mask)
{
  this->mask = mask & SMK_MASK;

  if (ready && this->mask)
    p->set_interrupt(this->mask);
  else
    p->clear_interrupt(SMK_MASK);
  return STATUS_READY;
}

void ASR_INTF::event(int reason)
{
  switch (reason)
    {
    case REASON_MASTER_CLEAR:
      master_clear();
      break;

    case ASR_REASON_DUMMY_CYCLE:
      if (activity == ASR_ACTIVITY_DUMMY)
        {
          if (output_pending)
            {
              activity = ASR_ACTIVITY_OUTPUT;
              p->queue(((1000000*11) / BAUD), this, ASR_REASON_OUTPUT );
              output_pending = false;
            }
          else
            activity = ASR_ACTIVITY_NONE;
        }
      break;

    case ASR_REASON_OUTPUT:
      if (activity == ASR_ACTIVITY_OUTPUT)
        {
          activity = ASR_ACTIVITY_NONE;
          ready = 1;
          p->set_interrupt(mask);

          //Printf("ASR out=<0x%0x>\n", data_buf);
          asr->put_asrch(data_buf);
        }
      break;

    case ASR_REASON_INPUT:
      if (activity == ASR_ACTIVITY_INPUT)
        {
          activity = ASR_ACTIVITY_NONE;
          input_pending = false;
          ready = true;
          p->set_interrupt(mask);
        }
      break;

    default:
      fprintf(stderr, "Unexpected reason: %d\n", reason);
      p->abort();
    }

}

void ASR_INTF::set_filename(char *filename, bool asr_ptp)
{
  asr->set_filename(filename, asr_ptp);
}
 
void ASR_INTF::asr_ptr_on(char *filename)
{
  asr->asr_ptr_on(filename);
}
 
void ASR_INTF::asr_ptp_on(char *filename)
{
  asr->asr_ptp_on(filename);
}
 
bool ASR_INTF::special(char c)
{
  return asr->special(c);
}
