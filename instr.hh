/* Honeywell Series 16 emulator $Id: instr.hh,v 1.4 2001/06/09 22:12:02 adrian Exp $
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
 * $Log: instr.hh,v $
 * Revision 1.4  2001/06/09 22:12:02  adrian
 * Various bug fixes
 *
 * Revision 1.3  2000/01/15 06:26:17  adrian
 * Generic shift and group A instructions
 *
 * Revision 1.2  1999/06/04 07:57:21  adrian
 * *** empty log message ***
 *
 * Revision 1.1  1999/02/20 00:06:35  adrian
 * Initial revision
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

class Instr
{
public:
  Instr(char *mnemonic=NULL, INSTR_TYPE type=UNDEFINED,
        unsigned short opcode=0,
        char *description=NULL,
        void (Proc::*exec)(unsigned short instr)=NULL);


  static char *disassemble(unsigned short addr, unsigned short instr, bool brk);
  static void build_instr_tables();

private:
  char *mnemonic;
  INSTR_TYPE type;
  unsigned short opcode;
  char *description;
  void (Proc::*exec)(unsigned short instr);

  static Instr **dispatch_table;

public:
  //
  // dispatch returns a pointer to the appropriate function
  // to do the action of the instruction it is passed
  //
  static inline void (Proc::*dispatch(unsigned short instr))(unsigned short instr)
    { return dispatch_table[instr]->exec; };
  static inline bool defined(unsigned short instr)
    { return dispatch_table[instr]->type != UNDEFINED; };

private:
  static Instr standard[];
  static Instr extended[];
  static Instr mem_parity[];
  static Instr arithmetic[];
  static Instr *instructions[];

#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
  static Instr NPL_group_a[];

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
  static void apply_one_alias( unsigned short alias[]);
  static void apply_aliases();
#endif

  char *do_disassemble(unsigned short addr, unsigned short instr, bool brk);
  static void build_one_instr_table(Instr *itable);
  static char *str_ea(unsigned short addr, unsigned short instr);
};

#endif
