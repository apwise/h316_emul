/*
 * Honeywell Series 16 emulator - convert a ASCII to teletype forced-parity
 *
 * Copyright (C) 1997, 2006, 2020  Adrian Wise
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
#include <cstring>

#include "tty_file.hh"
#define USAGE "usage: %s [-h|--help] <ASCII filename> <TTY filename>\n"

int main(int argc, char **argv)
{
  int usage = 0;
  int help = 0;
  int a = 1;
  TTY_file tty;
  FILE *fpo;
  int c;
  
  if ((argc > 1) &&
      ((strncmp(argv[a], "-h", 2)==0) || (strncmp(argv[a], "--h", 3)==0))) {
    help = 1;
  }
  
  if (help) {
    printf(USAGE, argv[0]);
    printf("Options:\n");
    printf("-h|--help  Print this help and exit\n");
    printf("\n");
    printf("           Lower-case letters are replaced by upper-case.\n");
    printf("           Newline replaced by <CR>-<XOFF>-<RUBOUT>-<LF>.\n");
    printf("           Most significant bit of all characters forced to 1.\n");
    printf("           End of file marked by <EOM>-XOFF>-<RUBOUT>.\n");
    exit(0);
  }

  if ((argc-a) != 2)
    usage = 1;

  if (usage) {
    fprintf(stderr, USAGE, argv[0]);
    exit(1);
  }

  tty.open(argv[a], tty.READ_ASCII);
  if (!tty.is_open()) {
    fprintf(stderr, "Could not open <%s> for reading\n",
            argv[a]);
    exit(1);
  }

  a++;
  fpo = fopen(argv[a], "wb");
  if (!fpo) {
    fprintf(stderr, "Could not open <%s> for writing\n",
            argv[a]);
    exit(1);
  }

  c = tty.getc();

  while (c != EOF) {
    putc(c, fpo);
    c = tty.getc();
  }

  tty.close();
  fclose(fpo);

  exit(0);
}


