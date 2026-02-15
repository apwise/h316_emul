/* Honeywell Series 16 emulator
 * Copyright (C) 2010, 2026  Adrian Wise
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

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstring>

#include <iostream>
#include <fstream>

#include <utility>
#include <list>
#include <set>
#include <map>

#include "instr.hpp"

static InstrTable instr_table;

static int fix_c(int &c)
{
  int uc = c & 0xff; // Unsigned
  int r = ' ';
  if (uc & 0x80) {
    uc &= 0x7f;
    if ((uc>=0x20) && (uc<0x7f))
      r = uc;
  }
  return r;
}

int main(int argc, char **argv)
{
  FILE *fp;
  int a = 1; // First argument to parse
  int c, c1, c2;
  int fc1, fc2;
  int full_addr;
  unsigned short addr, instr;
  const char *str;

  if (a != (argc-1)) {
    fprintf(stderr, "usage: %s <filename>\n",
            argv[0]);
    exit(1);
  }
  
  fp = fopen(argv[a], "rb");
  if (!fp) {
    fprintf(stderr, "Could not open <%s> for reading\n",
            argv[a]);
    exit(1);
  }
  
  full_addr = 0;
  do {
    c = c1 = getc(fp);
    if (c1 != EOF) {
      instr = (c1 & 0xff) << 8;
      c = c2 = getc(fp);
      if (c2 != EOF) {
        instr |= (c2 & 0xff);

        addr = full_addr & 0x7fff;

        str = instr_table.disassemble(addr, instr, false);

        fc1 = fix_c(c1);
        fc2 = fix_c(c2);

        printf("%06x %06o %03o %03o %c%c %02o%s\n",
               full_addr, (((int) instr) & 0xffff), (c1 & 0xff), (c2 & 0xff),
               ((char) fc1), ((char) fc2), ((full_addr >> 15) & 077), &str[1]);

        full_addr++;
      }
    }
  } while (c != EOF);
 
  fclose(fp);
  
  exit(0);
}
