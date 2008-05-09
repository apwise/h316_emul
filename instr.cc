/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005, 2007  Adrian Wise
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation1; either version 2 of the License, or
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stdtty.hh"

#include <iostream>

#ifdef NO_DO_PROCS
class Proc;
#define PD(proc) 0
#else
#include "proc.hh"
#define PD(proc) &Proc::proc
#endif

#include "instr.hh"

InstrTable::Instr::Instr(const char *mnemonic,
                         INSTR_TYPE type,
                         unsigned short opcode,
                         const char *description,
                         ExecFunc_pt exec,
                         bool alloc)
  : mnemonic(mnemonic),
    type(type),
    opcode(opcode),
    description(description),
    exec(exec),
    alloc(alloc)
{
}

InstrTable::Instr::~Instr()
{
  if (alloc) {
    // Oh yuk...
    free(const_cast<char *>(mnemonic));
    free(const_cast<char *>(description));
  }
}

const char *InstrTable::Instr::disassemble(unsigned short addr,
                                           unsigned short instr,
                                           bool brk,
                                           unsigned short y,
                                           bool y_valid)
{
  static char str[64];
  static char *p;

  if (brk)
    sprintf(str, "break: ");
  else
    sprintf(str, "%06o ", addr);
  p = str+7;
  
  switch(type)
    {
    case UNDEFINED:
      sprintf(p, " %06o    ???", instr);
      break;
    case MR:
      sprintf(p, "%c%1o %02o %04o %s%c %s",
              ((instr & 0x8000)?'-':' '),
              ((instr>>14) & 1), ((instr >> 10) & 0xf),
              (instr & 0x3ff), mnemonic,
              ((instr & 0x8000)?'*':' '),
              str_ea(addr, instr, y, y_valid));
      break;
    case SH:
      sprintf(p, " %04o %02o   %s  '%02o",
              ((instr>>6) & 0x3ff), (instr & 0x3f),
              mnemonic,
              static_cast<int>(-ex_sc(instr)));
      break;
    case IO:
      sprintf(p, " %02o %04o   %s  '%04o",
              ((instr>>10) & 0x3f), (instr & 0x3ff),
              mnemonic, (instr & 0x3ff));
      break;
    default:
      sprintf(p, " %06o    %s", instr, mnemonic);
      break;
    }
  
  return str;
}

signed short InstrTable::Instr::ex_sc(unsigned short instr)
{
  signed short res = instr & 0x003f;

  if (res != 0)
    res |= (~0x003f); // Sign extend

  return res;
}

const char *InstrTable::Instr::str_ea(unsigned short addr,
                                      unsigned short instr,
                                      unsigned short y,
                                      bool y_valid)
{
  unsigned short ea;
  static char str[64];
  char a[10];
  char b[10];
  bool is_ldx = false;
  bool is_jmp = false;
  bool full_ea = false;
  bool indexed = false;
  bool indirect = false;
  bool print_y = false;

  if (((instr >> 10) & 037) == 035)
    is_ldx = true;
  
  if (((instr >> 10) & 017) == 001)
    is_jmp = true;
  
  if (instr & 0x0200)
    ea = (addr & 0x7e00) | (instr & 0x01ff);
  else
    ea = (instr & 0x01ff);

  switch(ea-addr)
    {
    case -2: strcpy(a, "*-2"); break;
    case -1: strcpy(a, "*-1"); break;
    case 0: strcpy(a, "*"); break;
    case 1: strcpy(a, "*+1"); break;
    case 2: strcpy(a, "*+2"); break;
    default:
      sprintf(a, "'%06o", ea);
      full_ea = true;
    }
  
  if (((instr & 0x4000)!=0) && (!is_ldx))
    indexed = true;

  if ((instr & 0x8000)!=0)
    indirect = true;

  if ((indirect || indexed || ((!full_ea) && (!is_jmp))) &&
      y_valid)
    {
      print_y = true;
      sprintf(b, "='%06o", y);
    }

  sprintf(str, "%s%s%s", a,
          ((indexed) ? ",1" : ""),
          ((print_y) ? b : ""));
  
  return str;
}

#define IT(t) InstrTable::Instr::t

InstrTable::Instr InstrTable::standard[] = 
  {
    Instr("CRA", IT(GA), 0140040, "Clear A", PD(do_CRA)),
    Instr("IAB", IT(GB), 0000201, "Interchange A and B", PD(do_IAB)),
    Instr("IMA", IT(MR), 013,     "Interchange memory and A", PD(do_IMA)),
    Instr("INK", IT(GB), 0000043, "Input keys", PD(do_INK)),
    Instr("LDA", IT(MR), 002,     "Load A", PD(do_LDA)),
    Instr("LDX", IT(MR), 075,     "Load X", PD(do_LDX)),    //T=1
    Instr("OTK", IT(IOG),0171020, "Output keys", PD(do_OTK)),
    Instr("STA", IT(MR), 004,     "Store A", PD(do_STA)),
    Instr("STX", IT(MR), 055,     "Store X", PD(do_STX)),   //T=0
    Instr("ACA", IT(GA), 0141216, "Add C to A", PD(do_ACA)),
    Instr("ADD", IT(MR), 006,     "Add", PD(do_ADD)),
    Instr("AOA", IT(GA), 0141206, "Add one to A", PD(do_AOA)),
    Instr("SUB", IT(MR), 007,     "Subtract", PD(do_SUB)),
    Instr("TCA", IT(GA), 0140407, "Two's complement A", PD(do_TCA)),
    Instr("ANA", IT(MR), 003,     "And", PD(do_ANA)),
    Instr("CSA", IT(GA), 0140320, "Copy sign and set sign plus", PD(do_CSA)),
    Instr("CHS", IT(GA), 0140024, "Complement A's sign", PD(do_CHS)),
    Instr("CMA", IT(GA), 0140401, "Complement A", PD(do_CMA)),
    Instr("ERA", IT(MR), 005,     "Exclusive OR", PD(do_ERA)),
    Instr("SSM", IT(GA), 0140500, "Set sign minus", PD(do_SSM)),
    Instr("SSP", IT(GA), 0140100, "Set sign plus", PD(do_SSP)),
    Instr("ALR", IT(SH), 00416,   "Logical left rotate", PD(do_ALR)),
    Instr("ALS", IT(SH), 00415,   "Arithmetic left shift", PD(do_ALS)),
    Instr("ARR", IT(SH), 00406,   "Logical right rotate", PD(do_ARR)),
    Instr("ARS", IT(SH), 00405,   "Arithmetic right shift", PD(do_ARS)),
    Instr("LGL", IT(SH), 00414,   "Logical left shift", PD(do_LGL)),
    Instr("LGR", IT(SH), 00404,   "Logical right shift", PD(do_LGR)),
    Instr("LLL", IT(SH), 00410,   "Long left logical shift", PD(do_LLL)),
    Instr("LLR", IT(SH), 00412,   "Long left rotate", PD(do_LLR)),
    Instr("LLS", IT(SH), 00411,   "Long arithmetic left shift", PD(do_LLS)),
    Instr("LRL", IT(SH), 00400,   "Long right logical shift", PD(do_LRL)),
    Instr("LRR", IT(SH), 00402,   "Long right rotate", PD(do_LRR)),
    Instr("LRS", IT(SH), 00401,   "Long arithmetic right shift", PD(do_LRS)),
    Instr("INA", IT(IO), 054,     "Input to A", PD(do_INA)),
    Instr("OCP", IT(IO), 014,     "Output control pulse", PD(do_OCP)),
    Instr("OTA", IT(IO), 074,     "Output from A", PD(do_OTA)),
    Instr("SMK", IT(IO), 0174,    "Set mask", PD(do_SMK)),
    Instr("SKS", IT(IO), 034,     "Skip if ready line set", PD(do_SKS)),
    Instr("CAS", IT(MR), 011,     "Compare and skip", PD(do_CAS)),
    Instr("ENB", IT(GB), 0000401, "Enable interrupts", PD(do_ENB)),
    Instr("HLT", IT(GB), 0000000, "Halt", PD(do_HLT)),
    Instr("INH", IT(GB), 0001001, "Inhibit interrupts", PD(do_INH)),
    Instr("IRS", IT(MR), 012,     "Increment, replace and skip", PD(do_IRS)),
    Instr("JMP", IT(MR), 001,     "Jump", PD(do_JMP)),
    Instr("JST", IT(MR), 010,     "Jump and store", PD(do_JST)),
    Instr("NOP", IT(SK), 0101000, "No operation", PD(do_NOP)),
    Instr("RCB", IT(GA), 0140200, "Reset C bit", PD(do_RCB)),
    Instr("SCB", IT(GA), 0140600, "Set C bit", PD(do_SCB)),
    Instr("SKP", IT(SK), 0100000, "Skip", PD(do_SKP)),
    Instr("SLN", IT(SK), 0101100, "Skip if last non-zero", PD(do_SLN)),
    Instr("SLZ", IT(SK), 0100100, "Skip if last zero", PD(do_SLZ)),
    Instr("SMI", IT(SK), 0101400, "Skip if A minus", PD(do_SMI)),
    Instr("SNZ", IT(SK), 0101040, "Skip if A non-zero", PD(do_SNZ)),
    Instr("SPL", IT(SK), 0100400, "Skip if A plus", PD(do_SPL)),
    Instr("SR1", IT(SK), 0100020, "Skip if sense switch 1 reset", PD(do_SR1)),
    Instr("SR2", IT(SK), 0100010, "Skip if sense switch 2 reset", PD(do_SR2)),
    Instr("SR3", IT(SK), 0100004, "Skip if sense switch 3 reset", PD(do_SR3)),
    Instr("SR4", IT(SK), 0100002, "Skip if sense switch 4 reset", PD(do_SR4)),
    Instr("SRC", IT(SK), 0100001, "Skip if C reset", PD(do_SRC)),
    Instr("SS1", IT(SK), 0101020, "Skip if sense switch 1 set", PD(do_SS1)),
    Instr("SS2", IT(SK), 0101010, "Skip if sense switch 2 set", PD(do_SS2)),
    Instr("SS3", IT(SK), 0101004, "Skip if sense switch 3 set", PD(do_SS3)),
    Instr("SS4", IT(SK), 0101002, "Skip if sense switch 4 set", PD(do_SS4)),
    Instr("SSC", IT(SK), 0101001, "Skip if C set", PD(do_SSC)),
    Instr("SSR", IT(SK), 0100036, "Skip if no sense switch set", PD(do_SSR)),
    Instr("SSS", IT(SK), 0101036, "Skip is any sense switch set", PD(do_SSS)),
    Instr("SZE", IT(SK), 0100040, "Skip if A zero", PD(do_SZE)),
    Instr("CAL", IT(GA), 0141050, "Clear A left half", PD(do_CAL)),
    Instr("CAR", IT(GA), 0141044, "Clear A right half", PD(do_CAR)),
    Instr("ICA", IT(GA), 0141340, "Interchange characters in A", PD(do_ICA)),
    Instr("ICL", IT(GA), 0141140, "Interchange and clear left half of A", PD(do_ICL)),
    Instr("ICR", IT(GA), 0141240, "Interchange and clear right half of A", PD(do_ICR)),
    Instr()
  };

InstrTable::Instr InstrTable::extended[] = 
  {
    Instr("DXA", IT(GB), 0000011, "Disable Extended Mode", PD(do_DXA)),
    Instr("EXA", IT(GB), 0000013, "Enable Extended Mode", PD(do_EXA)),
    Instr()
  };

InstrTable::Instr InstrTable::mem_parity[] = 
  {
    Instr("RMP", IT(GB), 0000021, "Reset memory parity error", PD(do_RMP)),
    Instr()
  };

InstrTable::Instr InstrTable::arithmetic[] =
  {
    Instr("DBL", IT(GB), 0000007, "Enter Double Precision Mode", PD(do_DBL)),
    Instr("DIV", IT(MR), 017,     "Divide", PD(do_DIV)),
    Instr("MPY", IT(MR), 016,     "Multiply", PD(do_MPY)),
    Instr("NRM", IT(GB), 0000101, "Normalize", PD(do_NRM)),
    Instr("SCA", IT(GB), 0000041, "Shift count to A", PD(do_SCA)),
    Instr("SGL", IT(GB), 0000005, "Enter Single Precision Mode", PD(do_SGL)),

    Instr("iab_sca", IT(GB), 0000241, "Copy A to B, Shift count to A", PD(do_iab_sca)),

    Instr()
  };

#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
/*
 * If the generic group A routine is used then don't
 * bother with these (or their aliases).
 *
 * Still left here though because the disassembly in the
 * trace file is more use with these mnemonics! Might need to
 * revert to this older method...
 */
InstrTable::Instr InstrTable::NPL_group_a[] = 
  {
    Instr("ad1", IT(GA), 0140042, "Add one without setting overflow", PD(do_ad1)),
    Instr("ad1", IT(GA), 0140443, "Add one without setting overflow", PD(do_ad1_15)),
    Instr("adc", IT(GA), 0140453, "Add C without setting overflow", PD(do_adc_15)),
    Instr("adc", IT(GA), 0140052, "Add C without setting overflow", PD(do_adc)),
    Instr("cm1", IT(GA), 0140012, "Load C-1", PD(do_cm1)),
    Instr("ltr", IT(GA), 0140144, "Copy left to right", PD(do_ltr)),
    Instr("btr", IT(GA), 0140141, "OR left to right", PD(do_btr)),
    Instr("btl", IT(GA), 0140241, "OR right to left", PD(do_btl)),
    Instr("rtl", IT(GA), 0140250, "Copy right to left", PD(do_rtl)),
    Instr("rcb_ssp", IT(GA), 0140300, "reset C bits, set sign positive", PD(do_rcb_ssp)),
    Instr("cpy", IT(GA), 0140321, "Copy sign", PD(do_cpy)),
    Instr("btb", IT(GA), 0140341, "OR to both", PD(do_btb)),
    Instr("bcl", IT(GA), 0140150, "OR to right, clearing left", PD(do_bcl)),
    Instr("bcr", IT(GA), 0140244, "OR to left, clearing right", PD(do_bcr)),
    Instr("ld1", IT(GA), 0140402, "Load one into A register", PD(do_ld1)),
    Instr("isg", IT(GA), 0140412, "Load 2*C-1 into A register", PD(do_isg)),
    Instr("cma_aca", IT(GA), 0140413, "Complement A, Add C to A", PD(do_cma_aca)),
    Instr("cma_aca_c", IT(GA), 0140532, "Complement A, Add C to A, OR new A1 into C", PD(do_cma_aca_c)),
    Instr("a2a", IT(GA), 0140442, "Add two to A", PD(do_a2a)),
    Instr("a2c", IT(GA), 0140452, "Add 2*C to A", PD(do_a2c)),
    Instr("ics", IT(GA), 0140540, "Interchange, clear left, keep sign bit", PD(do_ics)),
    Instr("scb_a2a", IT(GA), 0140602, "Set carry, add 2 to A", PD(do_scb_a2a)),
    Instr("scb_aoa", IT(GA), 0140603, "Set carry, add 1 to A", PD(do_scb_aoa)),
    Instr("a2c_scb", IT(GA), 0140612, "Add 2*C to A, set C", PD(do_a2c_scb)),
    Instr("aca_scb", IT(GA), 0140613, "Add C to A, set C", PD(do_aca_scb)),
    Instr("icr_scb", IT(GA), 0140640, "Interchange and clear right, set C", PD(do_icr_scb)),
    Instr("rtl_scb", IT(GA), 0140650, "Copy right to left, set C", PD(do_rtl_scb)),
    Instr("btb_scb", IT(GA), 0140741, "Or to both halves, set C", PD(do_btb_scb)),
    Instr("noa", IT(GA), 0140000, "No action", PD(do_noa)),
    Instr()
  };
#endif

InstrTable::Instr *InstrTable::instructions[] =
  {
    standard,
    extended,
    mem_parity,
    arithmetic,
#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
    NPL_group_a,
#endif
    NULL
  };

InstrTable::InstrTable()
{
  build_instr_tables();
}

InstrTable::~InstrTable()
{
  unsigned long lim, i, j;

  lim = 1 << 16;

  for (i=0; i<lim; i++) {
    //std::cerr << "i = " << i << std::endl;
    if ((dispatch_table[i]) &&
        (dispatch_table[i]->get_alloc())) {
      for (j=i+1; j<lim; j++)
        if (dispatch_table[i] ==
            dispatch_table[j])
          dispatch_table[j] = 0;
      delete dispatch_table[i];
    }
  }

  delete [] dispatch_table;
  dispatch_table = NULL;
}


void InstrTable::build_one_instr_table(Instr itable[])
{
  Instr *ip;
  unsigned long i=0;
  int ind, t, s, addr, t0, t1;
  
  ip = itable;
  while(ip->get_mnemonic())
    {
      switch(ip->get_type())
        {
        case Instr::GB: // Generic type B
          dispatch_table[ip->get_opcode()] = ip;
          break;
          
        case Instr::SH: // Shift
          for (addr=0; addr<64; addr++)
            {
              i = (ip->get_opcode() << 6) | (addr & 0x03f);
              dispatch_table[i] = ip;
            }
          break;
          
        case Instr::SK: // Skip
          dispatch_table[ip->get_opcode()] = ip;
          break;
          
        case Instr::GA: // Generic type A
          dispatch_table[ip->get_opcode()] = ip;
          break;
          
        case Instr::MR: // Memory reference
          for (ind=0; ind < 2; ind++)
            {
              for (addr=0; addr<512; addr++)
                {
                  for (s=0; s<2; s++)
                    {
                      if (ip->get_opcode() & 0x20) // STX/LDX
                        t0=t1=(ip->get_opcode() >> 4) & 1;
                      else
                        {t0=0; t1=1;}
                      for (t=t0; t<=t1; t++)
                        {
                          i = ((ind & 1) << 15) |
                            ((t & 1) << 14) |
                            ((ip->get_opcode() & 0x0f) << 10) |
                            ((s & 1) << 9) |
                            (addr & 0777);
                          dispatch_table[i] = ip;
                        }
                    }
                }
            }
          break;
          
        case Instr::IO: // IO intructions
          for (addr=0; addr<1024; addr++)
            {
              i = ((ip->get_opcode() & 0x3f) << 10) | (addr & 0x3ff);
              if ( ((addr &0x3f) == 020) || ((addr &0x3f) == 024) )
                {
                  // device code is SMK
                  if (i != 0171020) // OTK
                    {
                      if (ip->get_opcode() & 0x40)
                        // opcode is flagged as SMK too 
                        dispatch_table[i] = ip;
                    }
                }
              else
                {
                  // device code is OTA
                  if (!(ip->get_opcode() & 0x40))
                    dispatch_table[i] = ip;
                }
            }
          break;
    
        case Instr::IOG: // IO instuction pretending to be Generic
          dispatch_table[ip->get_opcode()] = ip;
          break;
          
        default: abort();
        }
      ip++;
    } 
}

InstrTable::Instr InstrTable::ui("???", IT(UNDEFINED), 0, "Unimplemented", PD(unimplemented));
InstrTable::Instr InstrTable::gskp("skip?", IT(SH), 0, "Generic skip", PD(generic_skip));
InstrTable::Instr InstrTable::gshf("shft?", IT(SH), 0, "Generic shift", PD(generic_shift));
InstrTable::Instr InstrTable::gena("gnrc?", IT(SH), 0, "Generic group A", PD(generic_group_A));


void InstrTable::build_instr_tables()
{
  unsigned long lim, i;

  lim = 1 << 16;

  dispatch_table = new Instr *[lim];
  //
  // First set all instructions to unimplemented
  //

  for (i=0; i<lim; i++)
    dispatch_table[i] = &ui;
  
  i = 0;
  while (instructions[i])
    {
      build_one_instr_table(instructions[i]);
      i++;
    }

#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
  //
  // Many Group A instructions have alternate opcodes that
  // produce the same actions
  //
 apply_aliases();
#endif

  //
  // Now deal with the instructions that we have the ability
  // to deal with as groups; so that "microcoding" of the
  // machine works right. Acording to 
  // "Micro-coding the DDP-516 computer, Donald A Bell
  //  NPL Com. Sci. T.M. 54 April 1971"
  //

  // Skip...
  for (i=0x8000; i<0x8400; i++)
    if (dispatch_table[i] == &ui)
      dispatch_table[i] = &gskp;

  // Shift...
  for (i=0x4000; i<0x4400; i++)
    if (dispatch_table[i] == &ui)
      dispatch_table[i] = &gshf;

  // Generic group A...
  for (i=0xc000; i<0xc400; i++)
    if (dispatch_table[i] == &ui)
      dispatch_table[i] = &gena;
}


const char *InstrTable::disassemble(unsigned short addr,
                                    unsigned short instr,
                                    bool brk,
                                    unsigned short y,
                                    bool y_valid)
{
  return dispatch_table[instr]->disassemble(addr, instr, brk, y, y_valid);
}

InstrTable::Instr *InstrTable::lookup(const char *mnemonic) const
{
  // This could be much more efficient, but for now...
  int i;

  for (i=0; i<65536; i++)
    {
      const char *m = dispatch_table[i]->get_mnemonic();
      if ((m) && (strcmp(mnemonic, m) == 0))
        return dispatch_table[i];
    }
  return 0;
}

void InstrTable::dump_dispatch_table() const
{
  int i;
  for (i=0; i<65536; i++)
    {
      printf("%06o %s\n", i, dispatch_table[i]->get_mnemonic());
    }
}

#if ((!defined(GENERIC_GROUP_A)) || defined(TEST_GENERIC_GROUP_A))
#define LAST ((unsigned short) 0177777)

unsigned short InstrTable::HLT_alias[] =
  { 0000000, 0000002, 0000012, LAST }; 
unsigned short InstrTable::CMA_alias[] =
  {
    0140001, 0140003, 0140005, 0140007, 0140011, 0140013, 0140015, 0140017,
    0140021, 0140022, 0140023, 0140025, 0140026, 0140027, 0140031, 0140032,
    0140033, 0140035, 0140036, 0140037, 0140101, 0140103, 0140105, 0140107,
    0140111, 0140113, 0140115, 0140117, 0140401, 0140405, 0140411, 0140415,
    0140421, 0140425, 0140431, 0140435, 0140501, 0140505, 0140511, 0140515,
    LAST
  };
unsigned short InstrTable::CRA_alias[] =
  {
    0140002, 0140006, 0140040, 0140060, 0140102, 0140106, 0140440, 0140460,
    LAST
  };
unsigned short InstrTable::SSM_alias[] =
  {
    0140004, 0140014, 0140104, 0140114, 0140404, 0140414, 0140500, 0140504,
    0140510, 0140514, LAST
  };
unsigned short InstrTable::CHS_alias[] =
  {
    0140024, 0140034, 0140424, 0140434, LAST
  };
unsigned short InstrTable::InstrTable::CAR_alias[] =
  {
    0140044, 0140064, 0140444, 0140464, LAST
  };
unsigned short InstrTable::CAL_alias[] =
  {
    0140050, 0140070, 0140450, 0140470, LAST
  };
unsigned short InstrTable::SSP_alias[] =
  {
    0140100, 0140110, LAST
  };
unsigned short InstrTable::ICL_alias[] =
  {
    0140140, LAST
  };
unsigned short InstrTable::ICR_alias[] =
  {
    0140240, 0140260, LAST
  };
unsigned short InstrTable::RCB_alias[] =
  {
    0140200, 0140201, 0140203, 0140204, 0140205, 0140207, 0140210, 0140211,
    0140213, 0140214, 0140215, 0140217, 0140220, 0140221, 0140222, 0140223,
    0140224, 0140225, 0140226, 0140227, 0140230, 0140231, 0140232, 0140233,
    0140234, 0140235, 0140236, 0140237, 0140301, 0140303, 0140304, 0140305,
    0140307, 0140311, 0140313, 0140314, 0140315, 0140317, LAST
  };
unsigned short InstrTable::CSA_alias[] =
  {
    0140320, 0140330, LAST
  };
unsigned short InstrTable::TCA_alias[] =
  {
    0140403, 0140407, 0140422, 0140423, 0140426, 0140427, 0140503, 0140507,
    LAST
  };
unsigned short InstrTable::ICA_alias[] =
  {
    0140340, LAST
  };
unsigned short InstrTable::SCB_alias[] =
  {
    0140600, 0140601, 0140604, 0140605, 0140610, 0140611, 0140614, 0140615,
    0140620, 0140621, 0140624, 0140625, 0140630, 0140631, 0140634, 0140635,
    0140700, 0140701, 0140704, 0140705, 0140710, 0140711, 0140714, 0140715,
    0140720, 0140721, 0140724, 0140725, 0140730, 0140731, 0140734, 0140735,
    LAST
  };
unsigned short InstrTable::AOA_alias[] =
  {
    0140202, 0140206, 0140302, 0140306, LAST
  };
unsigned short InstrTable::AD1_alias[] =
  {
    0140042, 0140046,
    LAST
  };
unsigned short InstrTable::AD1_15_alias[] =
  {
    0140443, 0140447, 0140462, 0140463, 0140466, 0140467,
    LAST
  };
unsigned short InstrTable::ACA_alias[] =
  {
    0140212, 0140216, 0140312, 0140316, LAST
  };
unsigned short InstrTable::ADC_alias[] =
  {
    0140052, 0140056, LAST
  };
unsigned short InstrTable::ADC_15_alias[] =
  {
    0140453, 0140457, 0140472, 0140473, 0140476, 0140477, LAST
  };
unsigned short InstrTable::CM1_alias[] =
  {
    0140012, 0140016, 0140112, 0140116, LAST
  };
unsigned short InstrTable::LTR_alias[] =
  {
    0140144, 0140544, LAST
  };
unsigned short InstrTable::BTR_alias[] =
  {
    0140141, 0140143, 0140145, 0140147, 0140151, 0140153, 0140154, 0140155,
    0140157, 0140541, 0140545, 0140551, 0140554, 0140555, LAST
  };
unsigned short InstrTable::BTL_alias[] =
  {
    0140241, 0140243, 0140245, 0140247, 0140251, 0140253, 0140254, 0140255,
    0140257, 0140261, 0140262, 0140263, 0140265, 0140266, 0140267, 0140271,
    0140272, 0140273, 0140274, 0140275, 0140276, 0140277, LAST
  };
unsigned short InstrTable::RTL_alias[] =
  {
    0140250, 0140270, LAST
  };
unsigned short InstrTable::RCB_SSP_alias[] =
  {
    0140300, 0140310, LAST
  };
unsigned short InstrTable::CPY_alias[] =
  {
    0140321, 0140322, 0140323, 0140324, 0140325, 0140326, 0140327, 0140331,
    0140332, 0140333, 0140334, 0140335, 0140336, 0140337, LAST
  };
unsigned short InstrTable::BTB_alias[] =
  {
    0140341, 0140343, 0140345, 0140347, 0140351, 0140353, 0140354, 0140355,
    0140357, LAST
  };
unsigned short InstrTable::BCL_alias[] =
  {
    0140150, LAST
  };
unsigned short InstrTable::BCR_alias[] =
  {
    0140244, 0140264, LAST
  };
unsigned short InstrTable::LD1_alias[] =
  {
    0140402, 0140406, 0140502, 0140506, LAST
  };
unsigned short InstrTable::ISG_alias[] =
  {
    0140412, 0140416, 0140512, 0140516, LAST
  };
unsigned short InstrTable::CMA_ACA_alias[] =
  {
    0140413, 0140417, 0140432, 0140433, 0140436, 0140437, 0140513, 0140517,
    LAST
  };
unsigned short InstrTable::CMA_ACA_C_alias[] =
  {
    0140532, 0140533, 0140536, 0140537, LAST
  };
unsigned short InstrTable::A2A_alias[] =
  {
    0140442, 0140446, LAST
  };
unsigned short InstrTable::A2C_alias[] =
  {
    0140452, 0140456, LAST
  };
unsigned short InstrTable::ICS_alias[] =
  {
    0140540, LAST
  };
unsigned short InstrTable::SCB_A2A_alias[] =
  {
    0140602, 0140606, 0140702, 0140706, LAST
  };
unsigned short InstrTable::SCB_AOA_alias[] =
  {
    0140603, 0140607, 0140622, 0140623, 0140626, 0140627, 0140703, 0140707,
    0140722, 0140723, 0140726, 0140727, LAST
  };
unsigned short InstrTable::A2C_SCB_alias[] =
  {
    //0143612, 0140616, 0140712, 0140716, LAST
    0140612, 0140616, 0140712, 0140716, LAST
  };
unsigned short InstrTable::ACA_SCB_alias[] =
  {
    0140613, 0140617, 0140632, 0140633, 0140636, 0140637, 0140713, 0140717,
    0140732, 0140733, 0140736, 0140737, LAST
  };
unsigned short InstrTable::ICR_SCB_alias[] =
  {
    0140640, 0140660, LAST
  };
unsigned short InstrTable::RTL_SCB_alias[] =
  {
    0140650, 0140670, LAST
  };
unsigned short InstrTable::BTB_SCB_alias[] =
  {
    0140741, 0140745, 0140751, 0140754, 0140755, 0140761, 0140765, 0140771,
    0140774, 0140775, LAST
  };
unsigned short InstrTable::NOA_alias[] =
  {
    0140000, 0140010, 0140020, 0140030, 0140041, 0140043, 0140045, 0140047,
    0140051, 0140053, 0140054, 0140055, 0140057, 0140061, 0140062, 0140063,
    0140065, 0140066, 0140067, 0140071, 0140072, 0140073, 0140074, 0140075,
    0140076, 0140077, 0140400, 0140410, 0140420, 0140430, 0140441, 0140445,
    0140451, 0140454, 0140455, 0140461, 0140465, 0140471, 0140474, 0140475,
    LAST
  };



unsigned short *InstrTable::aliases[] = 
  {
    HLT_alias,
    CMA_alias,
    CRA_alias,
    SSM_alias,
    CHS_alias,
    CAR_alias,
    CAL_alias,
    SSP_alias,
    ICL_alias,
    ICR_alias,
    RCB_alias,
    CSA_alias,
    TCA_alias,
    ICA_alias,
    SCB_alias,
    AOA_alias,
    AD1_alias,
    AD1_15_alias,
    ACA_alias,
    ADC_alias,
    ADC_15_alias,
    CM1_alias,
    LTR_alias,
    BTR_alias,
    BTL_alias,
    RTL_alias,
    RCB_SSP_alias,
    CPY_alias,
    BTB_alias,
    BCL_alias,
    BCR_alias,
    LD1_alias,
    ISG_alias,
    CMA_ACA_alias,
    CMA_ACA_C_alias,
    A2A_alias,
    A2C_alias,
    ICS_alias,
    SCB_A2A_alias,
    SCB_AOA_alias,
    A2C_SCB_alias,
    ACA_SCB_alias,
    ICR_SCB_alias,
    RTL_SCB_alias,
    BTB_SCB_alias,
    NOA_alias,
  
    NULL
  };

//
// Generic group A instructions do the
// same whether or not bit 7 is set
//
#define B7 0001000

void InstrTable::apply_one_alias( unsigned short alias[] )
{
  int a;
  unsigned short limit;
  unsigned short i, proper_i;
  char temp_mnemonic[256], temp_description[256];
  
  if ((alias[0] & 0140000) == 0140000)
    limit = B7; // This is a group A instruction
  else
    limit = 0;

  //
  // The first step is to find one of the opcodes in the
  // list of aliased instructions that has been defined
  // already.
  //

  proper_i = LAST;
  for (a=0; (alias[a] != LAST); a++)
    {
      for (i=alias[a]; i<=(alias[a]+limit); i+=B7)
        if (dispatch_table[i]->get_type() != Instr::UNDEFINED)
          {
            if (proper_i==LAST)
              proper_i = i;
            else
              {
                fprintf(stderr, "This alias spans two instructions (%0o); ",
                        alias[a]);
                fprintf(stderr, "%s and %s\n",
                        dispatch_table[proper_i]->get_mnemonic(),
                        dispatch_table[i]->get_mnemonic());
                exit(1);
              }
          }
    }
  
  if (proper_i==LAST)
    {
      fprintf(stderr,"Failed to find '%06o\n", alias[0]);
      exit(1);
    }

  //
  // Having established that there is precisely one
  // "proper" instruction for this alias list create a
  // string to represent the mnemonic and description
  //

  sprintf(temp_mnemonic, "(%s)", dispatch_table[proper_i]->get_mnemonic());
  sprintf(temp_description, "(Alternative) %s", dispatch_table[proper_i]->get_description());

  for (a=0; (alias[a] != LAST); a++) {
    for (i=alias[a]; i<=(alias[a]+limit); i+=B7)
      if (i != proper_i)
        // for all but the proper instruction
        // create a new Instr structure
            
        dispatch_table[i] = new Instr(strdup(temp_mnemonic),
                                      dispatch_table[proper_i]->get_type(),
                                      i,
                                      strdup(temp_description),
                                      dispatch_table[proper_i]->get_exec(),
                                      true);
            
    }
  
}


void InstrTable::apply_aliases()
{
  int aa;
  for (aa=0; aliases[aa]; aa++)
    apply_one_alias(aliases[aa]);
}

#endif
