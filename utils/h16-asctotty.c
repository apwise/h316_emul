/*
 * Honeywell Series 16 emulator - convert a ASCII to teletype forced-parity
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
#include <string.h>

#define EOM  0203
#define LF   0212
#define CR   0215
#define XOFF 0223
#define RUBOUT 0377

#define USAGE "usage: %s [-h|--help] [-c] <ASCII filename> <TTY filename>\n"

int main(int argc, char **argv)
{
  FILE *fpi, *fpo;
  int c, d;
  int usage = 0;
  int help = 0;
  int a = 1;
  int add_eom = 0;
  int asr = 0;

  while (argv[a]) {
    if ((strncmp(argv[a], "-h", 2)==0) || (strncmp(argv[a], "--h", 3)==0)) {
      help = 1;
      break;
    } else if (strcmp(argv[a], "-a")==0) {
      asr = 1;
      a++;
    } else if (strcmp(argv[a], "-c")==0) {
      add_eom = 1;
      a++;
    } else
      break;
  }
  
  if (help) {
    printf(USAGE, argv[0]);
    printf("Options:\n");
    printf("-h|--help  Print this help and exit\n");
    printf("-a         Add <X-OFF><RUBOUT> to end of each record,\n");
    printf("           suitable for reading on an ASR.\n");
    printf("-c         Add <EOM> record at end of file\n");
    printf("\n");
    printf("           Lower-case letters are replaced by upper-case.\n");
    printf("           Newline replaced by LF-CR,\n");
    printf("           other control codes are deleted.\n");
    printf("           Most significant bit of all characters forced to 1.\n");
    exit(0);
  }

  if ((argc-a) != 2)
    usage = 1;

  if (usage) {
    fprintf(stderr, USAGE, argv[0]);
    exit(1);
  }

  fpi = fopen(argv[a], "r");
  if (!fpi) {
    fprintf(stderr, "Could not open <%s> for reading\n",
            argv[a]);
    exit(1);
  }

  a++;
  fpo = fopen(argv[a], "wb");
  if (!fpi) {
    fprintf(stderr, "Could not open <%s> for writing\n",
            argv[a]);
    exit(1);
  }

  c = getc(fpi);

  while (c != EOF) {
    c = c & 0177;  /* loose the top bit */
    d = 0;
			
    if (c < 0040) {
      /* Only control code allowed is newline */
      if (c == '\n')
        d = '\n';
    } else if ((c >= 'a') && (c <= 'z'))
      d = c - ('a'-'A'); /* fold lower case to upper */
    else
      d = c; /* Printing capitals are unaffected */
      
    if (d > 0) {
      if (d == '\n') {
        putc( LF, fpo); /* Line Feed Text */
        putc( CR, fpo); /* Carriage Return */
        if (asr) {
          putc( XOFF,   fpo); /* X-OFF (Control S) */
          putc( RUBOUT, fpo); /* RUBOUT */
        }
      } else {
        putc((d | 0200), fpo);
      }
    }
    c = getc(fpi);
  }
	
  if (add_eom) {
    putc( EOM, fpo);
    if (asr) {
      putc( XOFF,   fpo); /* X-OFF (Control S) */
      putc( RUBOUT, fpo); /* RUBOUT */
    }
  }

  fclose(fpi);
  fclose(fpo);

  exit(0);
}


