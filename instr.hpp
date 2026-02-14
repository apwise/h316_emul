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

#ifndef _INSTR_HPP_
#define _INSTR_HPP_

#include <cstdint>
#include <string>
#include <vector>
#include <map>

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

namespace h16 {
  
  class CPU;

  class InstrTable {
  public:

    typedef void (CPU::*ExecFunc_pt)(uint16_t instr);

    struct Instr {

      enum INSTR_TYPE {
        UD,
        GB, // Generic type B
        SH, // Shift
        SK, // Skip
        GA, // Generic type A
        MR, // Memory reference
        IO, // IO intructions
        IG  // IO instuction pretending to be Generic
      };
    
      std::string mnemonic;
      INSTR_TYPE  type;
      uint16_t    opcode;
      std::string description;
      ExecFunc_pt exec;

      Instr(const std::string &mnemonic,
            INSTR_TYPE type,
            uint16_t opcode,
            const std::string &description,
            ExecFunc_pt exec);
    
      const std::string disassemble(uint16_t addr,
                                    uint16_t instr,
                                    bool brk,
                                    uint16_t y = 0,
                                    bool y_valid = false) const;

      static signed short ex_sc(uint16_t instr);

      static std::string str_ea(uint16_t addr,
                                uint16_t instr,
                                uint16_t y = 0,
                                bool y_valid = false);
    };

    typedef std::vector<Instr> InstrTable_t;
    typedef std::vector<uint16_t> AliasTable_t;

    InstrTable();

    //
    // dispatch returns a pointer to the appropriate function
    // to do the action of the instruction it is passed
    //
    inline ExecFunc_pt dispatch(uint16_t instr)
    { return dispatch_table[instr]; }
    
    inline bool defined(uint16_t instr)
    { return instructions[instr].type != Instr::UD; }

    const std::string disassemble(uint16_t addr,
                                  uint16_t instr,
                                  bool brk,
                                  uint16_t y = 0,
                                  bool y_valid = false) const;
    
    const Instr *lookup(const std::string &mnemonic) const;
    void dump_instructions() const;
  
  private:
    static const uint16_t B7 = 0001000; // Big-endian bit 7
    
    InstrTable_t instructions;
    std::vector<ExecFunc_pt> dispatch_table;
    std::map<std::string, const Instr *> mnemonic_to_instr;

    static const InstrTable_t standard;
    static const InstrTable_t ea;
    static const InstrTable_t ml;
    static const InstrTable_t mp;
    static const InstrTable_t hsa;
#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
    static const InstrTable_t NPL_group_a;
#endif
    static const std::vector<InstrTable_t> instr_tables;

#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
    static const AliasTable_t HLT_alias;
    static const AliasTable_t CMA_alias;
    static const AliasTable_t CRA_alias;
    static const AliasTable_t SSM_alias;
    static const AliasTable_t CHS_alias;
    static const AliasTable_t CAR_alias;
    static const AliasTable_t CAL_alias;
    static const AliasTable_t SSP_alias;
    static const AliasTable_t ICL_alias;
    static const AliasTable_t ICR_alias;
    static const AliasTable_t RCB_alias;
    static const AliasTable_t CSA_alias;
    static const AliasTable_t TCA_alias;
    static const AliasTable_t ICA_alias;
    static const AliasTable_t SCB_alias;
    static const AliasTable_t AOA_alias;
    static const AliasTable_t AD1_alias;
    static const AliasTable_t AD1_15_alias;
    static const AliasTable_t ACA_alias;
    static const AliasTable_t ADC_alias;
    static const AliasTable_t ADC_15_alias;
    static const AliasTable_t CM1_alias;
    static const AliasTable_t LTR_alias;
    static const AliasTable_t BTR_alias;
    static const AliasTable_t BTL_alias;
    static const AliasTable_t RTL_alias;
    static const AliasTable_t RCB_SSP_alias;
    static const AliasTable_t CPY_alias;
    static const AliasTable_t BTB_alias;
    static const AliasTable_t BCL_alias;
    static const AliasTable_t BCR_alias;
    static const AliasTable_t LD1_alias;
    static const AliasTable_t ISG_alias;
    static const AliasTable_t CMA_ACA_alias;
    static const AliasTable_t CMA_ACA_C_alias;
    static const AliasTable_t A2A_alias;
    static const AliasTable_t A2C_alias;
    static const AliasTable_t ICS_alias;
    static const AliasTable_t SCB_A2A_alias;
    static const AliasTable_t SCB_AOA_alias;
    static const AliasTable_t A2C_SCB_alias;
    static const AliasTable_t ACA_SCB_alias;
    static const AliasTable_t ICR_SCB_alias;
    static const AliasTable_t RTL_SCB_alias;
    static const AliasTable_t BTB_SCB_alias;
    static const AliasTable_t NOA_alias;

    static const std::vector<AliasTable_t> aliases;
  
    void apply_one_alias(const AliasTable_t &alias);
    void apply_aliases();
#endif

    void build_one_instr_table(const InstrTable_t &itable);
    void build_instr_tables();
  
    // Common instructions
    static const Instr uimp; // Unimplemented instruction
    static const Instr gskp; // Generic skip
    static const Instr gshf; // Generic Shift
    static const Instr gena; // Generic Group-A
  };
}
#endif // _INSTR_HPP_
