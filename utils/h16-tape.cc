/* Analyse an X16 papertape file
 *
 * Copyright (C) 2006, 2008, 2012  Adrian Wise
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <iostream>
#include <fstream>

#include <utility>
#include <list>
#include <set>
#include <map>

#include "instr.hh"

#define TAPE_INITIAL_BUFFER_SIZE 1024

class Tape
{
public:
  Tape(char *filename=0);
  ~Tape();

  void read_file(char *filename);

private:
  unsigned char *buffer;
  unsigned long buffer_size;
  unsigned long buffer_used;
  void double_buffer_size();
  void add_to_buffer(unsigned char c);
};

// {{{ Tape::Tape(char *filename)

Tape::Tape(char *filename)
  : buffer(0),
    buffer_size(0),
    buffer_used(0)
{
  buffer = new unsigned char [buffer_size];

  if (filename)
    read_file(filename);
}

// }}}
// {{{ Tape::~Tape()

Tape::~Tape()
{
  if (buffer)
    delete [] buffer;
}

// }}}

// {{{ void Tape::double_buffer_size()

void Tape::double_buffer_size()
{
  unsigned char *new_buffer = 0;
  unsigned long p = 0;

  buffer_size = (buffer_size) ? (buffer_size * 2) : TAPE_INITIAL_BUFFER_SIZE;
  new_buffer = new unsigned char [buffer_size];

  if (buffer)
    {
      for (p=0; p<buffer_used; p++)
        new_buffer[p] = buffer[p];
      
      delete [] buffer;
    }
  
  buffer = new_buffer;
}

// }}}
// {{{ void Tape::add_to_buffer(unsigned char c)

void Tape::add_to_buffer(unsigned char c)
{
  if (buffer_used >= buffer_size)
    double_buffer_size();

  buffer[buffer_used++] = c;
}

// }}}
// {{{ void Tape::read_file(char *filename)

void Tape::read_file(char *filename)
{
  std::ifstream ifs(filename);

  while (ifs.good())
    add_to_buffer(ifs.get());

  ifs.close();
}

// }}}

static InstrTable *instr_table;

// {{{ static int gc(FILE *fp)

static int gc(FILE *fp)
{
  int c = getc(fp);

  //printf("gc(): c = %03o\n", c);

  if (c==EOF)
    {
      fprintf(stderr, "Unexpected EOF\n");
      exit(1);
    }
  return c&0xff;
}

// }}}
// {{{ static unsigned short read_8_8(FILE *fp)

static unsigned short read_8_8(FILE *fp)
{
  return (gc(fp) << 8) | gc(fp);
}

// }}}
// {{{ static void skip_leader(FILE *fp)

static void skip_leader(FILE *fp)
{
  int c=0;

  while (c==0)
    c = gc(fp);

  ungetc(c, fp);
}

// }}}
// {{{ static int read_8_8_block(FILE *fp, unsigned short *buffer)

static int read_8_8_block(FILE *fp, unsigned short *buffer)
{
  int i=0;
  unsigned short n;

  skip_leader(fp);

  while ((n=read_8_8(fp)) != 0)
    {
      buffer[i] = n;
      /*printf("%06o %06o\n", i, buffer[i]);*/
      i++;
    }
  return i;
}

// }}}
// {{{ static void read_expected(FILE *fp, int expect)

static void read_expected(FILE *fp, int expect)
{
  int c;
  c = gc(fp);

  if (c!=expect)
    {
      fprintf(stderr, "Expected %03o read %03o\n", expect, c);
      //exit(1);
    }
  /*printf("read %03o OK\n", c);*/
} 

// }}}
// {{{ static int translate(int c, bool &xof)

static int translate(int c, bool &xof)
{
  int n;

  if (c == 0223)
    {
      fprintf(stderr, "XOF mid-word\n");
      xof = 1;
      //exit(1);
    }

   switch(c & 0x7f)
    {
    case 0177: n = 023 | (c & 0x80); break;
    case 0176: n = 021 | (c & 0x80); break;
    case 0175: n = 012 | (c & 0x80); break;
    case 0174: n = 005 | (c & 0x80); break;
    default: n = c;
    }

  if (n & 0x60)
    {
      fprintf(stderr, "Channels 6 or 7 punched (%03o)\n", n);
      exit(1);
    }

  n = ((n&0x80)>>2) | (n & 0x1f);
  /*printf("translate %03o to %03o\n", c, n);*/

  return n;
}

// }}}
// {{{ static unsigned short read_silent(FILE *fp, bool &zero_flag, bool &xof)

static unsigned short read_silent(FILE *fp, bool &zero_flag, bool &xof)
{
  int c;
  unsigned short n;

  c = gc(fp);
  //printf("read_silent(): c = %03o\n", c);

  if (c == 0223)
    {
      xof = 1;
      return 0;
    }
  else
    xof = 0;

  if (c & 0x80)
    {
      zero_flag = 1;
      c &= 0x7f;
    }
  else
    zero_flag = 0;

  c = translate(c, xof);
  if (c & 0x10)
    {
      fprintf(stderr, "Channel 5 punched in 4-bit character\n");
      exit(1);
    }
  n = (c << 12);

  n |= translate(gc(fp), xof) << 6;
  n |= translate(gc(fp), xof);

  return n;
}

// }}}

static unsigned short core_low = 0xffff;
static unsigned short core_high = 0;

// {{{ static int read_silent_block(FILE *fp, int find_start,

static int read_silent_block(FILE *fp, int find_start,
                             unsigned short *core,
                             unsigned short bc)
{
  unsigned short addr, a, n;
  bool zero_flag, xof;
  unsigned short checksum = bc;
  unsigned short old_checksum = checksum;
  int wc = 0;
  int i, c;
  skip_leader(fp);

  c = gc(fp);

  if (c==0223)
    return 0;

  if (find_start)
    {
      while (c != 0201)
        c = gc(fp);
    }
  else if (c!=0201)
    {
      fprintf(stderr, "Expected %03o read %03o\n", 0201, c);
      exit(1);
    }

  addr = read_silent(fp, zero_flag, xof);

  checksum ^= addr;
  checksum = ((checksum & 1) << 15) |
    ((checksum >> 1) & 0x7fff);
  addr++;
  a = addr;

  if (zero_flag)
    {
      fprintf(stderr, "Zero flag on address\n");
      exit(1);
    }

  /*printf("addr = %06o\n", addr);*/

  n = read_silent(fp, zero_flag, xof);
  while (!xof)
    {
      if (zero_flag)
        {
          i = n | (-1<<16);
          while (i<0)
            {
              core[a++] = 0;
              i++;
              wc++;
            }
        }
      else
        {
          core[a++] = n;
          wc++;
        }
      old_checksum = checksum;
      checksum ^= n;
      checksum = ((checksum & 1) << 15) |
        ((checksum >> 1) & 0x7fff);
      
      n = read_silent(fp, zero_flag, xof);
    }
  
  read_expected(fp, 0377);
  
  checksum = core[--a];  /* from tape */
  old_checksum ^= wc; /* calculated here */
  
  printf("Block %03d %06o to %06o %06o %s\n",
   bc, addr, a-1, (a-addr),
   (checksum != old_checksum) ? "Bad checksum\n":"");

  if (checksum != old_checksum)
    printf("Expected: %06o Actual: %06o XOR: %06o\n",
           old_checksum, checksum, (old_checksum ^ checksum));

  if (addr < core_low) core_low = addr;
  if ((a-1) > core_high) core_high = (a-1);

  return 1;
}

// }}}
// {{{ static unsigned char asr_char(unsigned char c)

static unsigned char asr_char(unsigned char c)
{
  if ((c >= 0240) && (c <= 0337))
    return c & 0x7f;
  else if ((c==212) || (c==215))
    return ' ';
  else
    return 0;
}

// }}}
// {{{ static char *printable(unsigned short instr)

static char *printable(unsigned short instr)
{
  static char temp[5];
  
  temp[1] = (char) asr_char((instr >> 8) & 0xff);
  temp[2] = (char) asr_char(instr & 0xff);

  if (temp[1] && temp[2])
      temp[0] = temp[3] = '"';
  else
    temp[0] = '\0';
  temp[4]= '\0';

  return temp;
}

// }}}

static bool verilog = false;

// {{{ static void dissassemble_block(unsigned short *block, int size, int addr)

static void dissassemble_block(unsigned short *block, int size, int addr)
{
  int i;

  if (verilog)
    printf("@%04x\n", addr & 0xffff);
  else
    printf("{\n");

  for (i=0; i<size;i++)
    {
      if (verilog)
        printf("%04x /* %-35s %-4s */\n",
               block[i],
               instr_table->disassemble((i+addr), block[i], false),
               printable(block[i]));
      else
        printf("  0%06o%c /* %-35s %-4s */\n",
               block[i],
               ((i ==(size-1)) ? ' ' : ','),
               instr_table->disassemble((i+addr), block[i], false),
               printable(block[i]));

    }

  if (!verilog)
    printf("}\n");
}

// }}}

#define K64 (1 << 16)
#define K32 (1 << 15)
#define K16 (1 << 14)

enum WORD_INFO {
  WI_NULL = 0,
  WI_EXEC = 1  // Can be reached as code
};

/*
static unsigned int instr_may_reach(uint16_t addr, uint16_t instr,
                                    uint16_t addresses[3])
{
  return 0;
}

static void control_flow_analysis(const uint16_t core[],
                                  const std::list<uint16_t> entry_points,
                                  std::map<uint16_t, WORD_INFO> &info)
{
  std::pair<std::set<uint16_t>::iterator, bool> sr;

  // The set of addresses that need to be analysed...
  std::set<uint16_t> analyse;

  // Start by clearing the info structure
  info.clear();

  // Add all of the entry points
  std::list<uint16_t>::const_iterator li;
  for (li = entry_points.begin(); li != entry_points.end(); li++) {
    sr = analyse.insert(*li);
    if (!sr.second) {
      std::cout << "Entry point <" 
                << std::oct << (*li) << std::dec 
                << "> already dealt with - duplicate ignored." 
                << std::endl;
    }
  }

  std::set<uint16_t>::iterator ai;
  ai = analyse.begin();
  while (ai != analyse.end()) {

    uint16_t instr = *ai;

    // So (obviously) this address is reachable by the
    // control flow
    info[instr] = WI_EXEC;

    ai = analyse.begin();
  }

}
*/

int main(int argc, char **argv)
{
  FILE *fp;
  int bootstrap_length;
  unsigned short bootstrap[256];
  int loader_length, loader_addr;
  unsigned short loader[256];
  //int palap = 0;
  int expect_at_least;

  unsigned short *core;
  int i;
  int bc=0;

  //debug = (struct INSTR **) malloc(sizeof(struct INSTR *) * K64);
  //dispatch = (void (proc::**)(unsigned short))
  //malloc(sizeof(void (proc::*)(unsigned short)) * K64);

  //instr_table.build_instr_tables();
  instr_table = new InstrTable;

  core = (unsigned short *) malloc(sizeof(unsigned short) * K32);
  for (i=0; i<K32; i++)
    core[i] = 0;

  int a = 1;
  bool parsing_args = true;

  while (parsing_args && (a < argc))
    {
      if (strcmp(argv[a], "-v")==0)
        {
          verilog = true;
          a++;
        }
      else
        parsing_args = false;
    }

  if (a != (argc-1))
    {
      fprintf(stderr, "usage: %s [-v] <filename>\n",
       argv[0]);
      exit(1);
    }

  fp = fopen(argv[a], "rb");
  if (!fp)
    {
      fprintf(stderr, "Could not open <%s> for reading\n",
        argv[a]);
      exit(1);
    }

  Tape tape(argv[a]);

  skip_leader(fp);
  read_expected(fp, 020);
  bootstrap_length = read_8_8_block(fp, bootstrap);
  printf("bootstrap_length = %d\n", bootstrap_length);
  dissassemble_block(bootstrap, bootstrap_length, 020);

  // assume some form of palap
  // palap = 1;
  loader_addr = bootstrap[054 - 020]+2;
  expect_at_least = 040;
  loader_length = 0;

  while (loader_length < expect_at_least )
    {
      loader_length = read_8_8_block(fp, loader + loader_length);
      loader[loader_length++] = 0;
    }
  loader_length --;

  printf("loader_length = %d\n", loader_length);
  dissassemble_block(loader, loader_length, loader_addr);

  read_expected(fp, 0);
  read_expected(fp, 0223);
  read_expected(fp, 0377);
  //dissassemble_block(loader, loader_length, 0602);

  do{}
    while(read_silent_block(fp, 1, core, bc++));

  dissassemble_block(core+core_low, ((core_high-core_low)+1), core_low);

  fclose(fp);

  delete instr_table;
  
  exit(0);
}


//Local variables:
//folded-file: t
//End:
