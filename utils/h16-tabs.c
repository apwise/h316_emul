#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

/*
 * usage:
 *
 * tabs [-c<tab-char>] [-t[ ]<comma-separated-list>] [-o[ ]filename] [-] [<input-filename>]
 *
 */

static char tab_char = '\t';
static char *in_filename = 0;
static char *out_filename = 0;
static int tab_list = 0;
static unsigned long ntabs = 8;
#define NUM_TABS 256
static unsigned long tabs[NUM_TABS];

enum PENDING
  {
    PEND_NONE,
    PEND_TAB,
    PEND_FILENAME,
  };

static int read_tabs(char *buf)
{
  unsigned long num_tabs = 0;
  unsigned long n;
  char *str=buf;
  char *endptr;
  int reading = 1;
  unsigned long tab, i;

  while (((int) *str) && isspace((int) *str))
    str++;

  while ((reading) && (num_tabs < NUM_TABS))
    {
      errno = 0;
      n = strtoul(str, &endptr, 0);
      if ((endptr > str) && (errno == 0))
	tabs[num_tabs++] = n;
      else
	reading = 0;
      str = endptr;

      while (((int)*str) &&
	     (isspace((int)*str) || (((int)*str) == ',')))
	str++;
    }

  if (num_tabs >= NUM_TABS)
    return 1;

  if (num_tabs == 1)
    {
      ntabs = tabs[0];
      tab_list = 0;
      return 0;
    }

  /*
   * Check tab stops are monotonically increasing
   */
  tab = tabs[0];
  for (i=1; i<num_tabs; i++)
    {
      if (tabs[i] <= tab)
	return 1;
      tab = tabs[i];
    }
  ntabs = num_tabs;
  tab_list = 1;
  return 0;
}

static void expand_tabs(FILE *in_file, FILE *out_file)
{
  int col = 1;
  int c = 0;
  int i;

  do
    {
      c = fgetc(in_file);

      if (c==EOF)
        {
          /* Do nothing */
        }
      else if (c == '\n')
        {
          fputc(c, out_file);
          col = 1;
        }
      else if (c == tab_char)
        {
          if (tab_list)
            {
              /* Find a tab-stop (if any) > current position */
              i = 0;
              while ((i<ntabs) && (tabs[i] <= col))
                i++;

              if (i>=ntabs) /* replace tab by space */
                {
                  fputc(' ', out_file);
                  col++;
                }
              else
                {
                  do
                    {
                      fputc(' ', out_file);
                      col++;
                    } while (col < tabs[i]);
                }
            }
          else
            {
              do
                {
                  fputc(' ', out_file);
                  col++;
                } while ((col % ntabs) != 0);
            }
        }
      else
        {
          fputc(c, out_file);
          col++;
        }

    } while (c!=EOF);
}

int main(int argc, char **argv)
{
  int a;
  enum PENDING pending = PEND_NONE;
  int reading_args = 1;
  int usage = 0;
  
  FILE *in_file, *out_file;

  a=1;
  while (a < argc)
    {
      if (pending == PEND_TAB)
	{
          if (read_tabs(argv[a]))
            usage = 1;

	  pending = PEND_NONE;
	}
      else if (pending == PEND_FILENAME)
	{
	  out_filename = argv[a];
	  pending = PEND_NONE;
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

	    case 't':
	      if (argv[a][2])
                {
                  if (read_tabs(&argv[a][2]))
                    usage = 1;
                }
	      else
		pending = PEND_TAB;
	      break;

	    case 'o':
	      if (argv[a][2])
		out_filename = &argv[a][2];
	      else
		pending = PEND_FILENAME;
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

  expand_tabs(in_file, out_file);

  if (in_filename)
    fclose(in_file);
  if (out_filename)
    fclose(out_file);
  
  exit(0);
}
