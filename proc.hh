/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005, 2010, 2011  Adrian Wise
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

#include "instr.hh" // To get GENERIC_GROUP_A

class STDTTY;
class InstrTable;
class IODEV;
struct Btrace;
struct FP_INTF;

class Proc{

public:
  Proc(STDTTY *stdtty, bool HasEa);

  void mem_access(bool p_not_pp1, bool store);
  void do_instr(bool &run, bool &monitor_flag);
  unsigned short ea(unsigned short instr);

  void master_clear(); // do what the master-clear button does

  struct FP_INTF *fp_intf();
  
  /*
   * Interface routines to read and write memory
   *
   * writing to location "zero" (possibly relocated)
   * also changes the X register.
   */
  void write(unsigned short addr, signed short data);
  unsigned short read(unsigned short addr)
  {
    y = addr;
    m = core[addr & addr_mask];
    return m;
  };
  
  /*
   * Get and set registers - used by monitor
   *
   * Writing to X changes location "zero"
   */
  void set_py(unsigned short n) {p = y = n;};
  unsigned short get_p(void) {return p;};
  void set_a(unsigned short n) {a = n;};
  unsigned short get_a(void) {return a;};
  void set_b(unsigned short n) {b = n;};
  unsigned short get_b(void) {return b;};
  void set_x(unsigned short n);
  void set_just_x(unsigned short n);
  unsigned short get_x(void) {return x;};
  
  bool get_c() {return c;};
  bool get_ex() {return extend;};
  bool get_dp() {return dp;};

  unsigned short get_sc() {return sc & 0x3f;};

  /* 
   * Get and set sense switches
   */
  bool get_ss(int sw) {return ss[sw];};
  void set_ss(int sw, bool v) {ss[sw] = v;};
  
  unsigned long long get_half_cycles(void){return half_cycles;};

  void set_interrupt(unsigned short bit);
  void clear_interrupt(unsigned short bit);
  void set_rtclk(bool v);

  void dump_memory();
  void dump_trace(const char *filename, int n);
  void dump_disassemble(char *filename, int first, int last);
  void dump_vmem(char *filename, int exec_addr, bool octal=false);
  void dump_coemem(char *filename, int exec_addr);
  
  void start_button();
  void goto_monitor();
  
  const char *dis();
  void flush_events();
  void set_ptr_filename(char *filename);
  void set_ptp_filename(char *filename);
  void set_plt_filename(char *filename);
  void set_lpt_filename(char *filename);
  void set_asr_ptr_filename(char *filename);
  void asr_ptr_on(char *filename);
  void set_asr_ptp_filename(char *filename);
  void asr_ptp_on(char *filename);
  bool special(char k);

  bool optimize_io_poll(unsigned short instr);

  void abort();

  void sampled_io(bool x_drlin, unsigned short x_inb){
    drlin=x_drlin; inb=x_inb;};

  int get_wrt_info(unsigned short addr[2], unsigned short data[2]);

private:
  /*
   * the following are the machine registers
   */
  signed short a;
  signed short b;
  signed short x;
  signed short m;
  signed short op; // doesn't do anything!
  unsigned short p;
  unsigned short y; // Address register
  unsigned short j; // base sector relocation

  // sampled I/O values
  bool drlin;
  unsigned short inb;

  // Memory write testing
  int wrts;
  unsigned short wrt_addr[2];
  unsigned short wrt_data[2];

  /*
   * Various flags
   */
  bool pi, pi_pending;
  unsigned short interrupts;
  bool start_button_interrupt;
  bool rtclk;
  bool break_flag;
  bool break_intr;
  unsigned int break_addr;

  bool goto_monitor_flag;
  bool jmp_self_minus_one;
  bool last_jmp_self_minus_one;

  bool c;
  bool dp;
  bool extend;
  bool disable_extend_pending;
  const bool extend_allowed; // i.e. whether CPU has extended addressing
  const unsigned short addr_mask;

  signed short sc; // shift count

  /*
   * sense switches
   */
  int /*bool*/ ss[4];

  /*
   * the core memory 
   */
  signed short *core;
  bool *modified;

  bool run;     // flag to say we're still running
  bool fetched; // flag to say that an instruction has been fetched
  unsigned short fetched_p;

  unsigned long long half_cycles;

  /*
   * Instruction decode & dispatch
   */
  InstrTable instr_table;

  /*
   * devices
   */
  IODEV **devices;

  /*
   * Tracing
   */
  int trace_ptr;
  struct Btrace *btrace_buf;

  void increment_p(unsigned short n = 1);

  /* These routines are public in order that the instruction
     tables can refer to them. */
public:
  void unimplemented(unsigned short instr);

  void do_CRA(unsigned short instr);
  void do_IAB(unsigned short instr);
  void do_IMA(unsigned short instr);
  void do_INK(unsigned short instr);
  void do_LDA(unsigned short instr);
  void do_LDX(unsigned short instr);
  void do_OTK(unsigned short instr);
  void do_STA(unsigned short instr);
  void do_STX(unsigned short instr);
  void do_ACA(unsigned short instr);
  void do_ADD(unsigned short instr);
  void do_AOA(unsigned short instr);
  void do_SUB(unsigned short instr);
  void do_TCA(unsigned short instr);
  void do_ANA(unsigned short instr);
  void do_CSA(unsigned short instr);
  void do_CHS(unsigned short instr);
  void do_CMA(unsigned short instr);
  void do_ERA(unsigned short instr);
  void do_SSM(unsigned short instr);
  void do_SSP(unsigned short instr);
  void do_ALR(unsigned short instr);
  void do_ALS(unsigned short instr);
  void do_ARR(unsigned short instr);
  void do_ARS(unsigned short instr);
  void do_LGL(unsigned short instr);
  void do_LGR(unsigned short instr);
  void do_LLL(unsigned short instr);
  void do_LLR(unsigned short instr);
  void do_LLS(unsigned short instr);
  void do_LRL(unsigned short instr);
  void do_LRR(unsigned short instr);
  void do_LRS(unsigned short instr);
  void do_INA(unsigned short instr);
  void do_OCP(unsigned short instr);
  void do_OTA(unsigned short instr);
  void do_SMK(unsigned short instr);
  void do_SKS(unsigned short instr);
  void do_CAS(unsigned short instr);
  void do_ENB(unsigned short instr);
  void do_HLT(unsigned short instr);
  void do_INH(unsigned short instr);
  void do_IRS(unsigned short instr);
  void do_JMP(unsigned short instr);
  void do_JST(unsigned short instr);
  void do_NOP(unsigned short instr);
  void do_RCB(unsigned short instr);
  void do_SCB(unsigned short instr);
  void do_SKP(unsigned short instr);
  void do_SLN(unsigned short instr);
  void do_SLZ(unsigned short instr);
  void do_SMI(unsigned short instr);
  void do_SNZ(unsigned short instr);
  void do_SPL(unsigned short instr);
  void do_SR1(unsigned short instr);
  void do_SR2(unsigned short instr);
  void do_SR3(unsigned short instr);
  void do_SR4(unsigned short instr);
  void do_SRC(unsigned short instr);
  void do_SS1(unsigned short instr);
  void do_SS2(unsigned short instr);
  void do_SS3(unsigned short instr);
  void do_SS4(unsigned short instr);
  void do_SSC(unsigned short instr);
  void do_SSR(unsigned short instr);
  void do_SSS(unsigned short instr);
  void do_SZE(unsigned short instr);
  void do_CAL(unsigned short instr);
  void do_CAR(unsigned short instr);
  void do_ICA(unsigned short instr);
  void do_ICL(unsigned short instr);
  void do_ICR(unsigned short instr);

#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
  /*
   * NPL group A
   */
  void do_ad1(unsigned short instr);
  void do_ad1_15(unsigned short instr);
  void do_adc(unsigned short instr);
  void do_adc_15(unsigned short instr);
  void do_cm1(unsigned short instr);
  void do_ltr(unsigned short instr);
  void do_btr(unsigned short instr);
  void do_btl(unsigned short instr);
  void do_rtl(unsigned short instr);
  void do_rcb_ssp(unsigned short instr);
  void do_cpy(unsigned short instr);
  void do_btb(unsigned short instr);
  void do_bcl(unsigned short instr);
  void do_bcr(unsigned short instr);
  void do_ld1(unsigned short instr);
  void do_isg(unsigned short instr);
  void do_cma_aca(unsigned short instr);
  void do_cma_aca_c(unsigned short instr);
  void do_a2a(unsigned short instr);
  void do_a2c(unsigned short instr);
  void do_ics(unsigned short instr);
  void do_scb_a2a(unsigned short instr);
  void do_scb_aoa(unsigned short instr);
  void do_a2c_scb(unsigned short instr);
  void do_aca_scb(unsigned short instr);
  void do_icr_scb(unsigned short instr);
  void do_rtl_scb(unsigned short instr);
  void do_btb_scb(unsigned short instr);
  void do_noa(unsigned short instr);
#endif

  void do_EXA(unsigned short instr);
  void do_DXA(unsigned short instr);

  void do_RMP(unsigned short instr);

  void do_DBL(unsigned short instr);
  void do_DIV(unsigned short instr);
  void do_MPY(unsigned short instr);
  void do_NRM(unsigned short instr);
  void do_SCA(unsigned short instr);
  void do_SGL(unsigned short instr);
  void do_iab_sca(unsigned short instr);

  void generic_shift(unsigned short instr);
  void generic_skip(unsigned short instr);
#ifdef TEST_GENERIC_SKIP
  void test_generic_skip();
#endif
  void generic_group_A(unsigned short instr);
#ifdef TEST_GENERIC_GROUP_A
  void test_generic_group_A();
#endif

  static signed short ex_sc(unsigned short instr);

};

/* Bodge around missing "long long" support with mingw */
#ifndef PRIuLL
#if defined(_WIN32) || defined(__WIN32__)
#define PRIuLL "%l64u"
#else
#define PRIuLL "%llu"
#endif
#endif
