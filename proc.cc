// {{{ GPL copyright notice

/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 1999, 2005, 2010, 2011, 2018  Adrian Wise
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

// }}}

// {{{ Includes

#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include <iostream>

#ifdef RTL_SIM
#define NO_GTK (1)
#else
#include "iodev.hh"
#include "rtc.hh"
#include "ptr.hh"
#include "asr_intf.hh"
#include "stdtty.hh"
#endif

#include "instr.hh"
#include "proc.hh"

#ifndef NO_GTK
#include "gtk/fp.h"
#endif

using namespace std;

#define DEBUG 0
#define CORE_SIZE 32768
#define TRACE_BUF (1024*1024)

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

// }}}

// {{{ struct Btrace

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
  unsigned short p, instr, y;
};

// }}}
// {{{ key-in loader

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

// }}}

/*
 * Dummy IODEV that can be used for non-device time events
 * (MFM = mainframe)
 */
class MFM : public IODEV
{
public:
  MFM(Proc *p)
    : IODEV(p)
  {}
  
  ~MFM() {}
  
  void event(int reason)
  {
    p->event(reason);
  }
};
#define MFM_REASON_LIMIT 0

// {{{ Proc::Proc(STDTTY *stdtty)

Proc::Proc(STDTTY *stdtty UNUSED, bool HasEa)
  : extend_allowed(HasEa)
  , addr_mask((HasEa) ? 0x7fff : 0x3fff)
  , exit_code(0)
  , exit_called(false)
  , instr_table()
{
  long i;

  /*
   * Core memory
   * and flags to record which words get written
   */
  core = (signed short *) malloc(sizeof(signed short) * CORE_SIZE);
  modified = (bool *) malloc(sizeof(bool) * CORE_SIZE);

  for (i=0; i<CORE_SIZE; i++) {
    core[i] = 0;
    modified[i] = 0;
  }
  x = core[0];

  for (i=1; i<020; i++)
    core[i] = keyin[i-1];

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
#ifdef RTL_SIM
  devices = 0;
  dmc_devices = 0;
#else
  devices = IODEV::dispatch_table(this, stdtty);
  dmc_devices = IODEV::dmc_dispatch_table(this, stdtty, devices);
#endif

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

  wrts = 0;
  wrt_addr[0] = wrt_addr[1] = 0;
  wrt_data[0] = wrt_data[1] = 0;

  mfm = new MFM(this);
}

// }}}

Proc::~Proc()
{
#ifndef RTL_SIM
  event_queue.discard_events();
#endif
  delete mfm;
}

void Proc::exit(int code)
{
  exit_code   = code;
  exit_called = true;
  run = false;
}

void Proc::set_limit(unsigned long long half_cycles)
{
  event_queue.queue(this->half_cycles + half_cycles, mfm, MFM_REASON_LIMIT);
}

void Proc::event(int reason)
{
  switch (reason) {
  case MFM_REASON_LIMIT:
    printf("\n%010lu: limit reached\n", half_cycles);
    goto_monitor();
    break;

  default:
    fprintf(stderr, "Bad event reason <%d>\n", reason);
    exit(1);
  }
}

void Proc::dump_trace(const char *filename, int n)
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
                  instr_table.disassemble(btrace_buf[i].p,
                                          btrace_buf[i].instr,
                                          btrace_buf[i].brk,
                                          btrace_buf[i].y,
                                          true/*y_valid*/));
        }
      i = (i+1) % TRACE_BUF;
    } while (trace_ptr != i);
  
  if (fp != stdout)
    fclose(fp);
}

// }}}
// {{{ void Proc::dump_disassemble(char *filename, int first, int last)

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
      if (instr_table.defined(instr))
        fprintf(fp, "%s\n", 
                instr_table.disassemble(i, instr, false));
      else
        fprintf(fp, "%06o  %06o    %s\n", i, instr, "???");       
    }
  
  if (fp != stdout)
    fclose(fp);
}

// }}}

// {{{ void Proc::dump_vmem(char *filename, int exec_addr, bool octal)

void Proc::dump_vmem(char *filename, int exec_addr, bool octal)
{
  int i;
  unsigned short instr;
  bool mod;
  bool dac;
  bool skip = true;
  FILE *fp=stdout;
  char buf[256];

  //printf("dump_vmem(): %s %d\n", filename, exec_addr);

  if (filename) {
    fp = fopen(filename, "w");
    if (!fp) {
      fprintf(stderr, "Could not open <%s>\n", filename);
      return;
    }
  }
  
  for (i=0; i<CORE_SIZE; i++) {
    instr = core[i];
    mod = modified[i];
    dac = false;

    //printf("dump_vmem(): %d\n", i);

    if ((i == 0) && (exec_addr != 0)) {
      if ((exec_addr & (~0777)) != 0)
        // Not sector zero
        instr = 0102020; // JMP *'20
      else
        instr = 0002000 | exec_addr; // JMP exec_addr
      mod = true;
    } else if ((i == 020) && ((exec_addr & (~0777)) != 0)) {
      instr = exec_addr;
      mod = true;
      dac = true;
    } else if ((i>0) && (i<020))
      mod = true;

    if (mod && skip) {
      // Need an @ line
      if (octal)
        fprintf(fp, "@%06o\n", i);
      else
        fprintf(fp, "@%04x\n", i);
    }

    if (mod) {
      if (dac)
        sprintf(buf, "%06o  %06o    DAC  '%06o", i, instr, instr);       
      else if (instr_table.defined(instr))
        sprintf(buf, "%s", 
                instr_table.disassemble(i, instr, false));
      else
        sprintf(buf, "%06o  %06o    %s", i, instr, "???");       
      
      if (octal)
        fprintf(fp, "%06o // %s\n", instr, buf);
      else
        fprintf(fp, "%04x // %s\n", instr, buf);
    }

    skip = !mod;

  }
  
  if (fp != stdout)
    fclose(fp);
}

// }}}

void Proc::dump_coemem(char *filename, int exec_addr)
{
  int i, k, n, last_addr;
  unsigned short instr;
  FILE *fp=stdout;
  
  //printf("dump_coemem(): %s %d\n", filename, exec_addr);

  if (filename) {
    fp = fopen(filename, "w");
    if (!fp) {
      fprintf(stderr, "Could not open <%s>\n", filename);
      return;
    }
  }
  
  last_addr = 0;
  for (i=CORE_SIZE-1; i>=0; i--) {
    if (modified[i]) {
      last_addr = i;
      break;
    }
  }

  n = 1024 * 12;
  if (last_addr >= n)
    last_addr = n-1;
  
  fprintf(fp, "memory_initialization_radix=16;\n");
  fprintf(fp, "memory_initialization_vector=\n");
   
  k = 0;
  for (i=0; i<=last_addr; i++) {
    instr = core[i];
    
    if ((i == 0) && (exec_addr != 0)) {
      if ((exec_addr & (~0777)) != 0)
        // Not sector zero
        instr = 0102020; // JMP *'20
      else
        instr = 0002000 | exec_addr; // JMP exec_addr
    } else if ((i == 020) && ((exec_addr & (~0777)) != 0)) {
      instr = exec_addr;
    }

    fprintf(fp, "%04x%c", ((int) instr), ((i == last_addr)?';':','));

    k++;
    if ((k >= 8) || (i == last_addr)) {
      fprintf(fp, "\n");
      k = 0;
    } else {
      fprintf(fp, " ");
    }
  }
  
  if (fp != stdout)
    fclose(fp);
}

// {{{ void Proc::increment_p(unsigned short n)

void Proc::increment_p(unsigned short n)
{
  unsigned short g = ((extend) ?
                      ((p & 0x7fff) | (m & 0x8000)) :
                      ((p & 0x3fff) | (m & 0xc000)));
  unsigned short d = g + n;

  p = (((extend) || (!extend_allowed)) ? d :
       ((d & 0xbfff) | (fetched_p & 0x4000)));
}

// }}}

// {{{ void Proc::master_clear(void)

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
  
  pi = pi_pending = 0;
  interrupts = 0;
  dmc_req = 0;
  dmc_cyc = false;
  start_button_interrupt = 0;
  rtclk = false;
  melov = false;
  break_flag = false;
  break_intr = false;
  break_addr = 0;

  goto_monitor_flag = 0;
  last_jmp_self_minus_one = jmp_self_minus_one = false;
 
  c = 0;
  dp = 0;
  extend = 0;
  disable_extend_pending = 0;
  restrict = false;
  
  sc = 0x3f;
#ifndef RTL_SIM
  IODEV::master_clear_devices(devices);
  event_queue.discard_events();
#endif
}

// }}}

#ifndef NO_GTK
// {{{ struct FP_INTF *Proc::fp_intf()
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

// }}}
#endif

// {{{ static signed short short_add(signed short a, signed short m,

/*****************************************************************
 * Add setting the carry bit to overflow
 * Weird, I know but that's the way it works on a series 16
 *****************************************************************/
static signed short short_add(signed short a, signed short m,
                              bool &c)
{
  long aa, mm, rr;
  signed short r;

  aa = static_cast<signed long>(a);
  mm = static_cast<signed long>(m);
  rr = aa + mm;
  
  r = static_cast<signed short>(rr);

  c = (rr != static_cast<signed long>(r));

  return r;
}

// }}}
// {{{ static signed short short_sub(signed short a, signed short m,

static signed short short_sub(signed short a, signed short m,
                              bool &c)
{
  long aa, mm, rr;
  signed short r;

  aa = static_cast<signed long>(a);
  mm = static_cast<signed long>(m);
  rr = aa - mm;
  
  r = static_cast<signed short>(rr);

  c = (rr != static_cast<signed long>(r));

  return r;
}

// }}}
// {{{ static signed short short_adc(signed short a, signed short m,

/*****************************************************************
 * Add, with the C bit added in
 *****************************************************************/
static signed short short_adc(signed short a, signed short m,
                              bool &c)
{
  signed short r, v;

  v = ~(a ^ m);      // MSB is signs same
  r = a + m + c;
  v &= (r ^ m);      // if signs were same and now differ
  c = (v >> 15) & 1; // set overflow
  return r;
}

// }}}
// {{{ void Proc::set_x(unsigned short n)

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

void Proc::set_just_x(unsigned short n)
{
  x = n;
}

// }}}

/*****************************************************************
 * Set or clear one bit of the "interrupts" variable
 * Used by the devices to generate or clear interrupt
 * conditions.
 *****************************************************************/
void Proc::set_interrupt(unsigned short bit)
{
  interrupts |= bit;
}

void Proc::clear_interrupt(unsigned short bit)
{
  interrupts &= (~bit);
}

void Proc::set_rtclk(bool v)
{
  rtclk = v;
}

void Proc::set_dmcreq(unsigned short bit)
{
  dmc_req |= bit;
}

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
void Proc::do_instr(bool &x_run, bool &monitor_flag)
{
  unsigned short instr;

  run = true; // else we wouldn't have gotten here!

  /*
   * fetched records whether or not an instruction
   * has already been fetched (into the M register)
   */
  if (fetched) {
    
    /*
     * Yes, it has been fetched
     * Normal executions sequence...
     */
    
    if (goto_monitor_flag) {
      monitor_flag = 1;
      goto_monitor_flag = 0;
    }
    
    if (break_flag) {
      half_cycles -= 2; // Due to the fetch
      /*
       * If this was a break then the contents of
       * M are not an instruction, since it was not
       * fetched.
       */

      instr = (((break_intr) ?
                0120000 :  // JST *
                0024000) | // IRS
               break_addr);

      if (dmc_cyc) {
        instr = dmc_dev; // Mark DMC break
      }

      fetched_p = 0;

#ifndef RTL_SIM
      if ((!break_intr) && (break_addr == 061)) {
        rtclk = false;
      }
#endif
    } else {
      instr = m;

      /*
       * Step the program counter
       */
      fetched_p = p;

      /*
       * On the real hardware, at the instant that the
       * p comes through the adder, m is zero (which can
       * affect the upper bits of the resulting p.
       * So model that, but then the instruction gets
       * put (back) into the m register.
       */ 
      m = 0;
      increment_p();
      m = instr;
    }

    if (dmc_cyc) {
      bool dmc_erl;
      
      // DMC cycle 1
      dmc_addr     = read(break_addr);;
      break_addr   = break_addr + 1;
      half_cycles += 2;

      // DMC cycle 2
      dmc_erl      = ((dmc_addr & 0x7fff) == (read(break_addr) & 0x7fff));
      break_addr   = dmc_addr & 0x7fff;
      dmc_addr     = (dmc_addr & 0x8000) | ((dmc_addr + 1) & 0x7fff);
      half_cycles += 2;

      // DMC cycle 3
      if (dmc_addr & 0x8000) {
        // Do an input
        signed short tmp;
        dmc_devices[dmc_dev]->dmc(tmp, dmc_erl);
        //cout << "DMC " << dec << dmc_dev << " write " << oct << tmp << " to "
        //     << oct << break_addr << endl;
        write(break_addr, tmp);
      } else {
        // Do an output
        signed short tmp = read(break_addr);
        dmc_devices[dmc_dev]->dmc(tmp, dmc_erl);
      }
      break_addr   = 000020 + (dmc_dev * 2);
      half_cycles += 2;
      
      // DMC cycle 4
      write(break_addr, dmc_addr);
      dmc_cyc      = false;
      half_cycles += 2;

    } else {
      
      last_jmp_self_minus_one = jmp_self_minus_one;
      jmp_self_minus_one = false;
      melov = false;
      
      /*
       * Now simply jump to the routine that handles
       * this instruction.
       */
      (this->*instr_table.dispatch(instr))(instr);

      /*
       * We either interrupted, or interrupts weren't
       * enabled. Either way clear this flag.
       */
      start_button_interrupt = 0;
    }
    
    // binary trace ...
    
    btrace_buf[trace_ptr].brk = break_flag;
    btrace_buf[trace_ptr].v = true;
    btrace_buf[trace_ptr].half_cycles = half_cycles;
    btrace_buf[trace_ptr].a = a;
    btrace_buf[trace_ptr].b = b;
    btrace_buf[trace_ptr].c = c;
    btrace_buf[trace_ptr].x = x;
    btrace_buf[trace_ptr].p = (break_flag) ? 0xffff : fetched_p;
    btrace_buf[trace_ptr].y = y;  // EA of MR instructions
    btrace_buf[trace_ptr].instr = instr;
    
    trace_ptr = (trace_ptr + 1) % TRACE_BUF;
  } else {
    p = y; /* Front panel updates Y not P
            * So copy Y into P before fetching */
  }

  break_flag = false;
    
  /*
   * Figure out what break, if any, to do.
   */
  if (rtclk) {
    break_flag = true;
    break_intr = false;
    break_addr = 061;
  } else if (dmc_req != 0) {
    break_flag = true;
    break_intr = false;
    for (int i=0; i<16; i++) {
      const unsigned int mask = (1 << i);
      if ((dmc_req & mask) != 0) {
        break_addr = 000020 + (2 * i);
        dmc_req &= (~mask);
        dmc_dev = i; 
        break;
      }
    }
    dmc_cyc = true;
  } else if (pi && (interrupts || start_button_interrupt)) {
    break_flag = true;
    break_intr = true;
    break_addr = 063;
      
    pi = pi_pending = 0; // disable interrupts
    extend = extend_allowed; // force extended addressing    
  } if (melov) {
    break_flag = true;
    break_intr = true;
    break_addr = 062;
  } else {
    (void) read(p);   // Leaving the instruction in the m register
  }

  half_cycles += 2; // Due to fetch
  
  if (fetched) {
    /*
     * Enable the interrupts
     */
    if (pi_pending && (!pi) && (!dmc_cyc)) {
      pi = true;
      pi_pending = false;
    }
    
#ifndef RTL_SIM
    /*
     * If we're still running then service any
     * events that are due now.
     * If we just halted then flush all pending
     * events.
     */
    if (run) {
      (void) event_queue.call_devices(half_cycles);
    } else {
      event_queue.flush_events(half_cycles);
    }
#endif
  }
  
#ifndef RTL_SIM
  /*
   * If there is TTY input then service it
   */
  if (STDTTY::get_tty_input())
    STDTTY::service_tty_input();
#endif
  
  fetched = true;
  x_run = run; // pass back the run status
  monitor_flag = goto_monitor_flag; // and the monitor flag
}

unsigned short Proc::ea(unsigned short instr)
{
  unsigned short d;
  bool sec_zero;
  bool indirect;
  bool indexing;
  bool first = true;

  m = instr;
  y = fetched_p; // Address of instr (i.e. before increment)

  sec_zero = ((m & 0x0200) == 0);
  indexing = ((m & 0x4000) != 0);
  
  do {      
    indirect = ((m & 0x8000) != 0);
    
    if (sec_zero) {
      if ((extend) || (!extend_allowed))
        d = ((j & 0x7e00) | (m & 0x81ff));
      else
        d = ((j & 0x3e00) | (m & 0x81ff)) | (fetched_p & 0x4000);
    } else {
      if (first)
        d = ((y & 0xfe00) | (m & 0x01ff));
      else {
        if ((extend) || (!extend_allowed))
          d = m;
        else
          d = (m & 0xbfff) | (fetched_p & 0x4000);
      }        
    }
 
    if ((indexing) &&  // Looks like indexing called for
        ((!extend) ||  // Can always do it in non-extended mode
         (!indirect))) // and when no more indirection
      d += x;
      
    if ((extend) || (!extend_allowed))
      y = d;
    else
      y = (d & 0xbfff) | (y & 0x4000);
    
    if (indirect) {
      half_cycles += 2;
    
      (void) read(y & 0x7fff); /* sets m */
    
      sec_zero = ((m & ((extend) ? 0x7e00 : 0x3e00) ) == 0);
      if (!extend)
        indexing = ((m & 0x4000) != 0);
    }

    first = false;

  } while (indirect);
  
  //printf("ea() y = %06o\n", y);
  return y;
}

#ifndef RTL_SIM
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
      
      EventQueue::EventTime next_event_time;
      
      if (event_queue.next_event_time(next_event_time))
        {
          // jump time forward to the next event
          half_cycles = next_event_time;
    
          // call devices to actually change the state
          // of the devices
          event_queue.call_devices(half_cycles);
    
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

          if ((instr & 077) == IODEV::ASR_DEVICE)
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
#endif

void Proc::dump_memory()
{
  FILE *fp = fopen("memdump", "w");
  int i, k;
  unsigned short instr;

  k = -1;
  for (i=0; i<32768; i++)
    {
      if (modified[i])
        {
          if ((k+1) != i)
            fprintf(fp, "\n");

          instr = core[i];
          fprintf(fp, "%s\n",
                  instr_table.disassemble(i, instr, 0));
          k = i;
        }
    }

  fclose(fp);
}

void Proc::write(unsigned short addr, signed short data)
{
  m = data;
  y = addr;

  unsigned short ma = addr & addr_mask;

  if (((addr ^ j) & ((extend) ? 0x7fff : 0x3fff)) == 0) {
    core[ma] = x = data;
    modified[ma] = 1;
    if (wrts < 2) {
      wrt_addr[wrts] = ma;
      wrt_data[wrts] = data;
      wrts++;
    }
  } else {
    if ((ma == 0) || (ma >= 020)) {
      core[ma] = data;
      modified[ma] = 1;
      if (wrts < 2) {
        wrt_addr[wrts] = ma;
        wrt_data[wrts] = data;
        wrts++;
      }
    }
  }
  
  //if (((addr >= 03000) && (addr < 03010)) || (addr == 0107))
  //  printf("Write %06o @ %06o\n", data & 0xffff, addr&0xffff );
}

int Proc::get_wrt_info(unsigned short addr[2], unsigned short data[2])
{
  int r = wrts;
  int i;
  for (i=0; i<wrts; i++) {
    addr[i] = wrt_addr[i] & addr_mask;
    data[i] = wrt_data[i];
  }
  wrts = 0;
  return r;
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

const char *Proc::dis()
{
  unsigned short instr = core[p];
  return instr_table.disassemble(p, instr, 0);
}

void Proc::flush_events()
{
#ifndef RTL_SIM
  event_queue.flush_events(half_cycles);
#endif
}

void Proc::set_ptr_filename(char *filename UNUSED)
{
#ifndef RTL_SIM
  ((PTR *) devices[IODEV::PTR_DEVICE])->set_filename(filename);
#endif
}

void Proc::set_ptp_filename(char *filename UNUSED)
{
#ifndef RTL_SIM
  ((PTR *) devices[IODEV::PTP_DEVICE])->set_filename(filename);
#endif
}

void Proc::set_plt_filename(char *filename UNUSED)
{
#ifndef RTL_SIM
  ((PTR *) devices[IODEV::PLT_DEVICE])->set_filename(filename);
#endif
}

void Proc::set_lpt_filename(char *filename UNUSED)
{
#ifndef RTL_SIM
  ((PTR *) devices[IODEV::LPT_DEVICE])->set_filename(filename);
#endif
}

void Proc::set_asr_ptr_filename(char *filename UNUSED)
{
#ifndef RTL_SIM
  ((ASR_INTF *) devices[IODEV::ASR_DEVICE])->set_filename(filename, false);
#endif
}

void Proc::set_asr_ptp_filename(char *filename UNUSED)
{
#ifndef RTL_SIM
  ((ASR_INTF *) devices[IODEV::ASR_DEVICE])->set_filename(filename, true);
#endif
}

void Proc::asr_ptp_on(char *filename UNUSED)
{
#ifndef RTL_SIM
  ((ASR_INTF *) devices[IODEV::ASR_DEVICE])->asr_ptp_on(filename);
#endif
}

void Proc::asr_ptr_on(char *filename UNUSED)
{
#ifndef RTL_SIM
  ((ASR_INTF *) devices[IODEV::ASR_DEVICE])->asr_ptr_on(filename);
#endif
}

bool Proc::special(char k UNUSED)
{
  bool r = false;
  
#ifndef RTL_SIM
  r = ((ASR_INTF *) devices[IODEV::ASR_DEVICE])->special(k);
  
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
#endif
  return r;
}

/******************************************************************/


void Proc::abort()
{
  printf("Dumping \"tracefile\"\n");

  dump_trace("tracefile",0);

  printf("Dumping \"memdump\"\n");

  dump_memory();

  exit(1);
}

void Proc::unimplemented(unsigned short instr UNUSED)
{
#ifndef RTL_SIM
  printf("%s\n", __PRETTY_FUNCTION__);
  printf("Instr = `%06o at `%06o\n", instr, p);
  
  Proc::abort();
#endif
}

void Proc::do_CRA(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);
#endif
  a = 0;
}

void Proc::do_IAB(unsigned short instr UNUSED)
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
  unsigned short yy;
  signed short mm;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 4;

  yy = ea(instr);
  mm = read(yy);
  write(yy, a);
  a = mm;
}

void Proc::do_INK(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((c & 1) << 15) | ((dp & 1) << 14) |
    ((extend & 1) << 13) |
    (sc & 0x3f); /* NB Programmers' Reference is wrong,
                  * 6 bits to A (not 5) from schematics */
}

void Proc::do_LDA(unsigned short instr)
{
  unsigned addr;
  signed short original_a = a;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  addr = ea(instr);

  if (dp) addr &= ((~0)<<1); // loose the LSB if double precision

  a = read(addr);

  if (dp)
    {
      half_cycles += 2;
      addr |= 1;
      sc = original_a & 0x3f;
      b = read(addr);
    }
}

void Proc::do_LDX(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 4;
  write(((extend) ? j : ((j & 0x3fff) | (p & 0x4000))),
        read(ea(instr & 0xbfff)));
}

void Proc::do_OTK(unsigned short instr UNUSED)
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
      
  sc = (a & 0x1f); /* Really is only 5 bits (schematics) */
}

void Proc::do_STA(unsigned short instr)
{
  unsigned addr;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  addr = ea(instr);

  if (dp) addr &= ((~0L)<<1); // loose the LSB if double precision

  write(addr, a);

  // if addr is odd, then this write will be
  // overwritten by the b register in the case
  // of double precision store (DST)

  if (dp)
    {
      sc = a & 0x3f;
      half_cycles += 2;
      addr |= 1;
      write(addr, b);
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

void Proc::do_ACA(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = short_add(a, ((signed short)(c & 1)), c);
}

void Proc::do_ADD(unsigned short instr)
{
  signed short original_a = a;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  if (dp)
    {
      unsigned addr = ea(instr);
      signed long da, dm, v;

      half_cycles += 2;

      if (dp) addr &= ((~0)<<1); // loose the LSB
      dm = ((read(addr) & 0xffff) << 15) | (read(addr | 1) & 0x7fff);
      if (dm & 0x40000000)
        dm |= ((~0) << 31);
      
      da = ((a & 0xffff) << 15) | (b & 0x7fff);
      if (da & 0x40000000)
        da |= ((~0) << 31);

      v = ~(da ^ dm);        // Bit 30 is signs same
      da += dm;
      v &= (da ^ dm);        // if signs were same and now differ
 
      //printf("%s da=0x%08x\n", __PRETTY_FUNCTION__, da);  
      a = (da >> 15) & 0xffff;
      b = da & 0x7fff;
      c = (v >> 30) & 1;

      sc = original_a & 0x3f;
    }
  else
    a = short_add(a, read(ea(instr)), c);
}

void Proc::do_AOA(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = short_add(a, ((signed short) 1), c);
}

void Proc::do_SUB(unsigned short instr)
{
  signed short original_a = a;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  if (dp)
    {
      unsigned addr = ea(instr);
      signed long da, dm, v;

      half_cycles += 2;

      if (dp) addr &= ((~0)<<1); // loose the LSB
      dm = ((read(addr) & 0xffff) << 15) | (read(addr | 1) & 0x7fff);
      if (dm & 0x40000000)
        dm |= ((~0) << 31);
      
      da = ((a & 0xffff) << 15) | (b & 0x7fff);
      if (da & 0x40000000)
        da |= ((~0) << 31);

      dm = -dm;

      v = ~(da ^ dm);        // Bit 30 is signs same
      da += dm;
      v &= (da ^ dm);        // if signs were same and now differ

      a = (da >> 15) & 0xffff;
      b = da & 0x7fff;
      c = (v >> 30) & 1;

      sc = original_a & 0x3f;
    }
  else
    a = short_sub(a, read(ea(instr)), c);
}

void Proc::do_TCA(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 1;
  // C remains unmodified
  bool v = c;
  a = short_sub(((signed short)0), a, v);
}

void Proc::do_ANA(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;
  a &= read(ea(instr));
}

void Proc::do_CSA(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = (a & 0x8000) != 0;
  a &= 0x7fff;
}

void Proc::do_CHS(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a ^= 0x8000;
}

void Proc::do_CMA(unsigned short instr UNUSED)
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

void Proc::do_SSM(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a |= 0x8000;
}

void Proc::do_SSP(unsigned short instr UNUSED)
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
  signed short lsc;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  lsc = ex_sc(instr);

  while (lsc < 0)
    {
      half_cycles ++;
      c = (a >> 15) & 1;
      a = ((a & 0x7fff) << 1) | c;
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_ALS(unsigned short instr)
{
  signed short lsc;
  unsigned short d;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  lsc = ex_sc(instr);
  c = 0;
  while (lsc < 0)
    {
      half_cycles ++;
      d = ((a & 0x7fff) << 1);
      if ((a ^ d) & 0x8000)
        c = 1;
      a = d;
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_ARR(unsigned short instr)
{
  signed short lsc;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  lsc = ex_sc(instr);
  while (lsc < 0)
    {
      half_cycles ++;
      c = a & 1;
      a = ((a >> 1) & 0x7fff) | (c << 15);
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_ARS(unsigned short instr)
{
  signed short lsc;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  lsc = ex_sc(instr);
  while (lsc < 0)
    {
      half_cycles ++;
      c = a & 1;
      a = (a & 0x8000)|((a >> 1) & 0x7fff);
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_LGL(unsigned short instr)
{
  signed short lsc;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  lsc = ex_sc(instr);
  while (lsc < 0)
    {
      half_cycles ++;
      c = (a >> 15) & 1;
      a = (a << 1) & 0xfffe;
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_LGR(unsigned short instr)
{
  signed short lsc;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  lsc = ex_sc(instr);
  while (lsc < 0)
    {
      half_cycles ++;
      c = a & 1;
      a = ((a >> 1) & 0x7fff);
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_LLL(unsigned short instr)
{
  signed short lsc;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  lsc = ex_sc(instr);
  while (lsc < 0)
    {
      half_cycles ++;
      c = (a >> 15) & 1;
      a = ((a << 1) & 0xfffe) | ((b >> 15) & 1);
      b = (b << 1) & 0xfffe;
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_LLR(unsigned short instr)
{
  signed short lsc;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  lsc = ex_sc(instr);
  while (lsc < 0)
    {
      half_cycles ++;
      c = (a >> 15) & 1;
      a = ((a << 1) & 0xfffe) | ((b >> 15) & 1);
      b = ((b << 1) & 0xfffe) | c;
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_LLS(unsigned short instr)
{
  signed short lsc;
  signed short d;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  lsc = ex_sc(instr);
  c = 0;
  while (lsc < 0)
    {
      half_cycles ++;
      d = ((a << 1) & 0xfffe) | ((b >> 14) & 1);
      if ((d ^ a) & 0x8000)
        c = 1;
      a = d;
      b = (b & 0x8000) | ((b << 1) & 0x7ffe);
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_LRL(unsigned short instr)
{
  signed short lsc;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  lsc = ex_sc(instr);
  while (lsc < 0)
    {
      half_cycles ++;
      c = b & 1;
      b = ((a & 1) << 15) | ((b >> 1) & 0x7fff);
      a = (a >> 1) & 0x7fff;
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_LRR(unsigned short instr)
{
  signed short lsc;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  lsc = ex_sc(instr);
  while (lsc < 0)
    {
      half_cycles ++;
      c = b & 1;
      b = ((a & 1) << 15) | ((b >> 1) & 0x7fff);
      a = (c << 15) | ((a >> 1) & 0x7fff);
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_LRS(unsigned short instr)
{
  signed short lsc;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  lsc = ex_sc(instr);
  while (lsc < 0)
    {
      half_cycles ++;
      c = b & 1;
      b = (b & 0x8000) | ((a & 1) << 14) | ((b >> 1) & 0x3fff);
      a = (a & 0x8000) | ((a >> 1) & 0x7fff);
      lsc++;
    }
  sc = b & 0x3f;
}

void Proc::do_INA(unsigned short instr)
{
#ifndef RTL_SIM
  signed short d;
#endif
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;
  bool rerun = false;

  do {
#ifdef RTL_SIM
    if (drlin) {
      increment_p();
      if (instr & 01000)
        a = 0;
      a |= inb;
      rerun = false;
    }
#else
    if (devices[instr & 077]->ina(instr, d)) {
      increment_p();
      if (instr & 01000)
        a = 0;
      a |= d;
      rerun = false;
    } else
      rerun = optimize_io_poll(instr);
#endif
  } while (rerun);
}

void Proc::do_OCP(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;
#ifndef RTL_SIM
  devices[instr & 0x3f]->ocp(instr);
#endif
}

void Proc::do_OTA(unsigned short instr UNUSED)
{
  bool rerun = false;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  do {
#ifdef RTL_SIM
    if (drlin) {
      increment_p();
      rerun = false;
    }
#else
    if (devices[instr & 0x3f]->ota(instr, a)) {
      increment_p();
      rerun = false;
    } else
      rerun = optimize_io_poll(instr);
#endif
  } while (rerun);
}

void Proc::do_SMK(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
#ifndef RTL_SIM
  IODEV::set_mask(devices, a);
#endif
  half_cycles += 2;
}

void Proc::do_SKS(unsigned short instr UNUSED)
{
  bool rerun = 0;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 2;

  do {
#ifdef RTL_SIM
    if (drlin) {
      increment_p();
      rerun = 0;
    }
#else
    if (devices[instr & 0x3f]->sks(instr)) {
      increment_p();
      rerun = 0;
    } else
      rerun = optimize_io_poll(instr);
#endif
  } while (rerun);
}

void Proc::do_CAS(unsigned short instr)
{
  signed short mm;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 4;
  mm = read(ea(instr));
  if (a == mm)
    p += 1;
  else if (a < mm)
    p += 2;
}

void Proc::do_ENB(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  pi_pending = 1;
}

void Proc::do_HLT(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (restrict) {
    melov = true;
  } else {
    run = false;
  }
}

void Proc::do_INH(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  pi_pending = pi = false;
}

void Proc::do_IRS(unsigned short instr)
{
  unsigned short yy;
  signed short d;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 4;
  yy = ea(instr);
  d = read(yy) + 1;
  write(yy, d);
  increment_p(((!break_flag) && (d==0)) ? 1 : 0);

#ifndef RTL_SIM
  if ((d == 0) && break_flag &&
      (break_addr = 061) && (devices[IODEV::RTC_DEVICE])) {
    static_cast<RTC *>(devices[IODEV::RTC_DEVICE])->rollover();
  }
#endif
}

void Proc::do_JMP(unsigned short instr)
{
  unsigned short new_p;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  new_p = ea(instr);
  
  if ((new_p & addr_mask) == ((fetched_p-1) & addr_mask))
    jmp_self_minus_one = 1;
  
  p = (((extend) || (!extend_allowed)) ? new_p :
       ((new_p & 0xbfff) | (fetched_p & 0x4000)));

  if (disable_extend_pending)
    extend = disable_extend_pending = 0;
}

void Proc::do_JST(unsigned short instr)
{
  unsigned short yy, return_addr;
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  half_cycles += 4;
  yy = ea(instr);

  /*
   * Operation depends on extend mode.
   * Store 14 or 15 bit address as appropriate
   * leave upper bit(s) alone
   */

  if (extend)
    return_addr = (read(yy) & 0x8000) | (p & 0x7fff);
  else
    return_addr = (read(yy) & 0xc000) | (p & 0x3fff);

  write(yy, return_addr);
  p = yy+1;
}

void Proc::do_NOP(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
}

void Proc::do_RCB(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = 0;
}

void Proc::do_SCB(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = 1;
}

void Proc::do_SKP(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  increment_p();
}

void Proc::do_SLN(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (a & 0x0001)
    increment_p();
}

void Proc::do_SLZ(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if ((a & 0x0001) == 0)
    increment_p();
}

void Proc::do_SMI(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (a<0)
    increment_p();
}

void Proc::do_SNZ(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (a != 0)
    increment_p();
}

void Proc::do_SPL(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (a >= 0)
    increment_p();
}

void Proc::do_SR1(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (!ss[0])
    increment_p();
}

void Proc::do_SR2(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (!ss[1])
    increment_p();
}

void Proc::do_SR3(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (!ss[2])
    increment_p();
}

void Proc::do_SR4(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (!ss[3])
    increment_p();
}

void Proc::do_SRC(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (!c)
    increment_p();
}

void Proc::do_SS1(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (ss[0])
    increment_p();
}

void Proc::do_SS2(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (ss[1])
    increment_p();
}

void Proc::do_SS3(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (ss[2])
    increment_p();
}

void Proc::do_SS4(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (ss[3])
    increment_p();
}

void Proc::do_SSC(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (c)
    increment_p();
}

void Proc::do_SSR(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if ((!ss[0]) && (!ss[1]) && (!ss[2]) && (!ss[3]))
    increment_p();
}

void Proc::do_SSS(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (ss[0] || ss[1] || ss[2] || ss[3])
    increment_p();
}

void Proc::do_SZE(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  if (a==0)
    increment_p();
}

void Proc::do_CAL(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a &= 0x00ff;
}

void Proc::do_CAR(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a &= 0xff00;
}

void Proc::do_ICA(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((a << 8) & 0xff00) | ((a >> 8) & 0x00ff);
}

void Proc::do_ICL(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((a >> 8) & 0x00ff);
}

void Proc::do_ICR(unsigned short instr UNUSED)
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

void Proc::do_ad1(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = a + 1;
}

void Proc::do_ad1_15(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = a + 1;
  half_cycles++;
}

void Proc::do_adc(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = a + (c & 1);
}

void Proc::do_adc_15(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = a + (c & 1);
  half_cycles++;
}

void Proc::do_cm1(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = (-1) + (c & 1);
}

void Proc::do_ltr(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  
  a = (a & 0xff00) | ((a >> 8) & 0xff);
}

void Proc::do_btr(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a |= ((a >> 8) & 0xff);
}

void Proc::do_btl(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a |= ((a << 8) & 0xff00);
}

void Proc::do_rtl(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = ((a << 8) & 0xff00) | (a & 0xff);
}

void Proc::do_rcb_ssp(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = 0;
  a &= 0x7fff;
}

void Proc::do_cpy(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = (a >> 15) & 1;
}

void Proc::do_btb(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = ((a | (a << 8)) & 0xff00) | ((a | (a >> 8)) & 0xff);
}

void Proc::do_bcl(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = (( a | (a >> 8)) & 0xff);
}

void Proc::do_bcr(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = (( a | (a << 8)) & 0xff00);
}

void Proc::do_ld1(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = 1;
  half_cycles ++;
}

void Proc::do_isg(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = ((c & 1) << 1) - 1;
  half_cycles ++;
}

void Proc::do_cma_aca(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = (~a) + (c & 1);
  half_cycles ++;
}

void Proc::do_cma_aca_c(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = (~a) + (c & 1);
  c |= (a >> 15) & 1;

  half_cycles ++;
}

void Proc::do_a2a(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = a + 2;
  half_cycles ++;
}

void Proc::do_a2c(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = a + ((c & 1) << 1);
  half_cycles++;
}

void Proc::do_ics(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = (a & 0x8000) | ((a >> 8) & 0xff);
}

void Proc::do_scb_a2a(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = 1;
  a = a + 2;
  half_cycles++;
}

void Proc::do_scb_aoa(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  c = 1;
  a = a + 1;
  half_cycles++;
}

void Proc::do_a2c_scb(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = a + ((c & 1) << 1);
  c = 1;
  half_cycles++;
}

void Proc::do_aca_scb(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = a + (c & 1);
  c = 1;
  half_cycles++;
}

void Proc::do_icr_scb(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((a << 8) & 0xff00);
  c = 1;
}

void Proc::do_rtl_scb(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((a << 8) & 0xff00) | (a & 0xff);
  c = 1;
}

void Proc::do_btb_scb(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  a = ((a | (a << 8)) & 0xff00) | ((a | (a >> 8)) & 0xff);
  c = 1;
}

void Proc::do_noa(unsigned short instr UNUSED)
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

void Proc::do_DXA(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  disable_extend_pending = 1;
}

void Proc::do_EXA(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  disable_extend_pending = 0;
  extend = extend_allowed;
}

void Proc::do_ERM(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
  pi_pending = true;
  restrict = true;
}

/*
 * Memory parity option
 */
void Proc::do_RMP(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif
}

/*
 * High speed arithmetic option
 */

void Proc::do_DBL(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  dp = 1;
}

static int divide(short &ra, short &rb, const short &rm, short &sc, bool &cbitf)
{
  int d, e;
  bool azzzz, a00ff, m01ff, d01ff, remok;
  bool e00dj;

  int m = rm;
  int a = ra;
  int b = rb;

  bool c = true;
  int lsc = -17;
  int count = 0;

  bool madff = false;
  bool dogff = false;

  // T1
  a00ff = azzzz = (a < 0);
  m01ff = (m < 0);

  d = a;
  e = b;

  while (!dogff) {
    // T2

    // Some intermediate signals...
    d01ff = (d >> 15) & 1;
    remok = (d == 0) || (d01ff == azzzz);

    /* printf("lsc = %d, madff=%d, remok=%d, d01ff=%d\n",
       lsc, madff, remok, d01ff); */

    e00dj = (d01ff ^ m01ff ^ 1) & 1;

    if (lsc == -17) {
      // Essentially nothing happens
    } else if (lsc < -1) {

      if ((lsc == -16) && 
          (d01ff != azzzz))
        c = false;

      if (!c) {
        a00ff = d01ff;
        a = ((d << 1) & 0xfffe) | ((e >> 14) & 1);
        if ((a >> 15) & 1)
          a |= (-1 << 16);
        b = ((e << 1) & 0xfffe) | e00dj;
        if ((b >> 15) & 1)
          b |= (-1 << 16);
      }

    } else if (lsc == -1) {

      a00ff = d01ff;
      a = d;
      if ((a >> 15) & 1)
        a |= (-1 << 16);

      if (remok && (!d01ff))
        madff = true;

      if (!c) {
        //a00ff = d01ff;
        //a = d;
        //if ((a >> 15) & 1)
        //  a |= (-1 << 16);
        b = ((e << 1) & 0xfffe) | e00dj;
        if ((b >> 15) & 1)
          b |= (-1 << 16);

        //if ((lsc == -1) && remok && (!d01ff))
        // madff = true;
      }

    } else { // lsc == 0

      a00ff = d01ff;

      if (remok) {
        a = e & 0xffff;
        if ((a >> 15) & 1)
          a |= (-1 << 16);
        b = d & 0xffff;
        if ((b >> 15) & 1)        
          b |= (-1 << 16);
        madff = true;
        dogff = true;
      } else {
        a = d & 0xffff;
      }
    }

    /* printf("  a = %06d:%06o b = %06d:%06o a0=%d c=%d\n",
       a, (a & 0xffff),
       b, (b & 0xffff),
       a00ff, c); */

    if (lsc != 0)
      lsc++;

    // TLATE

    if (madff) {
      if ((a00ff != m01ff) &&
          (dogff))
        d = a + 1;
      else
        d = a;
    } else {
      if (a00ff == m01ff)
        d = a - m;
      else
        d = a + m;
      //printf("a00ff = %d, m01ff = %d\n", a00ff, m01ff);
    }
    e = b;

    /* 
       printf("   d = %06d:%07o e = %06d:%06o\n",
       d, (d & 0x1ffff),
       e, (e & 0xffff));
    */

    count++;
    assert(count < 20);
  }

  // T4
  if ((((d >> 15) & 3) != 0) &&
      (((d >> 15) & 3) != 3))
    c = 1;

  a = d & 0xffff;
  if ((a >> 15) & 1)
    a |= (-1 << 16);
        
  /*
    printf("T4\n");
    printf("  a = %06d:%06o b = %06d:%06o a0=%d c=%d\n",
    a, (a & 0xffff),
    b, (b & 0xffff),
    a00ff, c);
  */
  
  ra = a;
  rb = b;
  cbitf = c;
  sc = e & 0x3f;

  //printf("divide() count = %d\n", count);

  assert((count == 18) || (count == 19));

  return count+1;
}

void Proc::do_DIV(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  const short rm = (signed short) read(ea(instr));

  half_cycles += divide(a, b, rm, sc, c);
}

static long multiply(short &ra, short &rb, const short &rm, short &sc)
{
  int m = rm;
  int a = ra;
  int b = rb;
  int lsc = -8;
  bool b17, madff;
  int g, h;
  int d, e;
  long p, q;
  
  if (m & 0x8000) m |= ((~0) << 16);
  if (a & 0x8000) a |= ((~0) << 16);
  
  q = a * m;

  b = a;
  b17 = 0;
  a = 0;

  g = h = 0;

  while (lsc < 0) {

    if (lsc != -8) {
      if (madff) {
        b = ((((d & 1) != 0) ? ((~0) << 15) : 0) |
             ((a & 1) << 14) |
             ((e >> 2) & 0x3fff));
        b17 = ((e & 2) != 0);
        a = d >> 1;
      } else {
        b = ((((d & 2) != 0) ? ((~0) << 15) : 0) |
             ((d & 1) << 14) |
             ((e >> 2) & 0x3fff));
        b17 = ((e & 2) != 0);        
        a = d >> 2;
      }
    }

    lsc++;

    if ((b & 1) == b17) {
      madff = true;
      g = (a >> 1);
      if ((b & 2) != 0) { // B15 == 0?
        // No
        if (b17) {
          h = 0;
        } else {
          h = -m;
        }
      } else {
        // Yes (B15 == 0)
        if (b17) {
          h = m;
        } else {
          h = 0;
        }
      }
    } else {
      madff = false;
      g = a;
      if ((b & 2) != 0) { // B15 == 0?
        // No
        h = -m;
      } else {
        // Yes (B15 == 0)
        h = m;
      }
    }

    d = g + h;
    e = b;

  }

  if (madff) {
    b = (((a & 1) << 14) |
         ((e >> 2) & 0x3fff));
    a = d;
  } else {
    b = (((d & 1) << 14) |
         ((e >> 2) & 0x3fff));
    a = d >> 1;
  }

  p = (a << 15) | (b & 0x7fff);

  assert(p == q);

  ra = a;
  rb = b;
  sc = e & 0x3f;

  return p;
}

void Proc::do_MPY(unsigned short instr)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  const short rm = (signed short) read(ea(instr));

  half_cycles += 9;

  (void) multiply(a, b, rm, sc);
}

void Proc::do_NRM(unsigned short instr UNUSED)
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

void Proc::do_SCA(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  a = sc & 0x3f;
}

void Proc::do_SGL(unsigned short instr UNUSED)
{
#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  dp = 0;
}

void Proc::do_iab_sca(unsigned short instr UNUSED)
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
  int cccc = (instr >> 6) & 0x0f;
  short d;
  signed short lsc;

#if (DEBUG)
  printf("%s\n", __PRETTY_FUNCTION__);  
#endif

  // XXXXX - this was missing XXXXXXXXXX
  if ((cccc == 007) ||
      (cccc == 017))
    c = 0;
  // But is it for all shifts? Or just those that
  // set C by overflow? Matters if SC == 0. But can it be?
  // is zero lots?

  lsc = ex_sc(instr);
  while (lsc < 0)
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
      lsc++;
    }
  sc = b & 0x3f;
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
 
  if (instr & 0x001) cond &= ((~((int) c)) & 1);
  if (instr & 0x002) cond &= ((~ss[3]) & 1);
  if (instr & 0x004) cond &= ((~ss[2]) & 1);
  if (instr & 0x008) cond &= ((~ss[1]) & 1);
  if (instr & 0x010) cond &= ((~ss[0]) & 1);
  if (instr & 0x020) cond &= ((a == 0) & 1);
  if (instr & 0x040) cond &= ((~a) & 1);
  if (instr & 0x080) cond &= 1;
  if (instr & 0x100) cond &= (((~a) >> 15) & 1);

  if (instr & 0x200) cond ^= 1;

  if (cond) increment_p();
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
  bool M[17];
  int i;

  for (i=0; i<16; i++)
    M[16-i] = (instr >> i) & 1;
  M[0] = false; // (unused)

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

      EASBM = (M[9] || M[11] || azzzz);
      JAMKN = ((M[12] || M[16]) && (!azzzz));
      EASTL = (JAMKN) || (EASBM);

      //      EIK17 = ((M[15] || (c && (!M[13]))) && (!JAMKN));
      EIK17 = (M[15] && (c || (!M[13])) && (!JAMKN));

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

      azzzz = (M[8] && M[15] && (!azzzz));

    } while (azzzz);

  // T4
  CLATR = (M[11] || M[15] || M[16]);
  CLA1R = (M[10] || M[14]);
  EDAHS = ((M[11] && M[14]) || M[15] || M[16]);
  EDALS = ((M[11] && M[13]) || M[15] || M[16]);
  ETAHS = (M[9] && M[11]);
  ETALS = (M[10] && M[11]);
  EDA1R = ((M[8] && M[10]) || M[14]);

  overflow_to_c = (M[9] && (!M[11]));
  set_c = (M[8] && M[9]);
  d1_to_c = (M[10] && M[12]);

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
