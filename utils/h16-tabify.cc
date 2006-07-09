#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "instr.hh"

/*
 * usage:
 *
 * h16-tabify [-c<tab-char>] [-o[ ]filename] [-] [<input-filename>]
 *
 */

static char tab_char = '\\';
static char *in_filename = 0;
static char *out_filename = 0;

struct Pseudos
{
  const char *mnemonic;
  bool operands;
};

static Pseudos
{
  {"CF1", false};
  {"CF3", false};
  {"CF4", false};
  {"CF5", false};
  {"REL", false};
  {"ABS", false};
  {"LOAD", };
  {"ORG", true};
  {"FIN", false};
  {"MOR", false};
  {"END", false};
  {"EJCT", false};
  {"LIST", false};
  {"NLST", false};
  {"EXD", false};
  {"LXD", false};
  {"SETB", true};
  {"EQU", true};
  {"SET", true};
  {"DAC", true};
  {"DEC", true};
  {"DBP", true};
  {"OCT", true};
  {"HEX", true};
  {"BCI", true};
  {"VFD", true};
  {"BSS", true};
  {"BES", true};
  {"BSZ", true};
  {"COMN", true};
  {"SETC", true};
  {"ENT", true};
  {"SUBR", true};
  {"EXT", true};
  {"XAC", true};
  {"CALL", true};
  {"IFP", true};
  {"IFM", true};
  {"IFZ", true};
  {"IFN", true};
  {"", true};
  {"", true};
  {"", true};

  {0, false}
};

#define MAX_LINE 1024

static class InstrTable *instr_table = 0;

static void process_line(char i_buffer[], char o_buffer[])
{
  int ib = 0;
  int ob = 0;
  char word[MAX_LINE];

  if (!instr_table)
    instr_table = new InstrTable();

  /* Find first non-blank character */
  while (isspace(i_buffer[ib]))
    ib++;

  /* Is it a line comment character? */
  if (i_buffer[ib] == '*')
    {
      /* Yes, copy the whole line across - then we're done */
      while (i_buffer[ib])
	o_buffer[ob++] = i_buffer[ib++];
    }
  else
    {
      int i = ib;
      int j = 0;
      while (i_buffer[i] && isalnum(i_buffer[i]))
	{
	  word[j++] = i_buffer[i];
	  i++;
	}
      word[j] = '\0';

      /* word is the first continuous span of alphanumeric characters
       * is it a mnemonic? */

      InstrTable::Instr *instr = instr_table->lookup(word);

      if (instr)
	printf("\"%s\" is a menmonic\n", word);
      else
	printf("\"%s\"\n", word);
      
    }
}

static void tabify(FILE *in_file, FILE *out_file)
{
  int c = 0;
  char i_buffer[MAX_LINE];
  char o_buffer[MAX_LINE];
  int b;

  do
    {
      c = fgetc(in_file);

      if (c==EOF)
        {
          /* Do nothing */
        }
      else
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

    } while (c!=EOF);
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
      fprintf(stderr, "usage: %s [-c<tab-char>] [-t[ ]<comma-separated-list>] [-o[ ]filename] [-] [<input-filename>]",
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
