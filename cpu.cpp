/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 1999, 2005, 2010, 2011, 2018, 2026  Adrian Wise
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
 * This file constitutes the main simulation kernel of the processor.
 */

#include "cpu.hpp"
#include "rtc.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

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
namespace h16 {

  /*****************************************************************
   * The key-in loader
   *
   * Two versions - one for the high-speed paper-tape reader
   * one for the ASR.
   *****************************************************************/

#if 0 //PTR_ASR

  /* With ASR as device... */
  static uint16_t keyin[] = {
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

  static uint16_t keyin[] = {
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
}

using namespace h16;

CPU::CPU(bool hasEa)
  : ea_allowed(hasEa)
  , addr_mask((hasEa) ? 0x7fff : 0x3fff)
  , core(addr_mask+1)
  , modified(addr_mask+1)
  , half_cycles(0) {
  const unsigned core_size = addr_mask + 1;
  unsigned i;

  for (i=0; i<core_size; i++) {
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
   * Initialize the trace buffer
   */
  btrace_buf.resize(TRACE_BUF);
  for (auto &t: btrace_buf) {
    t.v = false;
  }
  trace_ptr = 0;

#ifdef TEST_GENERIC_SKIP
  test_generic_skip();
#endif
#ifdef TEST_GENERIC_GROUP_A
  test_generic_group_A();
#endif

  wrts = 0;
  wrt_addr[0] = wrt_addr[1] = 0;
  wrt_data[0] = wrt_data[1] = 0;
}

CPU::~CPU() {
}

void CPU::increment_p(uint16_t n) {
  uint16_t g = ((ea) ?
                ((p & 0x7fff) | (m & 0x8000)) :
                ((p & 0x3fff) | (m & 0xc000)));
  uint16_t d = g + n;

  p = (((ea) || (!ea_allowed)) ? d :
       ((d & 0xbfff) | (fetched_p & 0x4000)));
}

/*****************************************************************
 * Master clear
 *****************************************************************/
void CPU::master_clear() {
  a = 0;
  b = 0;
  x = 0xffff;
  p = 0;
  sc = 0x3f;
  j = 0;
  for (auto &p: prt) {
    p = false;
  }

  c = false;
  pi = pi_pending = false;
  ml = ml_pending = false;
  ea = ea_disable = pmi = false;
  dp = false;

  op = 0;
  m = 0;
  y = 0;

  start_button_interrupt = false;
  interrupts = 0;
  rtclk = false;
  dmc_req = 0;
  melov = false;

  run = false;
  fetched = false;

  melov_pending = false;
  prev_melov = false;
  dmc_cyc = false;
  dmc_dev = 0;
  fetched_p = 0;
  break_flag = false;
  break_intr = false;
  break_addr = 0;

  last_jmp_self_minus_one = jmp_self_minus_one = false;
}

bool CPU::optimize_io_poll(uint16_t instr) {
  bool r = false;

  if (last_jmp_self_minus_one) {

    /* Called from a polling IO instuction (INA, OTA, or SKS)
     * that was reached as a result of a 'JMP *-1' instruction
     * and which is not going to skip (the JMP *-1).
     * Unless something happens this is an infinite loop.
     *
     * Usually, however, some event will take place that will
     * either cause an interrupt, thus escaping from the
     * loop, or make the IO instruction skip.
     *
     * Two exceptions exist; the start-button interrupt,
     * and input from the ASR (where we are really waiting
     * for keyboard input).
     */

    if ( jump_time_to_event(half_cycles) ) {

      // rerun the IO command in the hope that
      // it will now skip the JMP *-1
      r = true;
        
    } else {

      io_polling(instr);

        
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
        
      if ((instr & 077) == static_cast<unsigned>(IoDevice::ASR)) {
        // Ought to wait for keyboard or GUI
        // printf("Polling ASR\n");
      } else {
        // Do something about apparent infinite
        // loop?
      }
    }

    //
    // If there is an interrupt now pending then
    // don't re-run the instruction, let the interrupt
    // be taken instead, similarly for a DMC break or
    // memory lockout violation
    //
    if ((pi && (interrupts || start_button_interrupt)) ||
        dmc_req || melov)
      r = 0;
  }

  return r;
}

/*****************************************************************
 * Add setting the carry bit to overflow
 * Weird, I know but that's the way it works on a series 16
 *****************************************************************/
int16_t CPU::short_add(int16_t a, int16_t m, bool &c) {
  long aa, mm, rr;
  int16_t r;

  aa = static_cast<signed long>(a);
  mm = static_cast<signed long>(m);
  rr = aa + mm;

  r = static_cast<int16_t>(rr);

  c = (rr != static_cast<signed long>(r));

  return r;
}

int16_t CPU::short_sub(int16_t a, int16_t m, bool &c) {
  long aa, mm, rr;
  int16_t r;

  aa = static_cast<signed long>(a);
  mm = static_cast<signed long>(m);
  rr = aa - mm;

  r = static_cast<int16_t>(rr);

  c = (rr != static_cast<signed long>(r));

  return r;
}

/*****************************************************************
 * Add, with the C bit added in
 *****************************************************************/
int16_t CPU::short_adc(int16_t a, int16_t m, bool &c) {
  int16_t r, v;

  v = ~(a ^ m);      // MSB is signs same
  r = a + m + c;
  v &= (r ^ m);      // if signs were same and now differ
  c = (v >> 15) & 1; // set overflow
  return r;
}

int32_t CPU::multiply(int16_t &ra, int16_t &rb, int16_t rm, int16_t &sc) {
  int m = rm;
  int a = ra;
  int b = rb;
  int lsc = -8;
  bool b17, madff=false;
  int g, h;
  int d = 0;
  int e = 0;
  int32_t p, q;

  if (m & 0x8000) m |= ((~0u) << 16);
  if (a & 0x8000) a |= ((~0u) << 16);

  q = a * m;

  b = a;
  b17 = 0;
  a = 0;

  g = h = 0;

  while (lsc < 0) {

    if (lsc != -8) {
      if (madff) {
        b = ((((d & 1) != 0) ? ((~0u) << 15) : 0) |
             ((a & 1) << 14) |
             ((e >> 2) & 0x3fff));
        b17 = ((e & 2) != 0);
        a = d >> 1;
      } else {
        b = ((((d & 2) != 0) ? ((~0u) << 15) : 0) |
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

unsigned CPU::divide(int16_t &ra, int16_t &rb, int16_t rm, int16_t &sc, bool &cbitf) {
  int d, e;
  bool azzzz, a00ff, m01ff, d01ff, remok;
  bool e00dj;

  int m = rm;
  int a = ra;
  int b = rb;

  bool c = true;
  int lsc = -17;
  unsigned count = 0;

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
          a |= ((~0u) << 16);
        b = ((e << 1) & 0xfffe) | e00dj;
        if ((b >> 15) & 1)
          b |= ((~0u) << 16);
      }

    } else if (lsc == -1) {

      a00ff = d01ff;
      a = d;
      if ((a >> 15) & 1)
        a |= ((~0u) << 16);

      if (remok && (!d01ff))
        madff = true;

      if (!c) {
        //a00ff = d01ff;
        //a = d;
        //if ((a >> 15) & 1)
        //  a |= (-1 << 16);
        b = ((e << 1) & 0xfffe) | e00dj;
        if ((b >> 15) & 1)
          b |= ((~0u) << 16);

        //if ((lsc == -1) && remok && (!d01ff))
        // madff = true;
      }

    } else { // lsc == 0

      a00ff = d01ff;

      if (remok) {
        a = e & 0xffff;
        if ((a >> 15) & 1)
          a |= ((~0u) << 16);
        b = d & 0xffff;
        if ((b >> 15) & 1)
          b |= ((~0u) << 16);
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
    a |= ((~0u) << 16);

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

/*****************************************************************
 * set the X register
 * Location "zero" in core tracks this, so the m and y registers
 * reflect this write.
 * With base sector relocation the J register holds the current
 * base sector. (J is normally zero)
 *****************************************************************/
void CPU::set_x(uint16_t n) {
  x = n;
  m = n;
  y = j;
  core[j] = n;
}

void CPU::set_just_x(uint16_t n) {
  x = n;
}

/*****************************************************************
 * Set or clear one bit of the "interrupts" variable
 * Used by the devices to generate or clear interrupt
 * conditions.
 *****************************************************************/
void CPU::set_interrupt(uint16_t bit) {
  interrupts |= bit;
}

void CPU::clear_interrupt(uint16_t bit) {
  interrupts &= (~bit);
}

void CPU::set_break(unsigned n, bool v) {
  if (n == 0) {
    rtclk = v;
  } else if (n <= 16) {
    uint16_t m = (1 << (n-1));
    if (v) {
      dmc_req |= m;
    } else {
      dmc_req &= ~m;
    }
  } else {
    std::cerr << "Unexpected set_break()" << std::endl;
    exit(1);
  }
}

/*****************************************************************
 * Front-panel memory access
 *****************************************************************/
void CPU::mem_access(bool p_not_pp1, bool store) {
  if (! p_not_pp1)
    y = y + 1;
  p = y;

  if (store)
    write(y, m);
  else
    m = read(y);

  fetched = false;
}

uint16_t CPU::e_a(uint16_t instr) { // Compute effective address
  uint16_t d;
  bool sec_zero;
  bool indirect;
  bool indexing;
  bool first = true;
  unsigned int indirect_count = 8;
  
  m = instr;
  y = fetched_p; // Address of instr (i.e. before increment)

  sec_zero = ((m & 0x0200) == 0);
  indexing = ((m & 0x4000) != 0);

  do {
    indirect = ((m & 0x8000) != 0);

    if (ml && (indirect_count == 0)) {
      melov = true;
      indirect = false;
    }
    
    if (sec_zero) {
      if (ea || (!ea_allowed))
        d = ((j & 0x7e00) | (m & 0x81ff));
      else
        d = ((j & 0x3e00) | (m & 0x81ff)) | (fetched_p & 0x4000);
    } else {
      if (first)
        d = ((y & 0xfe00) | (m & 0x01ff));
      else {
        if (ea || (!ea_allowed))
          d = m;
        else
          d = (m & 0xbfff) | (fetched_p & 0x4000);
      }
    }

    if ((indexing) &&  // Looks like indexing called for
        ((!ea) ||      // Can always do it in non-extended mode
         (!indirect))) // and when no more indirection
      d += x;

    if (ea || (!ea_allowed))
      y = d;
    else
      y = (d & 0xbfff) | (y & 0x4000);

    if (indirect) {
      half_cycles+=2;

      (void) read(y & 0x7fff); /* sets m */
      
      sec_zero = ((m & (ea ? 0x7e00 : 0x3e00) ) == 0);
      if (!ea) {
        indexing = ((m & 0x4000) != 0);
      }
      
      if (indirect_count > 0) {
        --indirect_count;
      }
    }

    first = false;

  } while (indirect);

  //printf("ea() y = %06o\n", y);
  return y;
}

void CPU::write(uint16_t addr, int16_t data) {
  bool prot = false;

  m = data;
  y = addr;

  uint16_t ma = addr & addr_mask;

  if (((addr ^ j) & ((ea) ? 0x7fff : 0x3fff)) == 0) {
    x = data;
  }

  if (ml) {
    if (! prt[ma >> 9]) {
      melov_pending = prot = true;
    }
  }

  if (((ma == 0) || (ma >= 020)) && (!prot)) {
    core[ma] = data;
    modified[ma] = 1;
    if (wrts < 2) {
      wrt_addr[wrts] = ma;
      wrt_data[wrts] = data;
      wrts++;
    }
  }

  //if (((addr >= 03000) && (addr < 03010)) || (addr == 0107))
  //  printf("Write %06o @ %06o\n", data & 0xffff, addr&0xffff );
}

int CPU::get_wrt_info(uint16_t addr[2], uint16_t data[2]) {
  int r = wrts;
  int i;
  for (i=0; i<2; i++) {
    addr[i] = (i < wrts) ? (wrt_addr[i] & addr_mask) : 0;
    data[i] = (i < wrts) ? wrt_data[i] : 0;
  }
  wrts = 0;
  return r;
}

std::string CPU::dis() {
  uint16_t instr = core[p];
  return instr_table.disassemble(p, instr, 0);
}

/******************************************************************/

void CPU::unimplemented(uint16_t instr [[maybe_unused]]) {
#if 0
  printf("Instr = `%06o at `%06o\n", instr, p);

  exit(1);
#endif
}

void CPU::do_CRA(uint16_t instr [[maybe_unused]]) {
  a = 0;
}

void CPU::do_IAB(uint16_t instr [[maybe_unused]]) {
  // In the real hardware the E register is used
  // to hold the B register.
  // This is also the "shift count" for the NRM instruction
  // so deliberately corrupt it here.
  sc = b;

  b = a;
  a = sc;
}

void CPU::do_IMA(uint16_t instr) {
  uint16_t yy;
  int16_t mm;
  half_cycles += 4;

  yy = e_a(instr);
  mm = read(yy);
  write(yy, a);
  a = mm;
  if (melov_pending) {
    melov = true; // Occurs immediately
  }
}

void CPU::do_INK(uint16_t instr [[maybe_unused]]) {
  a = ((c & 1) << 15) | ((dp & 1) << 14) | ((pmi & 1) << 13) |
    (sc & 0x3f); /* NB Programmers' Reference is wrong,
                  * 6 bits to A (not 5) from schematics */
}

void CPU::do_LDA(uint16_t instr) {
  unsigned addr;
  int16_t original_a = a;

  half_cycles+=2;

  addr = e_a(instr);

  if (dp) addr &= ((~0u)<<1); // lose the LSB if double precision

  a = read(addr);

  if (dp) {
    half_cycles+=2;
    addr |= 1;
    sc = original_a & 0x3f;
    b = read(addr);
  }
}

void CPU::do_LDX(uint16_t instr) {
  half_cycles += 4;
  write(((ea) ? j : ((j & 0x3fff) | (p & 0x4000))),
        read(e_a(instr & 0xbfff)));
  if (melov_pending) {
    x = read(((ea) ? j : ((j & 0x3fff) | (p & 0x4000))));
  }
}

void CPU::do_OTK(uint16_t instr [[maybe_unused]]) {
  half_cycles+=2;
  if (ml) {
    if (sks(instr) != IoStatus::WAIT) {
      increment_p();
    }
    melov = true;
  } else {
    c = (a & 0x8000) != 0;
    dp = (a & 0x4000) != 0;
    if ( (a & 0x2000) != 0 ) {
      ea_disable = false;
      ea = ea_allowed;
    } else {
      ea_disable = true;
    }
    sc = (a & 0x1f); /* Really is only 5 bits (schematics) */
  }
}

void CPU::do_STA(uint16_t instr) {
  unsigned addr;

  half_cycles+=2;

  addr = e_a(instr);

  write(addr, a);

  // if addr is odd, then this write will be
  // overwritten by the b register in the case
  // of double precision store (DST)

  if (dp) {
    sc = a & 0x3f;
    half_cycles+=2;
    addr |= 1;
    write(addr, b);
    if (melov_pending) {
      melov = true; // Occurs immediately
    }
  }
}

void CPU::do_STX(uint16_t instr) {
  unsigned addr;

  half_cycles+=2;

  addr = e_a(instr & 0xbfff);

  write(addr, x);
  if (melov_pending) {
    // Violating STX loads [ea] into X
    // (but doesn't change base sector location 0)
    x = read(addr);
  }
}

void CPU::do_ACA(uint16_t instr [[maybe_unused]]) {
  a = short_add(a, ((int16_t)(c & 1)), c);
}

void CPU::do_ADD(uint16_t instr) {
  int16_t original_a = a;

  half_cycles+=2;

  if (dp) {
    unsigned addr = e_a(instr);
    signed long da, dm, v;

    half_cycles+=2;

    if (dp) addr &= ((~0u)<<1); // lose the LSB
    dm = ((read(addr) & 0xffff) << 15) | (read(addr | 1) & 0x7fff);
    if (dm & 0x40000000)
      dm |= ((~0ul) << 31);

    da = ((a & 0xffff) << 15) | (b & 0x7fff);
    if (da & 0x40000000)
      da |= ((~0ul) << 31);

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
    a = short_add(a, read(e_a(instr)), c);
}

void CPU::do_AOA(uint16_t instr [[maybe_unused]]) {
  a = short_add(a, ((int16_t) 1), c);
}

void CPU::do_SUB(uint16_t instr) {
  int16_t original_a = a;

  half_cycles+=2;

  if (dp) {
    unsigned addr = e_a(instr);
    signed long da, dm, v;

    half_cycles+=2;

    if (dp) addr &= ((~0u)<<1); // lose the LSB
    dm = ((read(addr) & 0xffff) << 15) | (read(addr | 1) & 0x7fff);
    if (dm & 0x40000000)
      dm |= ((~0ul) << 31);

    da = ((a & 0xffff) << 15) | (b & 0x7fff);
    if (da & 0x40000000)
      da |= ((~0ul) << 31);

    dm = -dm;

    v = ~(da ^ dm);        // Bit 30 is signs same
    da += dm;
    v &= (da ^ dm);        // if signs were same and now differ

    a = (da >> 15) & 0xffff;
    b = da & 0x7fff;
    c = (v >> 30) & 1;

    sc = original_a & 0x3f;
  } else {
    a = short_sub(a, read(e_a(instr)), c);
  }
}

void CPU::do_TCA(uint16_t instr [[maybe_unused]])
{
  half_cycles += 1;
  // C remains unmodified
  bool v = c;
  a = short_sub(((int16_t)0), a, v);
}

void CPU::do_ANA(uint16_t instr) {
  half_cycles+=2;
  a &= read(e_a(instr));
}

void CPU::do_CSA(uint16_t instr [[maybe_unused]]) {
  c = (a & 0x8000) != 0;
  a &= 0x7fff;
}

void CPU::do_CHS(uint16_t instr [[maybe_unused]]) {
  a ^= 0x8000;
}

void CPU::do_CMA(uint16_t instr [[maybe_unused]]) {
  a = ~a;
}

void CPU::do_ERA(uint16_t instr) {
  half_cycles+=2;
  a ^= read(e_a(instr));
}

void CPU::do_SSM(uint16_t instr [[maybe_unused]]) {
  a |= 0x8000;
}

void CPU::do_SSP(uint16_t instr [[maybe_unused]]) {
  a &= 0x7fff;
}

int16_t CPU::ex_sc(uint16_t instr) {
  int16_t res = instr & 0x003f;

  if (res != 0)
    res |= (~0x003f); // Sign extend

  return res;
}

void CPU::do_ALR(uint16_t instr) {
  int16_t lsc;

  lsc = ex_sc(instr);

  while (lsc < 0) {
    half_cycles ++;
    c = (a >> 15) & 1;
    a = ((a & 0x7fff) << 1) | c;
    lsc++;
  }
  sc = b & 0x3f;
}

void CPU::do_ALS(uint16_t instr) {
  int16_t lsc;
  uint16_t d;

  lsc = ex_sc(instr);
  c = 0;
  while (lsc < 0) {
    half_cycles ++;
    d = ((a & 0x7fff) << 1);
    if ((a ^ d) & 0x8000)
      c = 1;
    a = d;
    lsc++;
  }
  sc = b & 0x3f;
}

void CPU::do_ARR(uint16_t instr) {
  int16_t lsc;

  lsc = ex_sc(instr);
  while (lsc < 0) {
    half_cycles ++;
    c = a & 1;
    a = ((a >> 1) & 0x7fff) | (c << 15);
    lsc++;
  }
  sc = b & 0x3f;
}

void CPU::do_ARS(uint16_t instr) {
  int16_t lsc;

  lsc = ex_sc(instr);
  while (lsc < 0) {
    half_cycles ++;
    c = a & 1;
    a = (a & 0x8000)|((a >> 1) & 0x7fff);
    lsc++;
  }
  sc = b & 0x3f;
}

void CPU::do_LGL(uint16_t instr) {
  int16_t lsc;

  lsc = ex_sc(instr);
  while (lsc < 0) {
    half_cycles ++;
    c = (a >> 15) & 1;
    a = (a << 1) & 0xfffe;
    lsc++;
  }
  sc = b & 0x3f;
}

void CPU::do_LGR(uint16_t instr) {
  int16_t lsc;

  lsc = ex_sc(instr);
  while (lsc < 0) {
    half_cycles ++;
    c = a & 1;
    a = ((a >> 1) & 0x7fff);
    lsc++;
  }
  sc = b & 0x3f;
}

void CPU::do_LLL(uint16_t instr) {
  int16_t lsc;

  lsc = ex_sc(instr);
  while (lsc < 0) {
    half_cycles ++;
    c = (a >> 15) & 1;
    a = ((a << 1) & 0xfffe) | ((b >> 15) & 1);
    b = (b << 1) & 0xfffe;
    lsc++;
  }
  sc = b & 0x3f;
}

void CPU::do_LLR(uint16_t instr) {
  int16_t lsc;

  lsc = ex_sc(instr);
  while (lsc < 0) {
    half_cycles ++;
    c = (a >> 15) & 1;
    a = ((a << 1) & 0xfffe) | ((b >> 15) & 1);
    b = ((b << 1) & 0xfffe) | c;
    lsc++;
  }
  sc = b & 0x3f;
}

void CPU::do_LLS(uint16_t instr) {
  int16_t lsc;
  int16_t d;
  lsc = ex_sc(instr);
  c = 0;
  while (lsc < 0) {
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

void CPU::do_LRL(uint16_t instr) {
  int16_t lsc;

  lsc = ex_sc(instr);
  while (lsc < 0) {
    half_cycles ++;
    c = b & 1;
    b = ((a & 1) << 15) | ((b >> 1) & 0x7fff);
    a = (a >> 1) & 0x7fff;
    lsc++;
  }
  sc = b & 0x3f;
}

void CPU::do_LRR(uint16_t instr) {
  int16_t lsc;

  lsc = ex_sc(instr);
  while (lsc < 0) {
    half_cycles ++;
    c = b & 1;
    b = ((a & 1) << 15) | ((b >> 1) & 0x7fff);
    a = (c << 15) | ((a >> 1) & 0x7fff);
    lsc++;
  }
  sc = b & 0x3f;
}

void CPU::do_LRS(uint16_t instr) {
  int16_t lsc;

  lsc = ex_sc(instr);
  while (lsc < 0) {
    half_cycles ++;
    c = b & 1;
    b = (b & 0x8000) | ((a & 1) << 14) | ((b >> 1) & 0x3fff);
    a = (a & 0x8000) | ((a >> 1) & 0x7fff);
    lsc++;
  }
  sc = b & 0x3f;
}

void CPU::do_INA(uint16_t instr) {
  int16_t d;
  half_cycles += 2;
  bool rerun = false;

  do {
    if (ina(instr, d) != IoStatus::WAIT) {
      increment_p();
      if (ml) {
        a = ~0;
      } else {
        if (instr & 01000)
          a = 0;
        a |= d;
      }
      rerun = false;
    } else {
      rerun = optimize_io_poll(instr);
    }
  } while (rerun);
  
  if (ml) {
    melov = true;
  }
}

void CPU::do_OCP(uint16_t instr [[maybe_unused]]) {
  half_cycles+=2;
  if (ml) {
    if (sks(instr) != IoStatus::WAIT) {
      increment_p();
    }
    melov = true;
  } else {
    ocp(instr);
  }
}

void CPU::do_OTA(uint16_t instr [[maybe_unused]]) {
  bool rerun = false;
  half_cycles+=2;

  do {
    if (((ml) ? sks(instr) : ota(instr, a)) != IoStatus::WAIT) {
      increment_p();
      rerun = false;
    } else {
      rerun = optimize_io_poll(instr);
    }
  } while (rerun);
  
  if (ml) {
    melov = true;
  }
}

void CPU::do_SMK(uint16_t instr) {
  half_cycles+=2;
  if (ml) {
    /*
     * The PRM claims that in memory locakout mode all I/O instructions
     * (including SMK) behave like SKS. However, this is incorrect since
     * the prevention of the skip based on decoding the device address
     * is not affected by being in ML mode. SMK therefore behaves
     * like a NOP.
     */
    melov = true;
  } else {
    unsigned int function = (instr >> 6) & 0x0f;
    if ((instr & 0x3f) == 020) {
      switch (function) {
      case 000: /* Standard interrupts      */
        smk(a);
        break;
      case 001: /* Priority Interrupt  1-16 */ break;
      case 002: /* Priority Interrupt 17-32 */ break;
      case 003: /* Priority Interrupt 33-48 */ break;
      case 004: /* Single line controller   */ break;

      case 013: j = a & 0x7e00;  break;
      case 014: write_prt(0, a); break;
      case 015: write_prt(1, a); break;
      case 016: write_prt(2, a); break;
      case 017: write_prt(3, a); break;
      default: /* Do nothing */  break;
      }
    } else if ((instr & 0x3f) == 024) {

    }
  }
}

void CPU::do_SKS(uint16_t instr [[maybe_unused]]) {
  bool rerun = 0;
  half_cycles+=2;

  do {
    if (sks(instr) != IoStatus::WAIT) {
      increment_p();
      rerun = 0;
    } else
      rerun = optimize_io_poll(instr);
  } while (rerun);

  if (ml) {
    melov = true;
  }
}

void CPU::do_CAS(uint16_t instr) {
  int16_t mm;
  half_cycles += 4;
  mm = read(e_a(instr));
  if (a == mm)
    p += 1;
  else if (a < mm)
    p += 2;
}

void CPU::do_ENB(uint16_t instr [[maybe_unused]]) {
  pi_pending = true;
}

void CPU::do_HLT(uint16_t instr [[maybe_unused]]) {
  if (ml) {
    melov_pending = true;
  } else {
    
    run = false;
  }
  ++half_cycles; // 1.5 cycle instruction
}

void CPU::do_INH(uint16_t instr [[maybe_unused]]) {
  if (ml) {
    melov_pending = true;
  }
  // Even if in memory lockout mode still treated as an inhibit
  pi_pending = pi = false;
}

void CPU::do_IRS(uint16_t instr) {
  uint16_t yy;
  int16_t d;
  half_cycles += 4;
  yy = e_a(instr);
  d = read(yy) + 1;
  write(yy, d);
  if (break_flag) {
    melov_pending = false; // Ignore memory lockout mode
  } else {
    increment_p((d==0) ? 1 : 0);
    if (melov_pending) {
      melov = true; // Occurs immediately
    }
  }
  if ((d == 0) && break_flag && (break_addr = 061)) {
    event(IoDevice::RTC,
          static_cast<unsigned>(RTC::Event::ROLLOVER));
  }
}

void CPU::do_JMP(uint16_t instr) {
  uint16_t new_p;

  new_p = e_a(instr);

  if ((new_p & addr_mask) == ((fetched_p-1) & addr_mask))
    jmp_self_minus_one = true;

  p = (((ea) || (!ea_allowed)) ? new_p :
       ((new_p & 0xbfff) | (fetched_p & 0x4000)));

  if (ea_disable)
    ea = ea_disable = false;
}

void CPU::do_JST(uint16_t instr) {
  uint16_t yy, return_addr;
  half_cycles += 4;
  yy = e_a(instr);

  /*
   * Operation depends on extend mode.
   * Store 14 or 15 bit address as appropriate
   * leave upper bit(s) alone
   */

  if (ea)
    return_addr = (read(yy) & 0x8000) | (p & 0x7fff);
  else
    return_addr = (read(yy) & 0xc000) | (p & 0x3fff);

  write(yy, return_addr);
  p = yy+1;
  if (break_flag) {
    melov_pending = false; // Ignore memory lockout mode
  } else {
    if (melov_pending) {
      melov = true; // Occurs immediately
    }
  }
}

void CPU::do_NOP(uint16_t instr [[maybe_unused]]) {
}

void CPU::do_RCB(uint16_t instr [[maybe_unused]]) {
  c = 0;
}

void CPU::do_SCB(uint16_t instr [[maybe_unused]]) {
  c = 1;
}

void CPU::do_SKP(uint16_t instr [[maybe_unused]]) {
  increment_p();
}

void CPU::do_SLN(uint16_t instr [[maybe_unused]]) {
  if (a & 0x0001)
    increment_p();
}

void CPU::do_SLZ(uint16_t instr [[maybe_unused]]) {
  if ((a & 0x0001) == 0)
    increment_p();
}

void CPU::do_SMI(uint16_t instr [[maybe_unused]]) {
  if (a<0)
    increment_p();
}

void CPU::do_SNZ(uint16_t instr [[maybe_unused]]) {
  if (a != 0)
    increment_p();
}

void CPU::do_SPL(uint16_t instr [[maybe_unused]]) {
  if (a >= 0)
    increment_p();
}

void CPU::do_SR1(uint16_t instr [[maybe_unused]]) {
  if (!ss[0])
    increment_p();
}

void CPU::do_SR2(uint16_t instr [[maybe_unused]]) {
  if (!ss[1])
    increment_p();
}

void CPU::do_SR3(uint16_t instr [[maybe_unused]]) {
  if (!ss[2])
    increment_p();
}

void CPU::do_SR4(uint16_t instr [[maybe_unused]]) {
  if (!ss[3])
    increment_p();
}

void CPU::do_SRC(uint16_t instr [[maybe_unused]]) {
  if (!c)
    increment_p();
}

void CPU::do_SS1(uint16_t instr [[maybe_unused]]) {
  if (ss[0])
    increment_p();
}

void CPU::do_SS2(uint16_t instr [[maybe_unused]]) {
  if (ss[1])
    increment_p();
}

void CPU::do_SS3(uint16_t instr [[maybe_unused]]) {
  if (ss[2])
    increment_p();
}

void CPU::do_SS4(uint16_t instr [[maybe_unused]]) {
  if (ss[3])
    increment_p();
}

void CPU::do_SSC(uint16_t instr [[maybe_unused]]) {
  if (c)
    increment_p();
}

void CPU::do_SSR(uint16_t instr [[maybe_unused]]) {
  if ((!ss[0]) && (!ss[1]) && (!ss[2]) && (!ss[3]))
    increment_p();
}

void CPU::do_SSS(uint16_t instr [[maybe_unused]]) {
  if (ss[0] || ss[1] || ss[2] || ss[3])
    increment_p();
}

void CPU::do_SZE(uint16_t instr [[maybe_unused]]) {
  if (a==0)
    increment_p();
}

void CPU::do_CAL(uint16_t instr [[maybe_unused]]) {
  a &= 0x00ff;
}

void CPU::do_CAR(uint16_t instr [[maybe_unused]]) {
  a &= 0xff00;
}

void CPU::do_ICA(uint16_t instr [[maybe_unused]]) {
  a = ((a << 8) & 0xff00) | ((a >> 8) & 0x00ff);
}

void CPU::do_ICL(uint16_t instr [[maybe_unused]]) {
  a = ((a >> 8) & 0x00ff);
}

void CPU::do_ICR(uint16_t instr [[maybe_unused]]) {
  a = ((a << 8) & 0xff00);
}

#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
/*
 * NPL group A
 */

void CPU::do_ad1(uint16_t instr [[maybe_unused]]) {
  a = a + 1;
}

void CPU::do_ad1_15(uint16_t instr [[maybe_unused]]) {
  a = a + 1;
  half_cycles++;
}

void CPU::do_adc(uint16_t instr [[maybe_unused]]) {
  a = a + (c & 1);
}

void CPU::do_adc_15(uint16_t instr [[maybe_unused]]) {
  a = a + (c & 1);
  half_cycles++;
}

void CPU::do_cm1(uint16_t instr [[maybe_unused]]) {
  a = (-1) + (c & 1);
}

void CPU::do_ltr(uint16_t instr [[maybe_unused]]) {
  a = (a & 0xff00) | ((a >> 8) & 0xff);
}

void CPU::do_btr(uint16_t instr [[maybe_unused]]) {
  a |= ((a >> 8) & 0xff);
}

void CPU::do_btl(uint16_t instr [[maybe_unused]]) {
  a |= ((a << 8) & 0xff00);
}

void CPU::do_rtl(uint16_t instr [[maybe_unused]]) {
  a = ((a << 8) & 0xff00) | (a & 0xff);
}

void CPU::do_rcb_ssp(uint16_t instr [[maybe_unused]]) {
  c = 0;
  a &= 0x7fff;
}

void CPU::do_cpy(uint16_t instr [[maybe_unused]]) {
  c = (a >> 15) & 1;
}

void CPU::do_btb(uint16_t instr [[maybe_unused]]) {
  a = ((a | (a << 8)) & 0xff00) | ((a | (a >> 8)) & 0xff);
}

void CPU::do_bcl(uint16_t instr [[maybe_unused]]) {
  a = (( a | (a >> 8)) & 0xff);
}

void CPU::do_bcr(uint16_t instr [[maybe_unused]]) {
  a = (( a | (a << 8)) & 0xff00);
}

void CPU::do_ld1(uint16_t instr [[maybe_unused]]) {
  a = 1;
  half_cycles ++;
}

void CPU::do_isg(uint16_t instr [[maybe_unused]]) {
  a = ((c & 1) << 1) - 1;
  half_cycles ++;
}

void CPU::do_cma_aca(uint16_t instr [[maybe_unused]]) {
  a = (~a) + (c & 1);
  half_cycles ++;
}

void CPU::do_cma_aca_c(uint16_t instr [[maybe_unused]]) {
  a = (~a) + (c & 1);
  c |= (a >> 15) & 1;

  half_cycles ++;
}

void CPU::do_a2a(uint16_t instr [[maybe_unused]]) {
  a = a + 2;
  half_cycles ++;
}

void CPU::do_a2c(uint16_t instr [[maybe_unused]]) {
  a = a + ((c & 1) << 1);
  half_cycles++;
}

void CPU::do_ics(uint16_t instr [[maybe_unused]]) {
  a = (a & 0x8000) | ((a >> 8) & 0xff);
}

void CPU::do_scb_a2a(uint16_t instr [[maybe_unused]]) {
  c = 1;
  a = a + 2;
  half_cycles++;
}

void CPU::do_scb_aoa(uint16_t instr [[maybe_unused]])
{
  c = 1;
  a = a + 1;
  half_cycles++;
}

void CPU::do_a2c_scb(uint16_t instr [[maybe_unused]]) {
  a = a + ((c & 1) << 1);
  c = 1;
  half_cycles++;
}

void CPU::do_aca_scb(uint16_t instr [[maybe_unused]]) {
  a = a + (c & 1);
  c = 1;
  half_cycles++;
}

void CPU::do_icr_scb(uint16_t instr [[maybe_unused]]) {
  a = ((a << 8) & 0xff00);
  c = 1;
}

void CPU::do_rtl_scb(uint16_t instr [[maybe_unused]])
{
  a = ((a << 8) & 0xff00) | (a & 0xff);
  c = 1;
}

void CPU::do_btb_scb(uint16_t instr [[maybe_unused]]) {
  a = ((a | (a << 8)) & 0xff00) | ((a | (a >> 8)) & 0xff);
  c = 1;
}

void CPU::do_noa(uint16_t instr [[maybe_unused]]) {
  /* Do Nothing */
}

#endif

/*
 * Extended addressing
 */

void CPU::do_DXA(uint16_t instr [[maybe_unused]]) {
  ea_disable = true;
}

void CPU::do_EXA(uint16_t instr [[maybe_unused]]) {
  ea_disable = false;
  ea = ea_allowed;
}

void CPU::do_ERM(uint16_t instr [[maybe_unused]]) {
  pi_pending = true;
  ml_pending = true;
}

/*
 * Memory parity option
 */
void CPU::do_RMP(uint16_t instr [[maybe_unused]]) {
}

/*
 * High speed arithmetic option
 */

void CPU::do_DBL(uint16_t instr [[maybe_unused]]) {
  dp = 1;
}

void CPU::do_DIV(uint16_t instr) {
  const int16_t rm = (int16_t) read(e_a(instr));

  half_cycles += divide(a, b, rm, sc, c);
}

void CPU::do_MPY(uint16_t instr) {
  const int16_t rm = (int16_t) read(e_a(instr));

  half_cycles += 9;

  (void) multiply(a, b, rm, sc);
}

void CPU::do_NRM(uint16_t instr [[maybe_unused]]) {
  int16_t d;

  sc = 0;
  //c = 0; does it clear C?

  while (( (a & 0x8000) == ((a & 0x4000) << 1)) && (sc < 32)) {
    half_cycles ++;
    d = ((a << 1) & 0xfffe) | ((b >> 14) & 1);
    a = d;
    b = (b & 0x8000) | ((b << 1) & 0x7ffe);
    sc++;
  }

  if (sc == 32)
    sc = 0;
}

void CPU::do_SCA(uint16_t instr [[maybe_unused]]) {
  a = sc & 0x3f;
}

void CPU::do_SGL(uint16_t instr [[maybe_unused]]) {
  dp = 0;
}

void CPU::do_iab_sca(uint16_t instr [[maybe_unused]]) {
  int16_t d;

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

void CPU::generic_shift(uint16_t instr) {
  int cccc = (instr >> 6) & 0x0f;
  int16_t d;
  int16_t lsc;

  // XXXXX - this was missing XXXXXXXXXX
  if ((cccc == 007) ||
      (cccc == 017))
    c = 0;
  // But is it for all shifts? Or just those that
  // set C by overflow? Matters if SC == 0. But can it be?
  // is zero lots?

  lsc = ex_sc(instr);
  while (lsc < 0) {
    half_cycles ++;

    switch (cccc) {
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


void CPU::generic_skip(uint16_t instr) {
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

  if (instr & 0x001) cond &= ((!c) & 1);
  if (instr & 0x002) cond &= ((!ss[3]) & 1);
  if (instr & 0x004) cond &= ((!ss[2]) & 1);
  if (instr & 0x008) cond &= ((!ss[1]) & 1);
  if (instr & 0x010) cond &= ((!ss[0]) & 1);
  if (instr & 0x020) cond &= ((a == 0) & 1);
  if (instr & 0x040) cond &= ((~a) & 1);
  if (instr & 0x080) cond &= 1;
  if (instr & 0x100) cond &= (((~a) >> 15) & 1);

  if (instr & 0x200) cond ^= 1;

  if (cond) increment_p();
}

#ifdef TEST_GENERIC_SKIP
void CPU::test_generic_skip() {
  uint16_t i;
  int j;

  for (i=0x8000; i<0x8400; i++) {
    if (instr_table.dispatch(i) != &CPU::generic_skip) {
      for (j=0; j<256; j++) {
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
        (this->*instr_table.dispatch(i))(i);
        generic_skip(i);
        if ((p != 0) && (p != 2)) {
          fprintf(stderr, "%s: '%06o j=0x%02x p=%d, a=0x%04x, c=%d\n",
                  __PRETTY_FUNCTION__, i, j, p, a, c);
          exit (1);
        }
      }
    }
  }
}
#endif

void CPU::generic_group_A(uint16_t instr) {
  // first break the instruction into bits
  bool M[17];
  int i;

  for (i=0; i<16; i++)
    M[16-i] = (instr >> i) & 1;
  M[0] = false; // (unused)

  bool azzzz = false;

  bool EASTL, EASBM, JAMKN, EIK17;
  int16_t s1, s2, s=0;
  bool v;
  int d;
  bool CLATR, CLA1R, EDAHS, EDALS, ETAHS, ETALS, EDA1R;
  bool overflow_to_c, set_c, d1_to_c;

  do {
    // T2
    if (azzzz) {
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

    if (JAMKN) {
      s = s1 ^ s2;
      v=0;
      (void) short_adc((s1 & 0x8000), (s2 & 0x8000), v);
    } else {
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
void CPU::test_generic_group_A() {
  uint16_t i;
  int j, k;
  uint16_t test_data[] = {
    0x0000, 0xffff, 0x5555, 0xaaaa,
    0x5aa5, 0xa55a, 0x137f, 0xfec8,
    0x8000, 0};
  int16_t sav_a;
  bool sav_c;
  unsigned long sav_h;
  bool problem;

  for (i=0xc000; i<0xc400; i++) {
    if (instr_table.dispatch(i) != &CPU::generic_group_A) {
      // test one instruction
      for (j=0; ((j==0)|test_data[j]); j++) {
        for (k=0; k<2; k++) {
          a = test_data[j];
          c = k;
          half_cycles = 0;

          (this->*instr_table.dispatch(i))(i);

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

          if (problem) {
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

void CPU::write_prt(unsigned int n, uint16_t v) {
  int i;

  for (i=0; i<16; i++) {
    prt[(16*n)+i] = (v >> (15-i)) & 1;
  }
}


/*****************************************************************
 * This is where the action happens!
 *
 *****************************************************************/
bool CPU::do_instr() {
  uint16_t instr;
  uint16_t dmc_addr=0;
  int16_t dmc_data=0;
  bool dmc_erl=false;

  run = true; // Might be cleared in do_HLT()

  /*
   * fetched records whether or not an instruction
   * has already been fetched (into the M register)
   */
  if (fetched) {

    /*
     * Yes, it has been fetched
     * Normal execution sequence...
     */

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

      if ((!break_intr) && (break_addr == 061)) {
        rtclk = false;
      }
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
      uint16_t tmp_addr;

      // DMC cycle 1
      dmc_addr     = read(break_addr);
      tmp_addr     = dmc_addr;
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
        dmc(dmc_dev, dmc_data, dmc_erl);
        //cout << "DMC " << dec << dmc_dev << " write " << oct << dmc_data << " to "
        //     << oct << break_addr << endl;
        write(break_addr, dmc_data);
      } else {
        // Do an output
        dmc_data = read(break_addr);
        dmc(dmc_dev, dmc_data, dmc_erl);
      }
      break_addr   = 000020 + (dmc_dev * 2);
      half_cycles += 2;

      // DMC cycle 4
      write(break_addr, dmc_addr);
      dmc_cyc      = false;
      half_cycles += 2;

      // Restore original dmc_addr for tracing, below
      dmc_addr = tmp_addr;
    } else {

      last_jmp_self_minus_one = jmp_self_minus_one;
      jmp_self_minus_one = false;
      prev_melov = melov;
      melov = melov_pending;
      melov_pending = false;

      /*
       * Now simply jump to the routine that handles
       * this instruction.
       */
      (this->*instr_table.dispatch(instr))(instr);
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

    if ((break_flag) && (instr < 16))  {
      // DMC break
      btrace_buf[trace_ptr].y = dmc_addr;
      btrace_buf[trace_ptr].p = dmc_data;
      btrace_buf[trace_ptr].c = dmc_erl;
    }

    trace_ptr = (trace_ptr + 1) % TRACE_BUF;
  } else {
    p = y; /* Front panel updates Y not P
            * So copy Y into P before fetching */
  }

  op = (c << 8) | (pi << 7) | (ml << 5) | (ea << 4) | (dp << 3);

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

    pi = pi_pending = false; // disable interrupts
    ea = ea_allowed; // force extended addressing
    melov = melov_pending = false; // Drop MLO violation
  } if (melov) {
    break_flag = true;
    break_intr = true;
    break_addr = 062;

    pi = pi_pending = false; // disable interrupts
    pmi = ea; // Previous mode indicator
    ea = ea_allowed; // force extended addressing
    melov = melov_pending = false;
    ml = ml_pending = false;
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
    if (ml_pending && (!ml) && (!dmc_cyc)) {
      ml = true;
      ml_pending = false;
    }
  }
  
  fetched = true;
  return run; // pass back the run status
}

void CPU::unwind_instr() {
  /*
   * unwind state changes in do_instr() before the call vi
   * instr_table.dispatch if it throws an exception.
   */
  melov_pending = melov;
  melov = prev_melov;
  jmp_self_minus_one = last_jmp_self_minus_one;
  p = fetched_p;
}
