/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 1999, 2005  Adrian Wise
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
 * This file constitutes the main simulation kernel of
 * the processor. 
 */

#include <stdlib.h>
#include <stdio.h>

#include "iodev.hh"
#include "ptr.hh"
#include "asr_intf.hh"
#include "stdtty.hh"

#include "event.hh"
#include "instr.hh"
#include "proc.hh"

#ifndef NO_GTK
#include "gtk/fp.h"
#endif

#define DEBUG 0
#define CORE_SIZE 32768
#define TRACE_BUF (1024*1024)

/*****************************************************************
 * Btrace; binary trace (because an earlier one that stored ASCII
 * strings was too slow).
 *
 * Store the main processor state in a circular buffer of 
 * Btrace structures allowing debugging of the route taken to
 * the point the processor stopped
 *****************************************************************/

struct Btrace
{
  bool v; // valid flag
  bool brk; // this is some sort of break
  unsigned long half_cycles;
  unsigned short a, b, x;
  bool c;
  unsigned short p, instr;
};

/*****************************************************************
 * The key-in loader
 *
 * Two versions - one for the high-speed paper-tape reader
 * one for the ASR.
 *****************************************************************/

#if 0 //PTR_ASR

/* With ASR as device... */
static unsigned short keyin[] =
  {
    0010057,
    0030004,
    0131004,
    0002003,
    0101040,
    0002003,
    0010000,
    0131004,
    0002010,
    0041470,
    0130004,
    0002013,
    0110000,
    0024000,
    0100040
  };

#else

static unsigned short keyin[] =
  {
    0010057,
    0030001,
    0131001,
    0002003,
    0101040,
    0002003,
    0010000,
    0131001,
    0002010,
    0041470,
    0130001,
    0002013,
    0110000,
    0024000,
    0100040
  };

#endif

/*****************************************************************
 * So, setup the Proc structure
 *****************************************************************/

Proc::Proc(STDTTY *stdtty)
{
  long i;

  /*
   * Core memory
   * and flags to record which words get written
   */
  core = (signed short *) malloc(sizeof(signed short) * CORE_SIZE);
  modified = (bool *) malloc(sizeof(bool) * CORE_SIZE);

  for (i=0; i<CORE_SIZE; i++)
    {
      core[i] = 0;
      modified[i] = 0;
    }
  x = core[0];

  for (i=1; i<020; i++)
    core[i] = keyin[i-1];

  /*
   * Build the instruction dispatch tables
   */
  Instr::build_instr_tables();

  /*
   * Front-panel sense switches
   */
  for (i=0; i<4; i++)
    ss[i] = 0;

  /*
   * We count in half-cycles, since some instructions
   * take something-and-a-half cycles.
   *
   * This counter can and does roll-over. For a 316
   * with a 1.6us cycle time this is approximately
   * once every 1 hour of simulated time.
   */
  half_cycles = 0;

  /*
   * Build a table of IO devices indexed by device
   * address
   */
  devices = IODEV::dispatch_table(this, stdtty);

  /*
   * Initialize the trace buffer
   */
  btrace_buf = new struct Btrace[TRACE_BUF];
  for (i=0;i<TRACE_BUF; i++)
    btrace_buf[i].v = 0;
  trace_ptr = 0;

  /*
   * Master-clear
   */
  run = 0;
  this->master_clear();

#ifdef TEST_GENERIC_SKIP
  test_generic_skip();
#endif
#ifdef TEST_GENERIC_GROUP_A
  test_generic_group_A();
#endif
}

/*****************************************************************
 * Dump the trace buffer
 *****************************************************************/
void Proc::dump_trace(char *filename, int n)
{
  int i;
  FILE *fp=stdout;
  
  /*
   * If a filename is passed then open it.
   */
  if (filename)
    {
      fp = fopen(filename, "w");
      if (!fp)
        {
          fprintf(stderr, "Could not open <%s>\n", filename);
          return;
        }
    }
  
  i = (trace_ptr + TRACE_BUF - n) % TRACE_BUF;

  do
    {
      if (btrace_buf[i].v)
        {
          fprintf(fp, "%010lu: A:%06o B:%06o X:%06o C:%d %s\n",
                  btrace_buf[i].half_cycles,
                  (btrace_buf[i].a & 0xffff),
                  (btrace_buf[i].b & 0xffff),
                  (btrace_buf[i].x & 0xffff),
                  (btrace_buf[i].c & 1),
                  Instr::disassemble(btrace_buf[i].p, btrace_buf[i].instr,
                                     btrace_buf[i].brk));
        }
      i = (i+1) % TRACE_BUF;
    } while (trace_ptr != i);
  
  if (fp != stdout)
    fclose(fp);
}

/*****************************************************************
 * Dump a disassembly of core between the given addresses
 *****************************************************************/
void Proc::dump_disassemble(char *filename, int first, int last)
{
  int i;
  unsigned short instr;
  FILE *fp=stdout;
  
  if (filename)
    {
      fp = fopen(filename, "w");
      if (!fp)        {
        fprintf(stderr, "Could not open <%s>\n", filename);
        return;
      }
    }
  
  for (i=first; i<=last; i++)
    {
      instr = core[i];
      if (Instr::defined(instr))
        fprintf(fp, "%s\n", 
                Instr::disassemble(i, instr, false));
      else
        fprintf(fp, "%06o  %06o    %s\n", i, instr, "???");       
    }
  
  if (fp != stdout)
    fclose(fp);
}

/*****************************************************************
 * Master clear
 *****************************************************************/
void Proc::master_clear(void)
{
  a = 0;
  b = 0;
  m = 0;
  p = 0;
  y = 0;
  j = 0;
  
  fetched = false;
  
  pi = pi_pending = pi_enable = 0;
  interrupts = 0;
  start_button_interrupt = 0;
  goto_monitor_flag = 0;
  last_jmp_self_minus_one = jmp_self_minus_one = false;
 
  c = 0;
  dp = 0;
  extend = 0;
  disable_extend_pending = 0;
  extend_allowed = 1;
  sc = 0;
  
  IODEV::master_clear_devices(devices);
  Event::discard_events();
}

#ifndef NO_GTK
/*****************************************************************
 * Build the front-panel interface structure
 *****************************************************************/
struct FP_INTF *Proc::fp_intf()
{
  struct FP_INTF *intf;
  int i;
  
  intf = new struct FP_INTF;
  
  intf->reg_value[RB_X] = &x;
  intf->reg_value[RB_A] = &a;
  intf->reg_value[RB_B] = &b;
  intf->reg_value[RB_PY] = (short *)&y;
  intf->reg_value[RB_OP] = &op;
  intf->reg_value[RB_M] = &m;
  
  for (i=0; i<4; i++)
    intf->ssw[i] = &ss[i];
  
  intf->pfh_not_pfi = 0;
  intf->mode=FPM_RUN;
  intf->store=0;
  intf->p_not_pp1=0;
  
  intf->running=run;
  
  intf->start_button_interrupt_pending=0;
  intf->power_fail_interrupt_pending=0;
  intf->power_fail_interrupt_acknowledge=0;
  
  /*
   * Call-back routines from the front-panel
   */
  
  intf->run = NULL;
  intf->master_clear = NULL;
  
  intf->cpu = CPU_H316;
  
  intf->data = (void *) this;
  
  return intf;
}
#endif

/*****************************************************************
 * Add setting the carry bit to overflow
 * Weird, I know but that's the way it works on a series 16
 *****************************************************************/
static signed short short_add(signed short a, signed short m,
                              bool &c)
{
  signed short r, v;

  v = ~(a ^ m);        // MSB is signs same
  r = a + m;
  v &= (r ^ m);        // if signs were same and now differ
  c = (v >> 15) & 1; // set overflow
  return r;
}

/*****************************************************************
 * Add, with the C bit added in
 *****************************************************************/
static signed short short_adc(signed short a, signed short m,
                              bool &c)
{
  signed short r, v;

  v = ~(a ^ m);        // MSB is signs same
  r = a + m + c;
  v &= (r ^ m);        // if signs were same and now differ
  c = (v >> 15) & 1; // set overflow
  return r;
}

/*****************************************************************
 * set the X register
 * Location "zero" in core tracks this, so the m and y registers
 * reflect this write.
 * With base sector relocation the J register holds the current
 * base sector. (J is normally zero)
 *****************************************************************/
void Proc::set_x(unsigned short n)
{
  x = n;
  m = n;
  y = j;
  core[j] = n;
}

/*****************************************************************
 * Set or clear one bit of the "interrupts" variable
 * Used by the devices to generate or clear interrupt
 * conditions.
 *****************************************************************/
void Proc::set_interrupt(unsigned short bit)
{
  interrupts |= bit;
};

void Proc::clear_interrupt(unsigned short bit)
{
  interrupts &= (~bit);
};

/*****************************************************************
 * Front-panel memory access
 *****************************************************************/
void Proc::mem_access(bool p_not_pp1, bool store)
{
  if (! p_not_pp1)
    y = y + 1;
  p = y;
  
  if (store)
    write(y, m);
  else
    m = read(y);
  
  fetched = false;
}

/*****************************************************************
 * This is where the action happens!
 *
 *****************************************************************/
void Proc::do_instr(bool &run, bool &monitor_flag)
{
  unsigned short instr, dp;

  this->run = 1; // else we wouldn't have gotten here!

  /*
   * fetched records whether or not an instruction
   * has already been fetched (into the M register)
   */
  if (fetched)
    {
      /*
       * Yes, it has been fetched
       * Normal executions sequence...
       */

      if(goto_monitor_flag)
        {
          monitor_flag = 1;
          goto_monitor_flag = 0;
        }

      /*
       * Interrupt enable is delayed by one instruction
       * (to enable return from interrupt). This is achieved
       * by setting pi_pending, which is copied to 
       * pi_enable here, and on to pi below.
       */
      pi_enable = pi_pending;

      /*
       * If pi is set (interrupts enabled)
       * then we might interrupt instead of executing
       * the fetched instruction
       */
      if (pi && (interrupts || start_button_interrupt))
        {
          pi = pi_pending = 0; // disable interrupts
          extend = extend_allowed; // force extended addressing
          
          //printf("Interrupt, P = `%06o `63=`%06o intr=%06o\n",
          //  p, core[063], interrupts);
          
          instr = 0120063; // JST *'63
          
          p -= 1; // so that when JST adds 1 we store correct address
          (this->*Instr::dispatch(instr))(instr);
          
          dp = 0xffff;
          btrace_buf[trace_ptr].brk = 1; // It was a break
        }
      else
        {
          dp = p; /* save P for the trace report
                   * since it may be modified */
          instr = m;
          
          last_jmp_self_minus_one = jmp_self_minus_one;
          jmp_self_minus_one = false;

          /*
           * Now simply jump to the routine that handles
           * this instruction.
           */
          (this->*Instr::dispatch(instr))(instr);
          
          btrace_buf[trace_ptr].brk = 0; // It was a normal instruction
        }
      
      // binary trace ...
      
      btrace_buf[trace_ptr].v = 1; // valid
      btrace_buf[trace_ptr].half_cycles = half_cycles;
      btrace_buf[trace_ptr].a = a;
      btrace_buf[trace_ptr].b = b;
      btrace_buf[trace_ptr].c = c;
      btrace_buf[trace_ptr].x = x;
      btrace_buf[trace_ptr].p = dp; // saved pc
      btrace_buf[trace_ptr].instr = instr;
      
      trace_ptr = (trace_ptr + 1) % TRACE_BUF;

      //       static bool trace_on = false;
      //       if (dp == 001461) trace_on = true;

      //       if (trace_on)
      //         {
      //           printf("A:%06o B:%06o X:%06o C:%d %s\n",
      //                   (a & 0xffff),
      //                   (b & 0xffff),
      //                   (x & 0xffff),
      //                   (c & 1),
      //                  Instr::disassemble(dp, instr,  false));
      //         }

      //       static long count=0;
      //       count ++;
      //       if ((count % 1000000) == 0)
      //         printf("count=%ld\n", count);
      //       if (count == 4000000)
      //         this->run = false;

      //       if ((half_cycles > 7500000) && ((half_cycles % 100000)==0))
      //         printf("%010lu\n", half_cycles);
      //       if (half_cycles == 33500000)
      //         this->run = false;

      /*
       * We either interrupted, or interrupts weren't
       * enabled. Either way clear this flag.
       */
      start_button_interrupt = 0;
     
      /*
       * Finally step the program counter
       */
      if (extend_allowed)
        p = (p + 1) & 0x7fff;
      else
        p = (p + 1) & 0x3fff;
    }
  else
    p = y; /* Front panel updates Y not P
            * So copy Y into P before fetching */

  (void) read(p);   // leaving the instruction in the m register
  half_cycles += 2; /* Fetch */
  
  if (fetched)
    {
      /*
       * Enable the interrupts
       */
      if (pi_enable && (!pi))
        {
          pi = 1;
          pi_enable = pi_pending = 0;
          //printf("Enabling Interrupt @ `%06o", dp);
        }
      
      /*
       * If we're still running then service any
       * events that are due now.
       * If we just halted then flush all pending
       * events.
       */
      if (this->run)
        (void) Event::call_devices(half_cycles, 1);
      else
        Event::flush_events(half_cycles);
    }

  /*
   * If there is TTY input then service it
   */
  if (STDTTY::get_tty_input())
    STDTTY::service_tty_input();

  fetched = true;
  run = this->run; // pass back the run status
  monitor_flag = goto_monitor_flag; // and the monitor flag
}

unsigned short Proc::ea(unsigned short instr)
{
  unsigned short d;
  bool sec_zero;
  bool ind;
  bool first = true;

  m = instr;
  y = p; // program counter
  sec_zero = ((instr & 0x0200) == 0);

  do
    {      
      ind = ((m & 0x8000) != 0);
      
      if (sec_zero)
        d = (j & 0x7e00) | (m & 0x81ff);
      else
        {
          if (first)
            d = (y & 0xfe00) | (m & 0x01ff);
          else
            {
              if (extend)
                d = m;
              else
                d = (p & 0x4000) | (m & 0xbfff);
            }
        }
      
      if ( ((m & 0x4000) && (!extend)) ||
           ((instr & 0x4000) && extend && (!ind)) )
        {
          // If in extend mode then normal indexing is
          // not allowed.
          // But when done with indirection the original
          // instr can call for indexing

          d += x;
        }
      
      /*
       * This is what is documented in the Programmers'
       * reference manual. However it isn't right because
       * it means that an attempt to address sector zero
       * from the upper 16kword of memory would address
       * sector 040
       */
       
      //       if (extend)
      //         y = d;
      //       else
      //         y = (d & 0xbfff) | (y & 0x4000);
      
      y = d & ((extend) ? 0x7fff : 0x3fff);

      if (ind)
        {
          half_cycles += 2;
    
          (void) read(y); /* sets m */
    
          if (extend)
            sec_zero = ((m & 0x7e00) == 0);
          else
            sec_zero = ((m & 0x3e00) == 0);
        }
      first = false;
    }
  while (ind);
  
  return y;
}

bool Proc::optimize_io_poll(unsigned short instr)
{
  bool r = false;

  if (last_jmp_self_minus_one)
    {

      /* We are at the point where we are executing a polling
       * IO instuction (INA, OTA, or SKS) that was reached
       * as a result of a 'JMP *-1' instruction, and which
       * is not going to skip (the JMP *-1).
       * Unless something happens this is an infinite loop.
       *
       * Usually however some event will take place that will
       * either cause an interrupt, thus escaping from the
       * loop, or make the IO instruction skip.
       *
       * Two exceptions exist; the start-button interrupt,
       * and input from the ASR (where we are really waiting
       * for keyboard input).
       */
      
      unsigned long next_event_time;
      
      if (Event::next_event_time(next_event_time))
        {
          // jump time forward to the next event
          half_cycles = next_event_time;
    
          // call devices to actually change the state
          // of the devices
          Event::call_devices(half_cycles, 1);
    
          // rerun the IO command in the hope that
          // it will now skip the JMP *-1
          r = true;
        }
      else
        {
          // There are no events pending
          //
          // Are we waiting on the ASR for input?
          //
          // We could wait for keyboard input
          // at this point, however this won't
          // work for the case that there is a GUI
          // frontpanel since that needs to be
          // serviced too.
          //
          // Ideally we should return to the
          // GUI code, remove the idle procedure
          // and wait until a GUI event or keyboard
          // input occurs before reinvoking the
          // call to the kernel.
          //
          // However, this is a bit complicated
          // so for now just let it poll the
          // device

          if ((instr & 077) == ASR_DEVICE)
            {
              // Ought to wait for keyboard or GUI
              // printf("Polling ASR\n");
            }
          else
            {
              // Do something about apparent infinite
              // loop?
            }
        }

      //
      // If there is an interrupt now pending then
      // don't re-run the instruction, let the interrupt
      // be taken instead
      //
      if (pi && (interrupts || start_button_interrupt))
        r = 0;
    }

  return r;
}

void Proc::dump_memory()
{
  FILE *fp = fopen("memdump", "w");
  int i, j;
  unsigned short instr;

  j = -1;
  for (i=0; i<32768; i++)
    {
      if (modified[i])
        {
          if ((j+1) != i)
            fprintf(fp, "\n");

          instr = core[i];
          fprintf(fp, "%s\n",
                  Instr::disassemble(i, instr, 0));
          j = i;
        }
    }

  fclose(fp);
}

void Proc::write(unsigned short addr, signed short data)
{
  m = data;
  y = addr;
  
  if (addr == j)
    {
      core[addr] = x = data;
      modified[addr] = 1;
    }
  else
    if ((addr<=0) || (addr>=020))
      {
        core[addr] = data;
        modified[addr] = 1;
      }
  
  //if (((addr >= 03000) && (addr < 03010)) || (addr == 0107))
  //  printf("Write %06o @ %06o\n", data & 0xffff, addr&0xffff );
}

/*
 * start button routines
 */
/*
  void Proc::set_start_button_interrupt()
  {
  start_button_interrupt = 1;
  }
*/

void Proc::start_button()
{
  //printf("%s\n", __PRETTY_FUNCTION__);
  start_button_interrupt = 1;
  //strt->queue_start();
}

void Proc::goto_monitor()
{
  goto_monitor_flag = 1;
}

char *Proc::dis()
{
  unsigned short instr = core[p];
  return Instr::disassemble(p, instr, 0);
}

void Proc::flush_events()
{
  Event::flush_events(half_cycles);
}

void Proc::set_ptr_filename(char *filename)
{
  ((PTR *) devices[PTR_DEVICE])->set_filename(filename);
}

void Proc::set_ptp_filename(char *filename)
{
  ((PTR *) devices[PTP_DEVICE])->set_filename(filename);
}

void Proc::set_lpt_filename(char *filename)
{
  ((PTR *) devices[LPT_DEVICE])->set_filename(filename);
}

void Proc::set_asr_ptr_filename(char *filename)
{
  ((ASR_INTF *) devices[ASR_DEVICE])->set_filename(filename, false);
}

void Proc::set_asr_ptp_filename(char *filename)
{
  ((ASR_INTF *) devices[ASR_DEVICE])->set_filename(filename, true);
}

void Proc::asr_ptp_on(char *filename)
{
  ((ASR_INTF *) devices[ASR_DEVICE])->asr_ptp_on(filename);
}

bool Proc::special(char k)
{
  bool r = false;
  
  r = ((ASR_INTF *) devices[ASR_DEVICE])->special(k);
  
  if ( (k & 0x7f) == 'h' )
    {
      printf("ALT-m Go to the monitor\n");
      printf("ALT-s Start button interrupt\n");
    }
  
  if (!r)
    {
      switch (k & 0x7f)
        {
        case 's':
          start_button();
          r = 1;
          break;
        case 'm':
          goto_monitor();
          r = 1;
          break;
        }
    }

  return r;
}

/******************************************************************/


void Proc::unimplemented(unsigned short instr)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  printf("Instr = `%06o at `%06o\n", instr, p);
  //exit(1);
}

void Proc::do_CRA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);
#endif
  a = 0;
}

void Proc::do_IAB(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);
#endif

  // In the real hardware the E register is used
  // to hold the B register.
  // This is also the "shift count" for the NRM instruction
  // so deliberately corrupt it here.
  sc = b;

  b = a;
  a = sc;
}

void Proc::do_IMA(unsigned short instr)
{
  unsigned short y;
  signed short m;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 4;

  y = ea(instr);
  m = read(y);
  write(y, a);
  a = m;
}

void Proc::do_INK(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((c & 1) << 15) | ((dp & 1) << 14) |
    ((extend & 1) << 13) |
    (sc & 0x1f);
}

void Proc::do_LDA(unsigned short instr)
{
  unsigned addr;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  addr = ea(instr);

  if (dp) addr &= 0xfffffffe; // loose the LSB if double precision

  a = read(addr);

  if (dp)
    {
      half_cycles += 2;
      addr |= 1;
      b = read(addr);
      sc = 0;
    }
}

void Proc::do_LDX(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 4;
  write(j, read(ea(instr & 0xbfff)));
}

void Proc::do_OTK(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;
  c = (a & 0x8000) != 0;
  dp = (a & 0x4000) != 0;
  if ( (a & 0x2000) != 0 )
    {
      disable_extend_pending = 0;
      extend = extend_allowed;
    }
  else
    disable_extend_pending = 1;
      
  sc = (a & 0x1f);
}

void Proc::do_STA(unsigned short instr)
{
  unsigned addr;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  addr = ea(instr);

  write(addr, a);

  // if addr is odd, then this write will be
  // overwritten by the b register in the case
  // of double precision store (DST)

  if (dp)
    {
      half_cycles += 2;
      addr |= 1;
      write(addr, b);
      sc = 0;
    }
}

void Proc::do_STX(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  write(ea(instr & 0xbfff), x);
}

void Proc::do_ACA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = short_add(a, ((signed short)(c & 1)), c);
}

void Proc::do_ADD(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  if (dp)
    {
      unsigned addr = ea(instr);
      signed long da, dm, v;

      half_cycles += 2;

      if (dp) addr &= 0xfffffffe; // loose the LSB
      dm = ((read(addr) & 0xffff) << 15) | (read(addr | 1) & 0x7fff);
      if (dm & 0x40000000)
        dm |= 0x80000000;
      
      da = ((a & 0xffff) << 15) | (b & 0x7fff);
      if (da & 0x40000000)
        da |= 0x80000000;

      v = ~(da ^ dm);        // Bit 30 is signs same
      da += dm;
      v &= (da ^ dm);        // if signs were same and now differ
 
      //printf("%s da=0x%08x\n", __PRETTY_FUNCTION__, da);  
      a = (da >> 15) & 0xffff;
      b = da & 0x7fff;
      c = (v >> 30) & 1;

      sc = 0;
    }
  else
    a = short_add(a, read(ea(instr)), c);
}

void Proc::do_AOA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = short_add(a, ((signed short) 1), c);
}

void Proc::do_SUB(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  if (dp)
    {
      unsigned addr = ea(instr);
      signed long da, dm, v;

      half_cycles += 2;

      if (dp) addr &= 0xfffffffe; // loose the LSB
      dm = ((read(addr) & 0xffff) << 15) | (read(addr | 1) & 0x7fff);
      if (dm & 0x40000000)
        dm |= 0x80000000;
      
      da = ((a & 0xffff) << 15) | (b & 0x7fff);
      if (da & 0x40000000)
        da |= 0x80000000;

      dm = -dm;

      v = ~(da ^ dm);        // Bit 30 is signs same
      da += dm;
      v &= (da ^ dm);        // if signs were same and now differ

      a = (da >> 15) & 0xffff;
      b = da & 0x7fff;
      c = (v >> 30) & 1;

      sc = 0;
    }
  else
    a = short_add(a, (-read(ea(instr))), c);
}

void Proc::do_TCA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 1;
  // C remains unmodified
  bool v = c;
  a = short_add(((signed short)0), (-a), v);
}

void Proc::do_ANA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;
  a &= read(ea(instr));
}

void Proc::do_CSA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = (a & 0x8000) != 0;
  a &= 0x7fff;
}

void Proc::do_CHS(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a ^= 0x8000;
}

void Proc::do_CMA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ~a;
}

void Proc::do_ERA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;
  a ^= read(ea(instr));
}

void Proc::do_SSM(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a |= 0x8000;
}

void Proc::do_SSP(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a &= 0x7fff;
}

signed short Proc::ex_sc(unsigned short instr)
{
  signed short res = instr & 0x003f;

  if (res != 0)
    res |= (~0x003f); // Sign extend

  return res;
}

void Proc::do_ALR(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  while (sc < 0)
    {
      half_cycles ++;
      c = (a >> 15) & 1;
      a = ((a & 0x7fff) << 1) | c;
      sc++;
    }
}

void Proc::do_ALS(unsigned short instr)
{
  unsigned short d;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  c = 0;
  while (sc < 0)
    {
      half_cycles ++;
      d = ((a & 0x7fff) << 1);
      if ((a ^ d) & 0x8000)
        c = 1;
      a = d;
      sc++;
    }
}

void Proc::do_ARR(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  while (sc < 0)
    {
      half_cycles ++;
      c = a & 1;
      a = ((a >> 1) & 0x7fff) | (c << 15);
      sc++;
    }
}

void Proc::do_ARS(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  while (sc < 0)
    {
      half_cycles ++;
      c = a & 1;
      a = (a & 0x8000)|((a >> 1) & 0x7fff);
      sc++;
    }
}

void Proc::do_LGL(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  while (sc < 0)
    {
      half_cycles ++;
      c = (a >> 15) & 1;
      a = (a << 1) & 0xfffe;
      sc++;
    }
}

void Proc::do_LGR(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  while (sc < 0)
    {
      half_cycles ++;
      c = a & 1;
      a = ((a >> 1) & 0x7fff);
      sc++;
    }
}

void Proc::do_LLL(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  while (sc < 0)
    {
      half_cycles ++;
      c = (a >> 15) & 1;
      a = ((a << 1) & 0xfffe) | ((b >> 15) & 1);
      b = (b << 1) & 0xfffe;
      sc++;
    }
}

void Proc::do_LLR(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  while (sc < 0)
    {
      half_cycles ++;
      c = (a >> 15) & 1;
      a = ((a << 1) & 0xfffe) | ((b >> 15) & 1);
      b = ((b << 1) & 0xfffe) | c;
      sc++;
    }
}

void Proc::do_LLS(unsigned short instr)
{
  signed short d;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  c = 0;
  while (sc < 0)
    {
      half_cycles ++;
      d = ((a << 1) & 0xfffe) | ((b >> 14) & 1);
      if ((d ^ a) & 0x8000)
        c = 1;
      a = d;
      b = (b & 0x8000) | ((b << 1) & 0x7ffe);
      sc++;
    }
}

void Proc::do_LRL(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  while (sc < 0)
    {
      half_cycles ++;
      c = b & 1;
      b = ((a & 1) << 15) | ((b >> 1) & 0x7fff);
      a = (a >> 1) & 0x7fff;
      sc++;
    }
}

void Proc::do_LRR(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  while (sc < 0)
    {
      half_cycles ++;
      c = b & 1;
      b = ((a & 1) << 15) | ((b >> 1) & 0x7fff);
      a = (c << 15) | ((a >> 1) & 0x7fff);
      sc++;
    }
}

void Proc::do_LRS(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  sc = ex_sc(instr);
  while (sc < 0)
    {
      half_cycles ++;
      c = b & 1;
      b = (b & 0x8000) | ((a & 1) << 14) | ((b >> 1) & 0x3fff);
      a = (a & 0x8000) | ((a >> 1) & 0x7fff);
      sc++;
    }
}

void Proc::do_INA(unsigned short instr)
{
  signed short d;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;
  bool rerun = 0;

  do {
    if (devices[instr & 077]->ina(instr, d))
      {
        p++;
        if (instr & 01000)
          a = 0;
        a |= d;
        rerun = 0;
      }
    else
      rerun = optimize_io_poll(instr);
  } while (rerun);
}

void Proc::do_OCP(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;
  devices[instr & 0x3f]->ocp(instr);
}

void Proc::do_OTA(unsigned short instr)
{
  bool rerun = 0;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  do {
    if (devices[instr & 0x3f]->ota(instr, a))
      {
        p++;
        rerun = 0;
      }
    else
      rerun = optimize_io_poll(instr);
  } while (rerun);
}

void Proc::do_SMK(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  IODEV::set_mask(devices, a);
  half_cycles += 2;
}

void Proc::do_SKS(unsigned short instr)
{
  bool rerun = 0;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  do {
    if (devices[instr & 0x3f]->sks(instr))
      {
        p++;
        rerun = 0;
      }
    else
      rerun = optimize_io_poll(instr);
  } while (rerun);
}

void Proc::do_CAS(unsigned short instr)
{
  signed short m;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 4;
  m = read(ea(instr));
  if (a == m)
    p += 1;
  else if (a < m)
    p += 2;
}

void Proc::do_ENB(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  pi_pending = 1;
}

void Proc::do_HLT(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  run = 0;
}

void Proc::do_INH(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  pi_enable = pi_pending = pi = 0;
}

void Proc::do_IRS(unsigned short instr)
{
  unsigned short y;
  signed short d;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 4;
  y = ea(instr);
  d = read(y) + 1;
  write(y, d);
  if (d==0)
    p++;
}

void Proc::do_JMP(unsigned short instr)
{
  unsigned short new_p;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  new_p = ea(instr);

  if (new_p == (p-1))
    jmp_self_minus_one = 1;

  p = new_p - 1; // 1 added on when p increments

  if (disable_extend_pending)
    extend = disable_extend_pending = 0;
}

void Proc::do_JST(unsigned short instr)
{
  unsigned short y, return_addr;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 4;
  y = ea(instr);

  /*
   * Operation depends on extend mode.
   * Store 14 or 15 bit address as appropriate
   * leave upper bit(s) alone
   */

  if (extend)
    return_addr = (read(y) & 0x8000) | ((p+1) & 0x7fff);
  else
    return_addr = (read(y) & 0xc000) | ((p+1) & 0x3fff);

  write(y, return_addr);
  p = y; // +1 added as standard
}

void Proc::do_NOP(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
}

void Proc::do_RCB(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = 0;
}

void Proc::do_SCB(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = 1;
}

void Proc::do_SKP(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  p++;
}

void Proc::do_SLN(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (a & 0x0001)
    p++;
}

void Proc::do_SLZ(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if ((a & 0x0001) == 0)
    p++;
}

void Proc::do_SMI(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (a<0)
    p++;
}

void Proc::do_SNZ(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (a != 0)
    p++;
}

void Proc::do_SPL(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (a >= 0)
    p++;
}

void Proc::do_SR1(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (!ss[0])
    p++;
}

void Proc::do_SR2(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (!ss[1])
    p++;
}

void Proc::do_SR3(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (!ss[2])
    p++;
}

void Proc::do_SR4(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (!ss[3])
    p++;
}

void Proc::do_SRC(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (!c)
    p++;
}

void Proc::do_SS1(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (ss[0])
    p++;
}

void Proc::do_SS2(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (ss[1])
    p++;
}

void Proc::do_SS3(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (ss[2])
    p++;
}

void Proc::do_SS4(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (ss[3])
    p++;
}

void Proc::do_SSC(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (c)
    p++;
}

void Proc::do_SSR(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if ((!ss[0]) && (!ss[1]) && (!ss[2]) && (!ss[3]))
    p++;
}

void Proc::do_SSS(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (ss[0] || ss[1] || ss[2] || ss[3])
    p++;
}

void Proc::do_SZE(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (a==0)
    p++;
}

void Proc::do_CAL(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a &= 0x00ff;
}

void Proc::do_CAR(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a &= 0xff00;
}

void Proc::do_ICA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((a << 8) & 0xff00) | ((a >> 8) & 0x00ff);
}

void Proc::do_ICL(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((a >> 8) & 0x00ff);
}

void Proc::do_ICR(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((a << 8) & 0xff00);
}

#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
/*
 * NPL group A
 */

void Proc::do_ad1(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = a + 1;
}

void Proc::do_ad1_15(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = a + 1;
  half_cycles++;
}

void Proc::do_adc(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = a + (c & 1);
}

void Proc::do_adc_15(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = a + (c & 1);
  half_cycles++;
}

void Proc::do_cm1(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = (-1) + (c & 1);
}

void Proc::do_ltr(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  
  a = (a & 0xff00) | ((a >> 8) & 0xff);
}

void Proc::do_btr(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a |= ((a >> 8) & 0xff);
}

void Proc::do_btl(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a |= ((a << 8) & 0xff00);
}

void Proc::do_rtl(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = ((a << 8) & 0xff00) | (a & 0xff);
}

void Proc::do_rcb_ssp(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = 0;
  a &= 0x7fff;
}

void Proc::do_cpy(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = (a >> 15) & 1;
}

void Proc::do_btb(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = ((a | (a << 8)) & 0xff00) | ((a | (a >> 8)) & 0xff);
}

void Proc::do_bcl(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = (( a | (a >> 8)) & 0xff);
}

void Proc::do_bcr(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = (( a | (a << 8)) & 0xff00);
}

void Proc::do_ld1(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = 1;
  half_cycles ++;
}

void Proc::do_isg(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = ((c & 1) << 1) - 1;
  half_cycles ++;
}

void Proc::do_cma_aca(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = (~a) + (c & 1);
  half_cycles ++;
}

void Proc::do_cma_aca_c(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = (~a) + (c & 1);
  c |= (a >> 15) & 1;

  half_cycles ++;
}

void Proc::do_a2a(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = a + 2;
  half_cycles ++;
}

void Proc::do_a2c(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = a + ((c & 1) << 1);
  half_cycles++;
}

void Proc::do_ics(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = (a & 0x8000) | ((a >> 8) & 0xff);
}

void Proc::do_scb_a2a(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = 1;
  a = a + 2;
  half_cycles++;
}

void Proc::do_scb_aoa(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = 1;
  a = a + 1;
  half_cycles++;
}

void Proc::do_a2c_scb(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = a + ((c & 1) << 1);
  c = 1;
  half_cycles++;
}

void Proc::do_aca_scb(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = a + (c & 1);
  c = 1;
  half_cycles++;
}

void Proc::do_icr_scb(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((a << 8) & 0xff00);
  c = 1;
}

void Proc::do_rtl_scb(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((a << 8) & 0xff00) | (a & 0xff);
  c = 1;
}

void Proc::do_btb_scb(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((a | (a << 8)) & 0xff00) | ((a | (a >> 8)) & 0xff);
  c = 1;
}

void Proc::do_noa(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  /* Do Nothing */
}

#endif

/*
 * Extended addressing
 */

void Proc::do_DXA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  disable_extend_pending = 1;
}

void Proc::do_EXA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  disable_extend_pending = 0;
  extend = extend_allowed;
}

/*
 * Memory parity option
 */
void Proc::do_RMP(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
}

/*
 * High speed arithmetic option
 */

void Proc::do_DBL(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  dp = 1;
}


void Proc::do_DIV(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  signed long da;
  unsigned short um;
  signed short sm, sa;

  half_cycles += 19;
  //
  // This is a guess since the actual divide time may
  // be 20, 21 or 22 half cycles. Since 2 are acounted for
  // by the fetch 18, 19 or 20 should be added here
  // however I haven't bothered with the data dependency
  // that determines the execution time.
  //

  // get the divisor, save its sign and then
  // convert it to unsigned.
  um = read(ea(instr));
  sm = (signed short) um;
  if (sm < 0)
    um = (-sm);

  sa = a; // Preserve the the sign of A
  da = ((a & 0xffff) << 15) | (b & 0x7fff);
  // Note that the sign bit is now in bit 30

  if (da & 0x40000000)
    da = (-da) & 0x7fffffff;

  /*
    printf("%s um=0x%04x sm=0x%04x\n", __PRETTY_FUNCTION__,
    um & 0xffff, sm & 0xffff);
    printf("%s a=0x%04x b=0x%04x c=%d\n", __PRETTY_FUNCTION__,
    a & 0xffff, b & 0xffff, c);
    printf("%s da=0x%08x\n", __PRETTY_FUNCTION__, da);
  */

  if (um == 0)
    c = 1;
  else
    {
      b = da % um;
      da = da / um;

      //
      // At this point an unsigned divide has
      // taken place.
      //
      // According to handwritten notes in the programmers'
      // manual the following holds:
      //
      // +/- gives -
      // -/- gives -
      // -/+ gives +
      // +/+ gives +
      //
      // However this appears to be wrong since in the
      // test code the following is expected:
      //
      // +/- gives - remainder +
      // -/- gives + remainder - 
      // -/+ gives - remainder -
      // +/+ gives + remainder +
      //
      // so, if the signs of the dividend and divisor
      // differ then the result is -ve
      

      if (((sm ^ sa) & 0x8000) != 0)
        da = -da;

      if (sa & 0x8000)
        b = -b;

      a = da & 0xffff;
      //
      // Test the upper bits of da for overflow
      // 
      c = ( ((a < 0) && ((da & 0xffff0000) != 0xffff0000)) ||
            ((a > 0) && ((da & 0xffff0000) != 0)) );

    }
    
  /*
    printf("%s a=0x%04x da=0x%08x b=0x%04x c=%d\n", __PRETTY_FUNCTION__,
    a & 0xffff, da & 0xffffffff, b & 0xffff, c);
  */

  sc = 0;
}

void Proc::do_MPY(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  signed long da;
  signed short sm;

  half_cycles += 9;

  // read returns unsigned so cast to a
  // signed value

  sm = (signed short) read(ea(instr));

  da = a * sm;

  // Now fix up the result to get the strange
  // 31 bit format in which the sign bit of the
  // lower 16-bit word is always zero.

  a = (da >> 15) & 0xffff;
  b = da & 0x7fff;

  sc = 0;
}

void Proc::do_NRM(unsigned short instr)
{
  signed short d;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  sc = 0;
  //c = 0; does it clear C?

  while (( (a & 0x8000) == ((a & 0x4000) << 1)) && (sc < 32))
    {
      half_cycles ++;
      d = ((a << 1) & 0xfffe) | ((b >> 14) & 1);
      a = d;
      b = (b & 0x8000) | ((b << 1) & 0x7ffe);
      sc++;
    }

  if (sc == 32)
    sc = 0;
}

void Proc::do_SCA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = sc & 0x3f;
}

void Proc::do_SGL(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  dp = 0;
}

void Proc::do_iab_sca(unsigned short instr)
{
  signed short d;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  d = sc;

  sc = b;
  b = a;
  a = d & 0x3f;
}

/*****************************************************************
 * Unusual instruction routines
 *****************************************************************/

/* generic_shift()
 *
 * is used for all of the undefined shift instructions,
 * it attempts to do what the 516 actually does, as
 * documented in "Micro-coding the DDP-516 Computer,
 * Donald A. Bell, NPL Com. Sci. T.M. 54 April 1971"
 * It isn't particularly useful, however some
 * programs accidently execute these instructions
 * (for example AB16-CMT5, a verification and test
 * program, executes '0417000) so it is necessary
 * to do something better than winge about an
 * "unimplemented instruction")
 *
 * At the present time (10 Sept 99) this has
 * NOT been tested against the real hardware to
 * prove it is correct.
 */

void Proc::generic_shift(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  int cccc = (instr >> 6) & 0x0f;
  short d;

  sc = ex_sc(instr);
  while (sc < 0)
    {
      half_cycles ++;

      switch (cccc)
        {
        case 003: // long right
          c = b & 1;
          b = (b & 0x8000) | ((a & 1) << 14) | ((b >> 1) & 0x3fff);
          a = ((a & 0x8000) | (c << 15)) | ((a >> 1) & 0x7fff);
          // note the bottom bit ORed in to the sign
          break;

        case 013: // long left
          d = ((a << 1) & 0xfffe) | ((b >> 14) & 1);
          if ((d ^ a) & 0x8000)
            c = 1;
          b = (b & 0x8000) | ((b << 1) & 0x7ffe) |
            ((a >> 15) & 1);
          a = d;
          // note top bit reappears at the bottom
          break;

        case 007:
          c = a & 1;
          a = ((a & 0x8000)| ((a & 1) << 15)) | ((a >> 1) & 0x7fff);
          // note the bottom bit ORed in to the sign
          break;

        case 017:
          d = ((a & 0x7fff) << 1) | ((a >> 15) & 1);
          if ((a ^ d) & 0x8000)
            c = 1;
          a = d;
          // note top bit reappears at the bottom
          break;

        default:
          abort();
        }
      sc++;
    }
}


void Proc::generic_skip(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);
#endif

  int cond = 1;

  /*  
   * R  - reverse condition
   * P  - accumulator positive
   * M  - memory parity error
   * E  - accumulator even
   * Z  - accumulator zero
   * S1 - sense switch 1 reset
   * S2 - sense switch 2 reset
   * S3 - sense switch 3 reset
   * S4 - sense switch 4 reset
   * C  - C-bit zero.
   */
 
  if (instr & 0x001) cond &= ((~c) & 1);
  if (instr & 0x002) cond &= ((~ss[3]) & 1);
  if (instr & 0x004) cond &= ((~ss[2]) & 1);
  if (instr & 0x008) cond &= ((~ss[1]) & 1);
  if (instr & 0x010) cond &= ((~ss[0]) & 1);
  if (instr & 0x020) cond &= ((a == 0) & 1);
  if (instr & 0x040) cond &= ((~a) & 1);
  if (instr & 0x080) cond &= 1;
  if (instr & 0x100) cond &= (((~a) >> 15) & 1);

  if (instr & 0x200) cond ^= 1;

  if (cond) p++;
}

#ifdef TEST_GENERIC_SKIP
void Proc::test_generic_skip()
{
  unsigned short i;
  int j;

  for (i=0x8000; i<0x8400; i++)
    {
      if (Instr::dispatch(i) != &Proc::generic_skip)
        {
          for (j=0; j<256; j++)
            {
              // set up the test condition
              ss[0] = j & 1;
              ss[1] = (j >> 1) & 1;
              ss[2] = (j >> 2) & 1;
              ss[3] = (j >> 3) & 1;
              c = (j >> 4) & 1;
              a = (((j >> 5) & 1) << 15) |
                (((j >> 6) & 1) ? 0x7ffe : 0) |
                ((j >> 7) & 1);

              p = 0;
              (this->*Instr::dispatch(i))(i);
              generic_skip(i);
              if ((p != 0) && (p != 2))
                {
                  fprintf(stderr, "%s: '%06o j=0x%02x p=%d, a=0x%04x, c=%d\n",
                          __PRETTY_FUNCTION__, i, j, p, a, c);
                  exit (1);
                }
            }
        }
    }
}
#endif

void Proc::generic_group_A(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  // first break the instruction into bits
  bool m[17];
  int i;

  for (i=0; i<16; i++)
    m[16-i] = (instr >> i) & 1;
  m[0] = false; // (unused)

  bool azzzz = false;

  bool EASTL, EASBM, JAMKN, EIK17;
  short s1, s2, s=0;
  bool v;
  int d;
  bool CLATR, CLA1R, EDAHS, EDALS, ETAHS, ETALS, EDA1R;
  bool overflow_to_c, set_c, d1_to_c;

  do
    {
      // T2
      if (azzzz)
        {
          a = s & 0xffff;
          half_cycles++;
        }

      // tlate

      EASBM = (m[9] || m[11] || azzzz);
      JAMKN = ((m[12] || m[16]) && (!azzzz));
      EASTL = (JAMKN) || (EASBM);

      //      EIK17 = ((m[15] || (c && (!m[13]))) && (!JAMKN));
      EIK17 = (m[15] && (c || (!m[13])) && (!JAMKN));

      s1 = (EASTL) ? a : 0;
      s2 = (EASBM) ? 0 : 0xffff;

      if (JAMKN)
        {
          s = s1 ^ s2;
          v=0;
          (void) short_adc((s1 & 0x8000), (s2 & 0x8000), v);
        }
      else
        {
          v = EIK17;
          s = short_adc(s1, s2, v);
        }

      // T3
      d = 0xffff; // Due to CLDTR
      d &= s;     // Due to ESTDS

      azzzz = (m[8] && m[15] && (!azzzz));

    } while (azzzz);

  // T4
  CLATR	= (m[11] || m[15] || m[16]);
  CLA1R	= (m[10] || m[14]);
  EDAHS	= ((m[11] && m[14]) || m[15] || m[16]);
  EDALS	= ((m[11] && m[13]) || m[15] || m[16]);
  ETAHS	= (m[9] && m[11]);
  ETALS	= (m[10] && m[11]);
  EDA1R	= ((m[8] && m[10]) || m[14]);

  overflow_to_c = (m[9] && (!m[11]));
  set_c	= (m[8] && m[9]);
  d1_to_c = (m[10] && m[12]);

  if (CLATR) // clear A register
    a = 0;
  if (CLA1R) // clear A1 register
    a &= 0x7fff;

  if (EDAHS) // enable D high to A high register
    a = (a & 0x00ff) | ((a | d) & 0xff00);
  if (EDALS) // enable D low to A low register
    a = (a & 0xff00) | ((a | d) & 0x00ff);
  if (ETAHS) // enable D transposed to A high register
    a = (a & 0x00ff) | ((a | (d << 8)) & 0xff00);
  if (ETALS) // enable D transposed to A low register
    a = (a & 0xff00) | ((a | (d >> 8)) & 0x00ff);

  if (EDA1R) // enable D1 to A1 register
    a = (a & 0x7fff) | ((a | d) & 0x8000);
  
  if (overflow_to_c)
    c = false;

  if (set_c) // unconditionally set C
    c = true;

  if (overflow_to_c) // conditionally set C from adder output
    c |= v;
  if (d1_to_c)
    c |= (d >> 15) & 1;
}

#ifdef TEST_GENERIC_GROUP_A
void Proc::test_generic_group_A()
{
  unsigned short i;
  int j, k;
  signed short test_data[] = {
    0x0000, 0xffff, 0x5555, 0xaaaa,
    0x5aa5, 0xa55a, 0x137f, 0xfec8,
    0x8000, 0};
  signed short sav_a;
  bool sav_c;
  unsigned long sav_h;
  bool problem;

  for (i=0xc000; i<0xc400; i++)
    {
      if (Instr::dispatch(i) != &Proc::generic_group_A)
        {
          // test one instruction
          for (j=0; ((j==0)|test_data[j]); j++)
            {
              for (k=0; k<2; k++)
                {
                  a = test_data[j];
                  c = k;
                  half_cycles = 0;

                  (this->*Instr::dispatch(i))(i);

                  sav_a = a;
                  sav_c = c;
                  sav_h = half_cycles;

                  a = test_data[j];
                  c = k;
                  half_cycles = 0;

                  generic_group_A(i);

                  problem = 0;
                  if (a != sav_a)
                    problem = 1;
                  if (c != sav_c)
                    problem = 1;
                  if (half_cycles != sav_h)
                    problem = 1;

                  if (problem)
                    {
                      fprintf(stderr,
                              "'%06o a=%04x c=%d instr: a=%04x c=%d %ld   gen: a=%04x c=%d %ld\n",
                              i & 0xffff, test_data[j] & 0xffff, k,
                              sav_a & 0xffff, sav_c, sav_h,
                              a & 0xffff, c, half_cycles );
                    }

                }
            }
        }
    }
}

#endif
