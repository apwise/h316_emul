/* Honeywell Series 16 emulator $Id: emul.cc,v 1.4 2001/06/09 22:12:02 adrian Exp $
 * Copyright (C) 1997, 1998  Adrian Wise
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
 * $Log: emul.cc,v $
 * Revision 1.4  2001/06/09 22:12:02  adrian
 * Various bug fixes
 *
 * Revision 1.3  1999/06/04 07:57:21  adrian
 * *** empty log message ***
 *
 * Revision 1.2  1999/02/25 06:54:55  adrian
 * Removed Printf, Fprintf etc.
 *
 * Revision 1.1  1999/02/20 00:06:35  adrian
 * Initial revision
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stdtty.hh"
#include "event.hh"
#include "proc.hh"
#include "instr.hh"
#include "monitor.hh"
#include "gtk/fp.h"

/* FP_UPDATE sets how many machine instructions to execute
 * before refreshing the GUI. It is basically a compromise
 * between precise updating of the front-panel display and
 * performance. It is chosen to be not a power of 2 or 10
 * in order to avoid keeping a static display when a program
 * is in a tight loop. */
#define FP_UPDATE 9753

#ifndef NO_GTK
/* fp_run() is the routine that is called by the GUI
 * front-panel to have the machine 'run'. It is installed
 * either as an idle routine, when the machine is
 * continuously running, or called once for a single-step
 * of FP memory access */

static void fp_run(struct FP_INTF *intf)
{
  Proc *p = (Proc *) intf->data;
  bool monitor_flag = 0;
  bool run = intf->running;
  int count = FP_UPDATE;

  /*
   * If a start button interrupt has been
   * caused then go tell the simulation kernel
   * that this occured.
   */
  if (intf->start_button_interrupt_pending)
    {
      //printf("%s start_button_interrupt_pending",
      //       __PRETTY_FUNCTION__);

      intf->start_button_interrupt_pending = 0;
      p->start_button();
    }

  if (intf->mode == FPM_MA)
    p->mem_access(intf->p_not_pp1, intf->store);
  else
    {
      if (intf->mode == FPM_SI)
        count = 1;

      /*
       * Do machine instructions until the count
       * expires, or the run flag is cleared (eg/ because
       * of a HLT instruction) or the monitor is
       * called for
       */
      while ((count--) && (run || (intf->mode == FPM_SI))
             && (!monitor_flag))
        p->do_instr(run, monitor_flag);

      if (intf->mode != FPM_SI)
        intf->running = run;
    }
}

/*
 * This is the called when the front-panel master clear
 * button is pressed
 */
static void fp_master_clear(struct FP_INTF *intf)
{
  Proc *p = (Proc *) intf->data;
  p->master_clear();
}

#endif

/*
 * Do whatever has to happen in the case that
 * an Alt-key is pressed
 */
static bool call_special_chars(Proc *p, int k)
{
  bool r = false;

  if ( k == ('s' | 0x80))
    {
      p->start_button();
      r = 1;
    }
  else if ( k == ('m' | 0x80))
    {
      p->goto_monitor();
      r = 1;
    }

  return r;
}

/*
 * Where it all starts...
 */
int main(int argc, char **argv)
{
  STDTTY *stdtty;
  Proc *p;
  bool front_panel = 1;
  int arg = 1;

  /*
   * If the very first option is "-t" then operate
   * in non-GUI mode (text mode) even if compiled with
   * GUI support
   */

  if ((argc>arg) && (strcmp(argv[arg], "-t")==0))
    {
      front_panel = 0;
      arg++;
    }

#ifndef NO_GTK
  /* Let GTK look at the options */
  if (front_panel)
    process_args(&argc, &argv);
#endif
  
  /*
   * Initialize event handling.
   *
   * Events are used to enable the I/O devices to schedule
   * call-backs at some future (simulation) time, allowing
   * then to simulate the timing associated with the
   * devices
   */
  Event::initialize();

  /*
   * Handling of the TTY is kind of complicated because
   * it does multiple things (it models the ASR connected to
   * the CPU and it is used to control the simulation and
   * provide the text-based monitor)
   *
   * Initialize the STDTTY and pass it to the newly created
   * Proc (processor), but then go back and tell the STDTTY
   * which Proc it is connected to; they each need to know
   * how to contact the other!
   */
  stdtty = new STDTTY;
  p = new Proc(stdtty);
  stdtty->set_proc(p, call_special_chars);

#ifndef NO_GTK  
  if (front_panel)
    {
      /*
       * Build an FP_INTF structure that interfaces
       * the GUI front-panel into the processor.
       * And then setup the front-panel and start
       * simulating...
       */
      struct FP_INTF *intf = p->fp_intf();
      intf->run = fp_run;
      intf->master_clear = fp_master_clear;

      setup_fp(intf);
    }
  else
#endif
    {
      /*
       * This is the text-based version
       */
      bool run = 0;
      bool monitor_flag = 0;
      Monitor *m;
      FILE *fp = NULL;
 
      /*
       * Initialize the monitor
       */
      m = new Monitor(p, stdtty, argc, argv);

      /*
       * Open a script file (if any)
       * And let the monitor process it.
       */
      if (argc > arg)
        {
          fp = fopen(argv[arg], "r");
          if (!fp)
            {
              fprintf(stderr, "Could not open <%s> for reading\n",
                      argv[arg]);
              exit(1);
            }
        }
      m->do_commands(run, &fp);
      
      /*
       * The monitor will return when the processor is to
       * 'run', or to quit the program. While the processor
       * is running simulate instructions, and when the
       * processor stops (or ALT-M is pressed, setting
       * monitor_flag) call the monitor again
       */

      while (run)
        {
          while (run && (!monitor_flag))
            p->do_instr(run, monitor_flag);
          
          monitor_flag = 0;
          m->do_commands(run, &fp);
        }
    }
  exit(0);
}
