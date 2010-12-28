/*
 * Honeywell Series 16 emulator - convert a teletype output to ASCII
 *
 * Copyright (C) 1997, 2006  Adrian Wise
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

#define USAGE "usage: %s [-h|--help] <TTY filename> <ASCII filename>\n"

int main(int argc, char **argv)
{
  int a = 1;
  int help = 0;
  int usage = 0;
  FILE *fpi, *fpo;
  int c, d;

  while (argv[a]) {
    if ((strncmp(argv[a], "-h", 2)==0) || (strncmp(argv[a], "--h", 3)==0))
      {
        help = 1;
        break;
      }
    else
      break;
  }
  
  if (help) {
    printf(USAGE, argv[0]);
    printf("Options:\n");
    printf("-h|--help  Print this help and exit\n");
    printf("\n");
    printf("           LF-CR replaced by newline,\n");
    printf("           other control codes are deleted.\n");
    printf("           Most significant bit of all characters forced to 0.\n");
    exit(0);
  }

  if ((argc-a) != 2)
    usage = 1;

  if (usage) {
    fprintf(stderr, USAGE, argv[0]);
    exit(1);
  }

  fpi = fopen(argv[a], "rb");
  if (!fpi) {
    fprintf(stderr, "Could not open <%s> for reading\n",
            argv[a]);
    exit(1);
  }
  a++;

  fpo = fopen(argv[a], "w");
  if (!fpi) {
    fprintf(stderr, "Could not open <%s> for writing\n",
            argv[a]);
    exit(1);
  }

  c = getc(fpi);

  while (c != EOF) {
    c = c & 0177;  /* lose the top bit */
    d = 0;

    if ((c < 0040) || (c == 0177)) {
      /* replace LF by newline (let local OS where this
         code is running worry about how to represent \n) */
      if (c == 0012)
        d = '\n';

      /* CR and other ctrl chars are ignored, including
       * X-OFF, RUBOUT that may occur at the end of each source line
       * (for ASR reader control) and the EOM record, if any. */
    }
    else
      d = c;
      
    if (d > 0)
      putc(d, fpo);
    c = getc(fpi);
  }

  fclose(fpi);
  fclose(fpo);

  exit(0);
}


