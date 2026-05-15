/* Honeywell Series 16 emulator
 *
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
 */

#ifndef _CPU_HPP_
#define _CPU_HPP_

#include <vector>
#include <array>
#include <cstdint>

#include "instr.hpp"
#include "io_types.hpp"

struct FP_INTF;

namespace h16 {
  class CPU {
    friend class InstrTable;
    
  public:
    CPU(bool hasEa);
    virtual ~CPU();

    void mem_access(bool p_not_pp1, bool store);
  
    /*
     * Get and set registers - used by monitor
     *
     * Writing to X changes location "zero"
     */
    void set_py(uint16_t n) {p = y = n;}
    uint16_t get_p() const {return p;}
    void set_a(uint16_t n) {a = n;}
    uint16_t get_a() const {return a;}
    void set_b(uint16_t n) {b = n;}
    uint16_t get_b() const {return b;}
    void set_x(uint16_t n);
    void set_just_x(uint16_t n);
    uint16_t get_x() const {return x;}
  
    /* 
     * Get and set sense switches
     */
    bool get_ss(int sw) {return ss[sw];}
    void set_ss(int sw, bool v) {ss[sw] = v;}
  
    uint64_t get_half_cycles() {return half_cycles;}
    uint64_t &get_half_cycles_ref() {return half_cycles;}

    //unsigned int get_dmc_dev(){return dmc_dev;}

    void set_interrupt(uint16_t mask);
    void clear_interrupt(uint16_t mask);
    void set_break(unsigned n, bool v);

    int get_wrt_info(uint16_t addr[2], uint16_t data[2]);

    std::string dis();

    struct FP_INTF *fp_intf();
  
    /*
     * Interface routines to read and write memory
     *
     * writing to location "zero" (possibly relocated)
     * also changes the X register.
     */
    void write(uint16_t addr, int16_t data);
    uint16_t read(uint16_t addr) {
      y = addr;
      m = core[addr & addr_mask];
      return m;
    };

  private:
    const bool ea_allowed; // i.e. whether CPU has extended addressing
    const uint16_t addr_mask;
    
  protected:
    virtual void master_clear();

    /*
     * Abstract virtual interface for I/O related things to pass
     * to a derived class (usually Proc or CpuRtl).
     */
    virtual IoStatus ina(uint16_t instr, int16_t &data)=0;
    virtual IoStatus sks(uint16_t instr)=0;
    virtual IoStatus ota(uint16_t instr, int16_t data)=0;
    virtual void ocp(uint16_t instr)=0;
    virtual void smk(uint16_t mask)=0;
    virtual void event(IoDevice dev, int reason)=0;
    virtual void dmc(unsigned dmc_dev, // 0 to 15
                     int16_t &data, bool erl) = 0;
    virtual bool jump_time_to_event(uint64_t &half_cycles) = 0;
    virtual void io_polling(uint16_t instr) = 0;
  
    bool do_instr();
    void unwind_instr();
    uint16_t get_m() {return m;}

    void set_run(bool x) { run = x; }
    bool get_run() { return run; }

    void set_sbi(bool x) { start_button_interrupt = x; }
    
    /*
     * The core memory 
     */
    std::vector<uint16_t> core;
    std::vector<bool> modified;

    /*
     * Tracing
     */
    struct Btrace {
      bool v; // valid flag
      bool brk; // this is some sort of break
      unsigned long half_cycles;
      uint16_t a, b, x;
      bool c;
      uint16_t p, instr, y;
    };

    unsigned trace_ptr;
    std::vector<Btrace> btrace_buf;

  private:

    // Architectural registers
    int16_t a; // accumulator
    int16_t b; // auxilliary register
    int16_t x; // index register
    uint16_t p; // program counter
    int16_t sc; // shift count
    uint16_t j; // base sector relocation
    std::array<bool, 64> prt; // Memory protection bits

    // Architectural flags (aka keys)
    bool c;  // Carry
    bool pi; // Permit Interrupts
    bool ml; // Memory Lockout mode
    bool ea; // Extended Addressing mode
    bool dp; // Double Precision mode

    // Registers that aren't really architectural
    int16_t op; // Flags, etc. as a word
    int16_t m;  // Memory register
    uint16_t y; // Address register

    // Interrupt and break requests
    bool start_button_interrupt;
    uint16_t interrupts;
    bool rtclk;
    uint16_t dmc_req;
    bool melov; // Memory Lockout Violation

    // Various flags
    bool run;     // flag to say still running
    bool fetched; // flag to say that an instruction has been fetched

    bool pi_pending; // pi due to be set (after next instruction)
    bool ml_pending; // ml due to be set (after next instruction)
    bool ea_disable; // disable extended-addressing (after next jmp)
    bool pmi;        // previous mode indicator (ea whn interrupted)
    
    bool melov_pending, prev_melov;
   
    bool dmc_cyc;
    unsigned int dmc_dev;
    uint16_t fetched_p;
 
    bool break_flag;
    bool break_intr;
    unsigned int break_addr;

    bool jmp_self_minus_one;
    bool last_jmp_self_minus_one;

    uint64_t half_cycles;

    // Memory write testing
    int wrts;
    uint16_t wrt_addr[2];
    uint16_t wrt_data[2];

    /*
     * sense switches
     */
    bool ss[4];
 
    /*
     * Instruction decode & dispatch
     */
    InstrTable instr_table;

    uint16_t e_a(uint16_t instr);


    void increment_p(uint16_t n = 1);
    void write_prt(unsigned int n, uint16_t v);

    bool optimize_io_poll(uint16_t instr);

    static int16_t ex_sc(uint16_t instr);

    static int16_t short_add(int16_t a, int16_t m, bool &c);
    static int16_t short_sub(int16_t a, int16_t m, bool &c);
    static int16_t short_adc(int16_t a, int16_t m, bool &c);
    static int32_t multiply(int16_t &ra, int16_t &rb, int16_t rm, int16_t &sc);
    static unsigned divide(int16_t &ra, int16_t &rb, int16_t rm, int16_t &sc, bool &cbitf);
  
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
  };
}
#endif // _CPU_HPP_
