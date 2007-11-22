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

#ifndef __INSTR_HH__
#define __INSTR_HH__

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
    
    typedef void (Proc::*ExecFunc_pt)(unsigned short instr);

    Instr(char *mnemonic=NULL,
	  INSTR_TYPE type=UNDEFINED,
	  unsigned short opcode=0,
	  char *description=NULL,
	  ExecFunc_pt exec=NULL,
	  bool alloc=false);
    ~Instr();
    
    const char *disassemble(unsigned short addr,
			    unsigned short instr,
			    bool brk,
			    unsigned short y = 0,
			    bool y_valid = false);

    static signed short ex_sc(unsigned short instr);

    const char *get_mnemonic(){return mnemonic;};
    INSTR_TYPE get_type(){return type;};
    unsigned short get_opcode(){return opcode;};
    const char *get_description(){return description;};
    ExecFunc_pt get_exec(){return exec;};
    bool get_alloc(){return alloc;};

  private:
    char *   mnemonic;
    INSTR_TYPE     type;
    unsigned short opcode;
    char *   description;
    ExecFunc_pt    exec;
    bool           alloc; // storage allocated with "new" (else static)

    static const char *str_ea(unsigned short addr,
			      unsigned short instr,
			      unsigned short y = 0,
			      bool y_valid = false);
  };

public:
  InstrTable();
  ~InstrTable();

  //
  // dispatch returns a pointer to the appropriate function
  // to do the action of the instruction it is passed
  //
  inline void (Proc::*dispatch(unsigned short instr))(unsigned short instr)
  { return dispatch_table[instr]->get_exec(); };
  inline bool defined(unsigned short instr)
  { return dispatch_table[instr]->get_type() != Instr::UNDEFINED; };

  const char *disassemble(unsigned short addr,
			  unsigned short instr,
			  bool brk,
			  unsigned short y = 0,
			  bool y_valid = false);

  Instr *lookup(const char *mnemonic) const;
  void dump_dispatch_table() const;
  
private:
  Instr **dispatch_table;

  static Instr standard[];
  static Instr extended[];
  static Instr mem_parity[];
  static Instr arithmetic[];
#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
  static Instr NPL_group_a[];
#endif
  static Instr *instructions[];

#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
  static unsigned short HLT_alias[];
  static unsigned short CMA_alias[];
  static unsigned short CRA_alias[];
  static unsigned short SSM_alias[];
  static unsigned short CHS_alias[];
  static unsigned short CAR_alias[];
  static unsigned short CAL_alias[];
  static unsigned short SSP_alias[];
  static unsigned short ICL_alias[];
  static unsigned short ICR_alias[];
  static unsigned short RCB_alias[];
  static unsigned short CSA_alias[];
  static unsigned short TCA_alias[];
  static unsigned short ICA_alias[];
  static unsigned short SCB_alias[];
  static unsigned short AOA_alias[];
  static unsigned short AD1_alias[];
  static unsigned short AD1_15_alias[];
  static unsigned short ACA_alias[];
  static unsigned short ADC_alias[];
  static unsigned short ADC_15_alias[];
  static unsigned short CM1_alias[];
  static unsigned short LTR_alias[];
  static unsigned short BTR_alias[];
  static unsigned short BTL_alias[];
  static unsigned short RTL_alias[];
  static unsigned short RCB_SSP_alias[];
  static unsigned short CPY_alias[];
  static unsigned short BTB_alias[];
  static unsigned short BCL_alias[];
  static unsigned short BCR_alias[];
  static unsigned short LD1_alias[];
  static unsigned short ISG_alias[];
  static unsigned short CMA_ACA_alias[];
  static unsigned short CMA_ACA_C_alias[];
  static unsigned short A2A_alias[];
  static unsigned short A2C_alias[];
  static unsigned short ICS_alias[];
  static unsigned short SCB_A2A_alias[];
  static unsigned short SCB_AOA_alias[];
  static unsigned short A2C_SCB_alias[];
  static unsigned short ACA_SCB_alias[];
  static unsigned short ICR_SCB_alias[];
  static unsigned short RTL_SCB_alias[];
  static unsigned short BTB_SCB_alias[];
  static unsigned short NOA_alias[];

  static unsigned short *aliases[];
  
  void apply_one_alias( unsigned short alias[]);
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
