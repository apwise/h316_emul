#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include "instr.hh"

/*
 * usage:
 *
 * h16-tabify [-c<tab-char>] [-o[ ]filename] [-l] [-] [<input-filename>]
 *
 */

static char tab_char = '\\';
static char *in_filename = 0;
static char *out_filename = 0;
static bool listing = false;

struct Pseudos
{
  const char *mnemonic;
  bool operands;
  bool is_mr;
};

static Pseudos pseudos[] = 
{
  {"CF1",  false,  false},
  {"CF3",  false,  false},
  {"CF4",  false,  false},
  {"CF5",  false,  false},
  {"REL",  false,  false},
  {"ABS",  false,  false},
  {"LOAD", false,  false},
  {"ORG",  true,   false},
  {"FIN",  false,  false},
  {"MOR",  false,  false},
  {"END",  true,   false},
  {"EJCT", false,  false},
  {"LIST", false,  false},
  {"NLST", false,  false},
  {"EXD",  false,  false},
  {"LXD",  false,  false},
  {"SETB", true,   false},
  {"EQU",  true,   false},
  {"SET",  true,   false},
  {"DAC",  true,   true },
  {"DEC",  true,   false},
  {"DBP",  true,   false},
  {"OCT",  true,   false},
  {"HEX",  true,   false},
  {"BCI",  true,   false},
  {"VFD",  true,   false},
  {"BSS",  true,   false},
  {"BES",  true,   false},
  {"BSZ",  true,   false},
  {"COMN", true,   false},
  {"SETC", true,   false},
  {"ENT",  true,   false},
  {"SUBR", true,   false},
  {"EXT",  true,   false},
  {"XAC",  true,   true },
  {"CALL", true,   false},
  {"IFP",  true,   false},
  {"IFM",  true,   false},
  {"IFZ",  true,   false},
  {"IFN",  true,   false},
  {"ENDC", false,  false},
  {"ELSE", false,  false},
  {"FAIL", false,  false},
  {"***",  true,   true },
  {"PZE",  true,   true },

  {0, false}
};

#define MAX_LINE 1024

static class InstrTable *instr_table = 0;

static void process_line(char i_buffer[], char o_buffer[])
{
  int ib = 0;
  int ob = 0;
  int pending_tabs=0;
  char word[MAX_LINE];

  if (!instr_table) {
    instr_table = new InstrTable();
    //instr_table->dump_dispatch_table();
  }

  /* Clip any trailing spaces */
  int k=strlen(i_buffer)-1;
  while ((k>0) && (isspace(i_buffer[k])))
    i_buffer[k--] = '\0';

  /* Find first non-blank character */
  while (isspace(i_buffer[ib]))
    ib++;

  if (listing) {
    //
    // A 4-digit decimal number at the end of the
    // line is probably the source line number
    // delete it.
    //
    k=strlen(i_buffer)-1;
    int n = k-4;
    if (n<0) n = 0;

    while ((k>n) &&
           (isdigit(i_buffer[k]) ||
            (i_buffer[k]=='O'))) // Common OCR error
      k--;

    if (k <= n) { // All four were digits
      k++;
      while ((k>0) && (isspace(i_buffer[k])))
        i_buffer[k--] = '\0';
    }

    //
    // Digits at the start of the line
    // are probably the source line number and the
    // assembled opcodes - discard.
    // A single minus '-' sign may also be deleted since
    // it is used in the opcode of memory reference instructions
    // that have the top (indirect) bit set.
    //
    bool first_minus = true;
    while (isspace(i_buffer[ib]) ||
           isdigit(i_buffer[ib]) ||
           ((i_buffer[ib] == '-') && (first_minus))) {
      if (i_buffer[ib] == '-')
        first_minus = false;
      ib++;
    }
  }

  /* Is it a line comment character? */
  if (i_buffer[ib] == '*') {
    /* Yes, copy the whole line across - then we're done */
    while (i_buffer[ib])
      o_buffer[ob++] = i_buffer[ib++];
  } else {
    bool is_instr = false;
    bool is_mr = false;
    bool operands = false;
    int j;
    int instr_count;
    
    for (instr_count=0; instr_count<2; instr_count++) {
      while (i_buffer[ib] && isspace(i_buffer[ib]))
        ib++; // Discard leading space
      j = 0;
      
      while (i_buffer[ib] &&
             (isalnum(i_buffer[ib]) ||
              ((i_buffer[ib]=='*') && (j<3)))) { // Pseudo could be "***"
        word[j++] = i_buffer[ib];
        ib++;
      }
      word[j] = '\0';
      
      InstrTable::Instr *instr = instr_table->lookup(word);
      
      if (instr) {
        is_instr = true;
        InstrTable::Instr::INSTR_TYPE type = instr->get_type();
        if (type == InstrTable::Instr::MR)
          is_mr = operands = true;
        else if ((type == InstrTable::Instr::SH) ||
                 (type == InstrTable::Instr::IO) ||
                 (type == InstrTable::Instr::IOG))
          operands = true;
      } else {
        /* Is it a pseudo-op ? */
        int k = 0;
        while (pseudos[k].mnemonic) {
          if (strcmp(pseudos[k].mnemonic, word)==0) {
            is_instr = true;
            operands = pseudos[k].operands;
            is_mr    = pseudos[k].is_mr;
            break;
          }
          k++;
        }
      }
      
      if (is_instr) {
        /* Word is an instruction.
         * If it's a memory-reference it might be indirect,
         * and so followed by an asterix */
        if (is_mr && (i_buffer[ib] == '*')) {
          word[j++] = i_buffer[ib++];
          word[j] = '\0';
        }
        
        o_buffer[ob++] = tab_char;
        int k=0;
        while (word[k])
          o_buffer[ob++] = word[k++];
        break;
      } else {
        /* Word isn't an instruction - 
         * if first time through loop assume it's a label
         * if second time - confused put space separator before */
        if (instr_count==1)
          o_buffer[ob++] = ' ';
        int k=0;
        while (word[k])
          o_buffer[ob++] = word[k++];
      }
    }
    
    if (is_instr)
      pending_tabs++;
    else
      o_buffer[ob++] = ' ';
    
    if (is_instr && operands) {
      while (i_buffer[ib] && isspace(i_buffer[ib]))
        ib++; // Discard leading space
      
      while (i_buffer[ib] && (!isspace(i_buffer[ib]))) {
        while (pending_tabs) {
          o_buffer[ob++] = tab_char;
          pending_tabs--;
        }
        o_buffer[ob++] = i_buffer[ib++];
      }
    }
    
    if (is_instr)
      pending_tabs++;
    else
      o_buffer[ob++] = ' ';
    
    while (i_buffer[ib] && isspace(i_buffer[ib]))
      ib++; // Discard leading space
    
    while (i_buffer[ib]) {
      while (pending_tabs) {
        o_buffer[ob++] = tab_char;
        pending_tabs--;
      }
      o_buffer[ob++] = i_buffer[ib++];
    }      
  }
  o_buffer[ob] = '\0';
}

static void tabify(FILE *in_file, FILE *out_file)
{
  char i_buffer[MAX_LINE];
  char o_buffer[MAX_LINE];
  int b;
  int c = fgetc(in_file);
  
  while (c!=EOF)
  {
    /* Read a line into the line buffer */
    b = 0;
    while ((b < (MAX_LINE-1)) && (c != EOF) && (c != '\n'))
      {
        if (c != '\0')
          i_buffer[b++] = c;
        c = fgetc(in_file);
      }
      
    if (c == '\n')
        c = fgetc(in_file); /* Discard line end character */
    
    i_buffer[b] = '\0';
    
    process_line(i_buffer, o_buffer);
    
      /* Write output characters to output file */
    b = 0;
    while ((b < MAX_LINE) && o_buffer[b])
      {
        fputc(o_buffer[b], out_file);
        b++;
      }
      fputc('\n', out_file);
  }
  
}

int main(int argc, char **argv)
{
  int pending_filename=0;
  int a;
  int reading_args = 1;
  int usage = 0;
  
  FILE *in_file, *out_file;

  a=1;
  while (a < argc)
    {
      if (pending_filename)
        {
          out_filename = argv[a];
          pending_filename = 0;
        }
      else if ((argv[a][0] == '-') && (reading_args))
        {
          switch(argv[a][1])
            {
            case '\0':
              /*
               * Just '-' turn off arg-reading to
               * allow a '-' to start the input
               * filename.
               */
              reading_args = 0;
              break;

            case 'c':
              /*
               * Specify tab char
               */
              switch(argv[a][2])
                {
                case '\\':
                  switch(argv[a][3])
                    {
                    case '\\': tab_char = '\\'; break;
                    case 't':tab_char = '\t'; break;
                    case 'v':tab_char = '\v'; break;
                    case 'n':tab_char = '\n'; break;
                    case 'r':tab_char = '\r'; break;
                    case '\'':tab_char = '\''; break;
                    case '\"':tab_char = '\"'; break;
                    default:
                      usage = 1;
                    }
                  break;
                case '\0':
                  usage = 1;
                  break;
                default:
                  tab_char = argv[a][2];
                }
              break;

            case 'o':
              if (argv[a][2])
                out_filename = &argv[a][2];
              else
                pending_filename = 1;
              break;

            case 'l':
              listing = true;
              break;

            default:
              usage = 1;
            }
        }
      else
        {
          /*
           * Should be the input filename
           */
          if (a == (argc-1))
            in_filename = argv[a];
          else
            usage = 1;
        }
      a++;
    }

  if (usage)
    {
      fprintf(stderr, "usage: %s [-c<tab-char>] [-o[ ]filename] [-l] [-] [<input-filename>]\n",
              argv[0]);
      exit(1);
    }

  if (in_filename)
    {
      in_file = fopen(in_filename, "r");
      if (!in_file)
        {
          fprintf(stderr, "Could not open <%s> for reading\n", in_filename);
          exit(1);
        }
    }
  else
    in_file = stdin;

  if (out_filename)
    {
      out_file = fopen(out_filename, "w");
      if (!out_file)
        {
          fprintf(stderr, "Could not open <%s> for writing\n", out_filename);
          exit(1);
        }
    }
  else
    out_file = stdout;

  tabify(in_file, out_file);

  if (in_filename)
    fclose(in_file);
  if (out_filename)
    fclose(out_file);
  
  exit(0);
}
