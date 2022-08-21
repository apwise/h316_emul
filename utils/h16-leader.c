#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/*
 * usage:
 *
 * h16-leader [-h | --help]
 *
 * h16-leader [-l num] [-t num] [-e] [-o output-filename] [input-filename]
 *
 * h16-leader [-l num] [-t num] [-e] input-filename output-filename
 *
 * -l : number of leader  bytes (default zero)
 * -t : number of trailer bytes (default same as no. leader bytes)
 * -e : add EOT mark after content (unless there already is one) 
 *
 * By default, strip the nul-byte leader and trailer (of a papertape
 * file). Optionally, set the amount of leader and trailer to known
 * lengths.
 *
 * The motivation is to allow simple binary comparison of papertape
 * files.
 */

int output_leader(int num, FILE *fout)
{
  int r = 0;
  int i;
  for (i=0; i<num; i++) {
    if (putc(0, fout) < 0) {
      r = 2;
      break;
    }
  }
  return r;
}

int add_eot(FILE *fout)
{
  int r = 0;
  if (putc(0203, fout) < 0) {
    r = 2;
  } else if (putc(0223, fout) < 0) {
    r = 2;
  } else if (putc(0377, fout) < 0) {
    r = 2;
  }
  return r;
}

int strip_leader(FILE *fin, FILE *fout, bool *p_eot)
{
  int r = 0;
  int c = getc(fin);
  int zeros = 0;
  enum EOT
  {
   EOT0, EOT1, EOT2, EOT3
  } eot = EOT0;
  
  // strip the leader
  while (c == 0) {
    c = getc(fin);
  }

  while ((c != EOF) && (r == 0)) {
    if (c == 0) {
      zeros++;
    } else {
      if (zeros > 0) {
        r = output_leader(zeros, fout);
        zeros = 0;
        eot = EOT0;
      }
      switch(eot) {
      case EOT3:
      case EOT0: eot = (c==0203) ? EOT1 : EOT0; break;
      case EOT1: eot = (c==0223) ? EOT2 : EOT0; break;
      case EOT2: eot = (c==0377) ? EOT3 : EOT0; break;
      }
      if (putc(c, fout) < 0) {
        r = 2;
      }
    }
    if (r == 0) {
      c = getc(fin);
    }
  }

  if (p_eot) {
    *p_eot = ((r==0) && (eot==EOT3));
  }
  return r;
}

int number(const char *str, int *usage)
{
  unsigned long u;
  char *endptr;
  
  u = strtoul(str, &endptr, 0);

  if (((endptr-str) != strlen(str)) ||
      (u > 10000)) {
    if (usage) *usage = 3;
    u = 0;
  }
  return u;
}

int main(int argc, char **argv)
{
  int i = 1;
  int usage = 0;
  int fnargs = 0;
  char *input_name = NULL;
  char *output_name = NULL;
  FILE *fin = stdin;
  FILE *fout = stdout;
  int leader = 0;
  int trailer = -1;
  int r = 0;
  bool ensure_eot = false;
  bool eot = false;
  
  while ((i < argc) && (!usage)) {
    if ((strcmp(argv[i], "-h"    ) == 0) ||
        (strcmp(argv[i], "--help") == 0)) {
      usage = 1;
    } else if (strcmp(argv[i], "-o") == 0) {
      i++;
      if ((i < argc) && (!output_name)) {
        output_name = argv[i];
      } else {
        usage = 1;
      }
    } else if (strcmp(argv[i], "-l") == 0) {
      i++;
      if (i < argc) {
        leader = number(argv[i], &usage);
      }
    } else if (strcmp(argv[i], "-t") == 0) {
      i++;
      if (i < argc) {
        trailer = number(argv[i], &usage);
      }
    } else if (strcmp(argv[i], "-e") == 0) {
     ensure_eot = true;
    } else {
      if (argv[i][0] == '-') {
        // An unrecognized option
        usage = 1;
      }
      // Treat as a filename argument
      if (fnargs == 0) {
        // This should be an input filename
        if ((output_name) && ((i+1) < argc)) {
          // There's extra arguments
          usage = 1;
        }
        input_name = argv[i];
        fnargs++;
      } else if (fnargs == 1) {
        // This should be an output filename
        if (output_name) {
          // Already got one (-o argument)
          usage = 1;
        }
        output_name = argv[i];
        fnargs++;
      } else {
        // Too many arguments
        usage = 1;
      }
    }
    i++;
  }

  if (usage) {
    fprintf(stderr, "%s [-h | --help] | \\\n"
            "%s [-l num] [-t num] [-e] [-o output-filename] [input-filename] | \\\n"
            "%s [-l num] [-t num] [-e] input-filename output-filename\n"
            "-l : number of leader  bytes (default zero)\n"
            "-t : number of trailer bytes (default same as no. leader bytes)\n"
            "-e : add EOT mark after content (unless there already is one)\n", 
            argv[0], argv[0], argv[0]);
    exit(1);
  }

  // If -t option not given, default to same as leader
  // i.e. default zero, or the value given in -l option
  if (trailer < 0) {
    trailer = leader;
  }
  
  if (input_name) {
    fin = fopen(input_name, "rb");
    if (!fin) {
      fprintf(stderr, "Failed to open <%s> for input\n", input_name);
    }
  }
  
  if (output_name) {
    fout = fopen(output_name, "wb");
    if (!fout) {
      fprintf(stderr, "Failed to open <%s> for output\n", output_name);
    }
  }

  if (leader > 0) {
    r = output_leader(leader, fout);
  }
  
  if (r == 0) {
    r = strip_leader(fin, fout, &eot);
  }

  if ((r==0) && (ensure_eot) && (! eot)) {
    r = add_eot(fout);
  }

  if ((r == 0) && (trailer > 0)) {
    r = output_leader(trailer, fout);
  }

  exit(r);
}
