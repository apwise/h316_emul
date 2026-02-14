/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005, 2010, 2011, 2026  Adrian Wise
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

#ifndef _PROC_HPP_
#define _PROC_HPP_

#include <vector>
#include <cstdint>
#include "instr.hpp" // To get GENERIC_GROUP_A
#include "event_queue.hpp"

class STDTTY;
class InstrTable;
class IODEV;
struct Btrace;
struct FP_INTF;
class MFM;

class Proc{

public:
  Proc(STDTTY *stdtty, bool HasEa);
  ~Proc();

  void exit(int code);
  bool get_exit_called(int &code){code = exit_code; return exit_called;}

  void mem_access(bool p_not_pp1, bool store);
  void do_instr(bool &run, bool &monitor_flag);
  uint16_t ea(uint16_t instr);

  void master_clear(); // do what the master-clear button does

  struct FP_INTF *fp_intf();
  
  /*
   * Interface routines to read and write memory
   *
   * writing to location "zero" (possibly relocated)
   * also changes the X register.
   */
  void write(uint16_t addr, int16_t data);
  uint16_t read(uint16_t addr)
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
  void set_py(uint16_t n) {p = y = n;}
  uint16_t get_p(void) {return p;}
  void set_a(uint16_t n) {a = n;}
  uint16_t get_a(void) {return a;}
  void set_b(uint16_t n) {b = n;}
  uint16_t get_b(void) {return b;}
  void set_x(uint16_t n);
  void set_just_x(uint16_t n);
  uint16_t get_x(void) {return x;}
  
  bool get_c() {return c;}
  bool get_ex() {return extend;}
  bool get_dp() {return dp;}

  uint16_t get_sc() {return sc & 0x3f;}

  /* 
   * Get and set sense switches
   */
  bool get_ss(int sw) {return ss[sw];}
  void set_ss(int sw, bool v) {ss[sw] = v;}
  
  uint64_t get_half_cycles(void){return half_cycles;}

  void set_interrupt(uint16_t bit);
  void clear_interrupt(uint16_t bit);
  void set_rtclk(bool v);

  void set_dmcreq(uint16_t bit);
  unsigned int get_dmc_dev(){return dmc_dev;}
  
  void dump_memory();
  void dump_trace(const char *filename, int n);
  void dump_disassemble(char *filename, int first, int last);
  void dump_vmem(char *filename, int exec_addr, bool octal=false);
  void dump_coemem(char *filename, int exec_addr);
  
  void start_button();
  void goto_monitor();
  void set_limit(uint64_t half_cycles);
  void set_sbi(uint64_t half_cycles);
  
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

  bool optimize_io_poll(uint16_t instr);

  void abort();

  void sampled_io(bool x_drlin, uint16_t x_inb){
    drlin=x_drlin; inb=x_inb;}

  int get_wrt_info(uint16_t addr[2], uint16_t data[2]);

  void queue(unsigned long microseconds, IODEV *device, int reason)
  {
#ifndef RTL_SIM
    EventQueue::EventTime event_time = (half_cycles +
                                        (half_cycles_per_microsecond * microseconds));
    event_queue.queue(event_time, device, reason);
#endif
  }
  
  void queue_hc(uint64_t half_cycles, IODEV *device, int reason)
  {
#ifndef RTL_SIM
    EventQueue::EventTime event_time = this->half_cycles + half_cycles;
    event_queue.queue(event_time, device, reason);
#endif
  }
  void event(int reason);
  
private:
  MFM *mfm;

  /*
   * the following are the machine registers
   */
  int16_t a;
  int16_t b;
  int16_t x;
  int16_t m;
  int16_t op;
  uint16_t p;
  uint16_t y; // Address register
  uint16_t j; // base sector relocation
  std::vector<bool> prt;
  
  // sampled I/O values
  bool drlin;
  uint16_t inb;

  // Memory write testing
  int wrts;
  uint16_t wrt_addr[2];
  uint16_t wrt_data[2];

  /*
   * Various flags
   */
  bool pi, pi_pending;
  uint16_t interrupts;
  uint16_t dmc_req;
  bool dmc_cyc;
  unsigned int dmc_dev;
  
  bool start_button_interrupt;
  bool rtclk;
  bool melov; // Memory Lockout Violation
  bool pending_melov;
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
  bool restrict;
  const bool extend_allowed; // i.e. whether CPU has extended addressing
  const uint16_t addr_mask;

  int16_t sc; // shift count

  /*
   * sense switches
   */
  int /*bool*/ ss[4];

  /*
   * the core memory 
   */
  int16_t *core;
  bool *modified;

  bool run;     // flag to say we're still running
  bool fetched; // flag to say that an instruction has been fetched
  uint16_t fetched_p;

  int exit_code;
  bool exit_called;
  
  /*
   * Instruction decode & dispatch
   */
  InstrTable instr_table;

  /*
   * devices
   */
  IODEV **devices;
  IODEV **dmc_devices;
#ifndef RTL_SIM
  EventQueue event_queue;
#endif
  EventQueue::EventTime half_cycles;
  static constexpr double cycle_time = 1.60; // Microseconds
  static constexpr double half_cycles_per_microsecond = (2.0 / cycle_time);
  /*
   * Tracing
   */
  int trace_ptr;
  struct Btrace *btrace_buf;

  void increment_p(uint16_t n = 1);
  void write_prt(unsigned int n, uint16_t v);

  /* These routines are public in order that the instruction
     tables can refer to them. */
public:
  void unimplemented(uint16_t instr);

  void do_CRA(uint16_t instr);
  void do_IAB(uint16_t instr);
  void do_IMA(uint16_t instr);
  void do_INK(uint16_t instr);
  void do_LDA(uint16_t instr);
  void do_LDX(uint16_t instr);
  void do_OTK(uint16_t instr);
  void do_STA(uint16_t instr);
  void do_STX(uint16_t instr);
  void do_ACA(uint16_t instr);
  void do_ADD(uint16_t instr);
  void do_AOA(uint16_t instr);
  void do_SUB(uint16_t instr);
  void do_TCA(uint16_t instr);
  void do_ANA(uint16_t instr);
  void do_CSA(uint16_t instr);
  void do_CHS(uint16_t instr);
  void do_CMA(uint16_t instr);
  void do_ERA(uint16_t instr);
  void do_SSM(uint16_t instr);
  void do_SSP(uint16_t instr);
  void do_ALR(uint16_t instr);
  void do_ALS(uint16_t instr);
  void do_ARR(uint16_t instr);
  void do_ARS(uint16_t instr);
  void do_LGL(uint16_t instr);
  void do_LGR(uint16_t instr);
  void do_LLL(uint16_t instr);
  void do_LLR(uint16_t instr);
  void do_LLS(uint16_t instr);
  void do_LRL(uint16_t instr);
  void do_LRR(uint16_t instr);
  void do_LRS(uint16_t instr);
  void do_INA(uint16_t instr);
  void do_OCP(uint16_t instr);
  void do_OTA(uint16_t instr);
  void do_SMK(uint16_t instr);
  void do_SKS(uint16_t instr);
  void do_CAS(uint16_t instr);
  void do_ENB(uint16_t instr);
  void do_HLT(uint16_t instr);
  void do_INH(uint16_t instr);
  void do_IRS(uint16_t instr);
  void do_JMP(uint16_t instr);
  void do_JST(uint16_t instr);
  void do_NOP(uint16_t instr);
  void do_RCB(uint16_t instr);
  void do_SCB(uint16_t instr);
  void do_SKP(uint16_t instr);
  void do_SLN(uint16_t instr);
  void do_SLZ(uint16_t instr);
  void do_SMI(uint16_t instr);
  void do_SNZ(uint16_t instr);
  void do_SPL(uint16_t instr);
  void do_SR1(uint16_t instr);
  void do_SR2(uint16_t instr);
  void do_SR3(uint16_t instr);
  void do_SR4(uint16_t instr);
  void do_SRC(uint16_t instr);
  void do_SS1(uint16_t instr);
  void do_SS2(uint16_t instr);
  void do_SS3(uint16_t instr);
  void do_SS4(uint16_t instr);
  void do_SSC(uint16_t instr);
  void do_SSR(uint16_t instr);
  void do_SSS(uint16_t instr);
  void do_SZE(uint16_t instr);
  void do_CAL(uint16_t instr);
  void do_CAR(uint16_t instr);
  void do_ICA(uint16_t instr);
  void do_ICL(uint16_t instr);
  void do_ICR(uint16_t instr);

#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
  /*
   * NPL group A
   */
  void do_ad1(uint16_t instr);
  void do_ad1_15(uint16_t instr);
  void do_adc(uint16_t instr);
  void do_adc_15(uint16_t instr);
  void do_cm1(uint16_t instr);
  void do_ltr(uint16_t instr);
  void do_btr(uint16_t instr);
  void do_btl(uint16_t instr);
  void do_rtl(uint16_t instr);
  void do_rcb_ssp(uint16_t instr);
  void do_cpy(uint16_t instr);
  void do_btb(uint16_t instr);
  void do_bcl(uint16_t instr);
  void do_bcr(uint16_t instr);
  void do_ld1(uint16_t instr);
  void do_isg(uint16_t instr);
  void do_cma_aca(uint16_t instr);
  void do_cma_aca_c(uint16_t instr);
  void do_a2a(uint16_t instr);
  void do_a2c(uint16_t instr);
  void do_ics(uint16_t instr);
  void do_scb_a2a(uint16_t instr);
  void do_scb_aoa(uint16_t instr);
  void do_a2c_scb(uint16_t instr);
  void do_aca_scb(uint16_t instr);
  void do_icr_scb(uint16_t instr);
  void do_rtl_scb(uint16_t instr);
  void do_btb_scb(uint16_t instr);
  void do_noa(uint16_t instr);
#endif

  void do_EXA(uint16_t instr);
  void do_DXA(uint16_t instr);

  void do_ERM(uint16_t instr);

  void do_RMP(uint16_t instr);

  void do_DBL(uint16_t instr);
  void do_DIV(uint16_t instr);
  void do_MPY(uint16_t instr);
  void do_NRM(uint16_t instr);
  void do_SCA(uint16_t instr);
  void do_SGL(uint16_t instr);
  void do_iab_sca(uint16_t instr);

  void generic_shift(uint16_t instr);
  void generic_skip(uint16_t instr);
#ifdef TEST_GENERIC_SKIP
  void test_generic_skip();
#endif
  void generic_group_A(uint16_t instr);
#ifdef TEST_GENERIC_GROUP_A
  void test_generic_group_A();
#endif

  static int16_t ex_sc(uint16_t instr);

};

/* Bodge around missing "long long" support with mingw */
#ifndef PRIuLL
#if defined(_WIN32) || defined(__WIN32__)
#define PRIuLL "%l64u"
#else
#define PRIuLL "%llu"
#endif
#endif

#endif // _PROC_HPP_
