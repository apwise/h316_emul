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


//Local variables:
//folded-file: t
//End:
