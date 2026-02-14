/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005, 2026  Adrian Wise
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

#ifndef __INSTR_HH__
#define __INSTR_HH__

#include <cstdint>

/*
 * Turn the following on to use just the common code for
 * generic group A that understands the meanings of the various
 * instruction bits.
 * If left commented out this routine is used for those
 * instructions not identified in the "microcoding" paper
 */
//#define GENERIC_GROUP_A
//#define TEST_GENERIC_GROUP_A
//#define TEST_GENERIC_SKIP

class Proc;

class InstrTable
{
public:

  class Instr
  {
  public:

    enum INSTR_TYPE 
      {
        UNDEFINED,
        GB, // Generic type B
        SH, // Shift
        SK, // Skip
        GA, // Generic type A
        MR, // Memory reference
        IO, // IO intructions
        IOG // IO instuction pretending to be Generic
      };
    
    typedef void (Proc::*ExecFunc_pt)(uint16_t instr);

    Instr(const char *mnemonic=0,
          INSTR_TYPE type=UNDEFINED,
          uint16_t opcode=0,
          const char *description=0,
          ExecFunc_pt exec=0,
          bool alloc=false);
    ~Instr();
    
    const char *disassemble(uint16_t addr,
                            uint16_t instr,
                            bool brk,
                            uint16_t y = 0,
                            bool y_valid = false);

    static signed short ex_sc(uint16_t instr);

    const char *get_mnemonic(){return mnemonic;};
    INSTR_TYPE get_type(){return type;};
    uint16_t get_opcode(){return opcode;};
    const char *get_description(){return description;};
    ExecFunc_pt get_exec(){return exec;};
    bool get_alloc(){return alloc;};

  private:
    const char *   mnemonic;
    INSTR_TYPE     type;
    uint16_t opcode;
    const char *   description;
    ExecFunc_pt    exec;
    bool           alloc; // storage allocated with "new" (else static)

    static const char *str_ea(uint16_t addr,
                              uint16_t instr,
                              uint16_t y = 0,
                              bool y_valid = false);
  };

public:
  InstrTable();
  ~InstrTable();

  //
  // dispatch returns a pointer to the appropriate function
  // to do the action of the instruction it is passed
  //
  inline void (Proc::*dispatch(uint16_t instr))(uint16_t instr)
  { return dispatch_table[instr]->get_exec(); };
  inline bool defined(uint16_t instr)
  { return dispatch_table[instr]->get_type() != Instr::UNDEFINED; };

  const char *disassemble(uint16_t addr,
                          uint16_t instr,
                          bool brk,
                          uint16_t y = 0,
                          bool y_valid = false);

  Instr *lookup(const char *mnemonic) const;
  void dump_dispatch_table() const;
  
private:
  Instr **dispatch_table;

  static Instr standard[];
  static Instr extended[];
  static Instr restricted[];
  static Instr mem_parity[];
  static Instr arithmetic[];
#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
  static Instr NPL_group_a[];
#endif
  static Instr *instructions[];

#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
  static uint16_t HLT_alias[];
  static uint16_t CMA_alias[];
  static uint16_t CRA_alias[];
  static uint16_t SSM_alias[];
  static uint16_t CHS_alias[];
  static uint16_t CAR_alias[];
  static uint16_t CAL_alias[];
  static uint16_t SSP_alias[];
  static uint16_t ICL_alias[];
  static uint16_t ICR_alias[];
  static uint16_t RCB_alias[];
  static uint16_t CSA_alias[];
  static uint16_t TCA_alias[];
  static uint16_t ICA_alias[];
  static uint16_t SCB_alias[];
  static uint16_t AOA_alias[];
  static uint16_t AD1_alias[];
  static uint16_t AD1_15_alias[];
  static uint16_t ACA_alias[];
  static uint16_t ADC_alias[];
  static uint16_t ADC_15_alias[];
  static uint16_t CM1_alias[];
  static uint16_t LTR_alias[];
  static uint16_t BTR_alias[];
  static uint16_t BTL_alias[];
  static uint16_t RTL_alias[];
  static uint16_t RCB_SSP_alias[];
  static uint16_t CPY_alias[];
  static uint16_t BTB_alias[];
  static uint16_t BCL_alias[];
  static uint16_t BCR_alias[];
  static uint16_t LD1_alias[];
  static uint16_t ISG_alias[];
  static uint16_t CMA_ACA_alias[];
  static uint16_t CMA_ACA_C_alias[];
  static uint16_t A2A_alias[];
  static uint16_t A2C_alias[];
  static uint16_t ICS_alias[];
  static uint16_t SCB_A2A_alias[];
  static uint16_t SCB_AOA_alias[];
  static uint16_t A2C_SCB_alias[];
  static uint16_t ACA_SCB_alias[];
  static uint16_t ICR_SCB_alias[];
  static uint16_t RTL_SCB_alias[];
  static uint16_t BTB_SCB_alias[];
  static uint16_t NOA_alias[];

  static uint16_t *aliases[];
  
  void apply_one_alias( uint16_t alias[]);
  void apply_aliases();
#endif

  void build_one_instr_table(Instr itable[]);
  void build_instr_tables();
  
  //Common instructions
  static Instr ui; // Unimplemented instruction
  static Instr gskp;
  static Instr gshf;
  static Instr gena;


};

#endif
