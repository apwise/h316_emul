#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_LEN_QUOTE 20
#define DEFAULT_LEFT_QUOTE "`"
#define DEFAULT_RIGHT_QUOTE "'"

static int single_file;
static int target_lines;
static char left_quote[MAX_LEN_QUOTE+1];
static char right_quote[MAX_LEN_QUOTE+1];
static char *input_filename;
static char *output_root;
static char *title;
static char *css_url;

#define LINES_PER_FILE 300

#define RESERVED   256
#define NUMBER     257
#define IDENTIFIER 258
/*#define SPACE      259*/
#define COMMENT    260
#define STRING     261
#define ASSIGN     262

static int token_number;
static int line_number;
static int file_number;
static int max_file_number = 0;

static FILE *start_new_file(FILE *fp, int realy_open);
static char *output_file_name(int file_num);

#define BUFLEN 1024
static char *lex_buf, *lex_buf2;
static int buf_ptr;
static int spaces, spaces2;

static void putch(int ch)
{
  if (buf_ptr >= (BUFLEN-1)) /* 1 for '\0' */
    {
      fprintf(stderr, "lex_buf overflow\n");
      exit(1);
    }
  lex_buf2[buf_ptr++] = ch;
}

static int pending_ch;

static int getch(FILE *fp)
{
  int r;

  if (pending_ch!=EOF)
    r = pending_ch;
  else
    r = getc(fp);

  pending_ch = EOF;
  return r;
}

static void ungetch(int ch)
{
  pending_ch = ch;
}

static void init_lex()
{
  pending_ch = EOF;
  lex_buf = malloc(BUFLEN);
  lex_buf2 = malloc(BUFLEN);
}

/**********************************************************
 * Routines used by table driven lexer
 *********************************************************/
static int my_isblank(int ch)
{
  return ((ch == ' ') || (ch == '\t'));
}
static int my_isdot(int ch)
{
  return (ch == '.');
}
static int my_isquote(int ch)
{
  return (ch == '"');
}
static int my_isamper(int ch)
{
  return (ch == '&');
}
static int my_isnl(int ch)
{
  return (ch == '\n');
}
static int my_issquot(int ch)
{
  return (ch == '\'');
}

struct LexRule {
  int token;              /* The token to return */
  int (*start_test)(int); /* Routine to test start condition */
  int (*cont_test)(int);  /* Routine to test continuation condition */
  int invert_cont;        /* (bool) invert continuation test */
  int unget_last;         /* (bool) whether to unget the ending char */
} rule_table[] = 
{
  /*  {SPACE,      my_isblank, my_isblank,    0, 1},*/
  {RESERVED,   my_isdot,   isalnum,       0, 1},
  {IDENTIFIER, isalpha,    isalnum,       0, 1},
  {STRING,     my_isquote, my_isquote,    1, 0},
  {COMMENT,    my_isamper, my_isnl,       1, 1},
  {NUMBER,     my_issquot, isdigit,       0, 1},
  {NUMBER,     isdigit,    isdigit,       0, 1},
  {0,          NULL,       NULL,          0, 0}
};

static int lex(FILE *fp)
{
  int ch;
  int i;

  char *t;

  t = lex_buf; lex_buf = lex_buf2; lex_buf2 = t;
  spaces = spaces2; spaces2 = 0;

  buf_ptr = 0;

  ch = getch(fp);
  while (my_isblank(ch))
    {
      spaces2++;
      ch = getch(fp);
    }

  i = 0;
  while (rule_table[i].start_test)
    {
      if (rule_table[i].start_test(ch))
        {
          do
            {
              putch(ch);
              ch = getch(fp);
            } while ( rule_table[i].cont_test(ch) != 
                      rule_table[i].invert_cont );
          if ( rule_table[i].unget_last )
            ungetch(ch);
          else
            putch(ch);
          putch(0);

          return rule_table[i].token;
        }
      i++; /* try next start test */
    }

  putch(ch);

  if (ch == ':')
    {
      ch = getch(fp);
      if (ch == '=')
        {
          putch(ch);
          putch(0);
          return ASSIGN;
        }
      else
        {
          ungetch(ch);
          ch = ':';
        }
    }
  /* Didn't find a special token so this character stands
     for itself */

  putch(0);
  return ch;
}

/*********************************************************
 * Symbol table
 *********************************************************/
enum SymbolType
{
  ST_NONE,
  ST_PROCEDURE,
  ST_COMPCONST,
  ST_CONSTANT,
  ST_INTEGER,
  ST_ARRAY,
  ST_LABEL,
  ST_SWITCH,

  ST_NUM
};

struct Symbol
{
  char *name;
  enum SymbolType type;
  int defined; /* (bool) whether definition (as opposed to
                * declaration) has been reached */
  int local; /* (bool) whether the symbol is local */
  int token_number; 
  int line_number;
  int file_number;
};

#define SYMBOL_TABLE_SIZE 10000

static struct Symbol **symbol_table = NULL;
static int symbols;
static int procedure_symbols;

static void init_symbol_table()
{
  symbol_table = malloc(SYMBOL_TABLE_SIZE *
                         sizeof(struct Symbol *));
  symbols = 0;
  procedure_symbols = symbols;
}

/*
 * record how many samples have been defined at the
 * start of a procedure, so the locals can be deleted
 * at the end.
 */
static void record_procedure_symbols()
{
  procedure_symbols = symbols;
}

static void wind_back_procedure()
{
  while(symbols>procedure_symbols)
    {
      symbols--;
      free(symbol_table[symbols]->name);
      free(symbol_table[symbols]);
    }
}

static struct Symbol *add_symbol(char *name)
{
  struct Symbol *s;
  symbol_table[symbols] = s = malloc(sizeof(struct Symbol));
  s->name = strdup(name);
  s->type = ST_NONE;
  s->defined = 1;
  s->local = 0;
  s->token_number = token_number;
  s->line_number = line_number;
  s->file_number = file_number;
  
  symbols++;
  return s;
}

static struct Symbol *lookup(char *name)
{
  int i;
  struct Symbol *s;
  for (i=symbols-1; i>=0; i--)
    {
      s = symbol_table[i];
      if (strcmp(s->name, name)==0)
        return s;
    }

  return NULL;
}

/**********************************************************
 * Rendering routine
 *********************************************************/
void char_sub(int c, FILE *fp)
{
  if (!fp) return;

  switch(c)
    {
    case ' ': fprintf(fp, "&nbsp;"); break;
    case '\t': fprintf(fp, "&nbsp;"); break;
    case '<': fprintf(fp, "&lt;"); break;
    case '>': fprintf(fp, "&gt;"); break;
    case '&': fprintf(fp, "&amp;"); break;
    case '"': fprintf(fp, "&quot;"); break;
    default:
      putc(c, fp);
    }
}

static void copy_str(FILE *fp, char *str)
{
  int i = 0;
  while (str[i])
    {
      char_sub(str[i] & 0xff, fp);
      i++;
    }
}
static void copy_token(FILE *fp)
{
  copy_str(fp, lex_buf);
}

static int doindent(int token, char *buf)
{
  if (token == '(')
    return 1;

  if ((token == RESERVED) && (strcmp(buf, ".BEGIN")==0))
    return 1;

  return 0;
}

static int unindent(int token, char *buf)
{
  if (token == ')')
    return 1;

  if ((token == RESERVED) && (strcmp(buf, ".END")==0))
    return 1;

  return 0;
}

static int pending_end_code = -1;

static void force_end_code(FILE *fp)
{
  if ((fp) && (pending_end_code))
    fprintf(fp, "</span>");
  pending_end_code = -1;
}

static void start_code(FILE *fp, int class)
{
  if ( (pending_end_code != (-1)) &&
       (class != pending_end_code) )
    force_end_code(fp);

  if ( (pending_end_code == (-1)) ||
       (class != pending_end_code) )
    {
      if (fp)
        {
          if (class)
            fprintf(fp, "<span CLASS=%c>", class);
/*           else */
/*             fprintf(fp, "<span>"); */
        }
    }

  pending_end_code = -1;
}

static void end_code(FILE *fp, int class)
{
  pending_end_code = class;
}


static int good_file_break(int token, int token2)
{
  int r = 0;

  if ((token == RESERVED) && (strcmp(lex_buf, ".ORIGIN")==0))
    r = 1;
  else if ((token == RESERVED) && (strcmp(lex_buf, ".PROCEDURE")==0))
    r = 1;
  else if ((token2 == RESERVED) && (strcmp(lex_buf2, ".PROCEDURE")==0))
    {
      if ((token == '@') ||
          ((token == RESERVED) && (strcmp(lex_buf, ".CONDITIONAL")==0)))
        r = 1;
    }
  else if ((token == '@') &&
           ((token2 == RESERVED) && (strcmp(lex_buf2, ".CONDITIONAL")==0)))
    r = 1;

  return r;
}

char *ref_name(char *name, int define_locals, char *pn)
{
  static char buf[256];
  int i;

  if (define_locals)
    sprintf(buf, "%s_%s", pn, name);
  else
    sprintf(buf, "%s", name);
  
  for (i=0; buf[i]; i++)
    if (buf[i] == ' ')
      buf[i] = '_';

  return buf;
}

void render_token(int token, int next_token, FILE **pfp)
{
  int i;
  
  int set_line_start = 0;
  static int line_start = 0;
  static int indent = 0;
  static int procedure_seen = 0;
  static int forward_seen = 0;
  static int compconst_seen = 0;
  static int constant_seen = 0;
  static int define_locals = 0;
  static int procedure_begins = 0;
  static char *procedure_name = NULL;
  static int integer_seen = 0;
  static int array_seen = 0;
  static int set_seen = 0;
  static int lsb_seen = 0;
  static int lrb_seen = 0;
  static int address_seen = 0;
  static int percent_seen = 0;
  static int label_seen = 0;
  static int goto_seen = 0;
  static int switch_seen = 0;
  static int equals_seen = 0;

  struct Symbol *s;
  FILE *fp = *pfp;

  token_number ++;

  if (line_start)
    {
      /* Start a new file ? */
      if ((line_number > target_lines) &&
          good_file_break(token, next_token) &&
          (!single_file))
        {
          fp = (*pfp) = start_new_file(fp, (fp!=0));
        }


      /* If the first token on this line
         un-indents then do it now, before printing
         the indenting spaces */

      if (unindent(token, lex_buf))
          indent--;

      for (i=0; i<indent; i++)
        {
          char_sub(' ', fp);
          char_sub(' ', fp);
        }
    }
  else
    {
      if (unindent(token, lex_buf))
        indent--;

      /* render any leading spaces */
      if (spaces > 0)
        {
          if (pending_end_code == -1)
            start_code(fp, 0);

          for (i=0; i<spaces; i++)
            char_sub(' ', fp);
        }
    }

  if (doindent(token, lex_buf))
    indent++;

  i = 0; /* Used for class */
  switch (token)
    {
    case '\n':
      if (fp) fprintf(fp, "\n");
/*       if (fp) fprintf(fp, "<br>\n"); */
      set_line_start = 1;
      line_number++;
      break;

    case EOF:
      break;
      
      /*case SPACE:
      if (!line_start)
        copy_token(fp); 
      break;
      */

    case RESERVED:
      start_code(fp, 'R');
      copy_token(fp);
      end_code(fp,'R');

      if (strcmp(".PROCEDURE", lex_buf)==0)
        {
          procedure_seen = 1;
          procedure_begins = 0;
          if (!forward_seen)
            define_locals = 1;
        }
      else if (strcmp(".FORWARD", lex_buf)==0)
        {
          forward_seen = 1;
          define_locals = 0;
        }
      else if (strcmp(".COMPCONST", lex_buf)==0)
        compconst_seen = 1;
      else if (strcmp(".CONSTANT", lex_buf)==0)
        constant_seen = 1;
      else if (strcmp(".ARRAY", lex_buf)==0)
        array_seen = 1;
      else if (strcmp(".SET", lex_buf)==0)
        set_seen = 1;
      else if (strcmp(".ADDRESS", lex_buf)==0)
        address_seen = 1;
      else if (strcmp(".BEGIN", lex_buf)==0)
        {
          if (define_locals)
            procedure_begins++;
        }
      else if (strcmp(".END", lex_buf)==0)
        {
          if (procedure_begins>0)
            {
              procedure_begins--;
              if (procedure_begins <= 0)
                {
                  define_locals = 0;
                  if (procedure_name)
                    {
                      free(procedure_name);
                      procedure_name = 0;
                    }
                  wind_back_procedure(); /* delete locals from sym tab */
                }
            }
        }
      else if (strcmp(".INTEGER", lex_buf)==0)
        integer_seen = 1;
      else if (strcmp(".LABEL", lex_buf)==0)
        label_seen = 1;
      else if (strcmp(".GOTO", lex_buf)==0)
        goto_seen = 1;
      else if (strcmp(".SWITCH", lex_buf)==0)
        switch_seen = 1;
      break;

    case STRING:
      start_code(fp, 'S');
      copy_token(fp);
      end_code(fp,'S');
      break;

    case COMMENT:
      start_code(fp, 'C');
      copy_token(fp);
      end_code(fp,'C');
      break;

    case IDENTIFIER:
      {
        int gen_hyper = 0;
        int gen_name = 0;

        if ((define_locals) || (!fp))
          {
            /* i.e. locals on both pases,
               globals on pass 1 only */

            if (integer_seen)
              {
                s = add_symbol(lex_buf);
                s->type = ST_INTEGER;
                s->defined = (!forward_seen);
                s->local = define_locals;
              }
            else if ((array_seen) && (!address_seen) &&
                     (!lsb_seen) && (!lrb_seen))
              {
                s = add_symbol(lex_buf);
                s->type = ST_ARRAY;
                s->local = define_locals;

                s->defined = 1;
                if ( (next_token==RESERVED) &&
                     (strcmp(lex_buf2, ".LATER")==0))
                  s->defined = 0;
              }
            else if ((label_seen) || (goto_seen) ||
                     (switch_seen && equals_seen))
              {
                /*printf("label declaration: %s (%s) %d\n",
                  lex_buf, procedure_name, symbols);*/
                if ((label_seen) || (!lookup(lex_buf)))
                  {
                    s = add_symbol(lex_buf);
                    s->type = ST_LABEL;
                    s->defined = 0;
                    s->local = define_locals;
                  }
              }
            else if (switch_seen && (!equals_seen))
              {
                /* This is a switch identifer
                 * (rather than a label implicitly declared
                 * in a switch statement) */
                s = add_symbol(lex_buf);
                s->type = ST_SWITCH;
                s->local = define_locals;
              }
            else if (next_token==':')
              {
                // This looks like a label definition
                if ( (s=lookup(lex_buf)) )
                  {
                    if (!s->defined)
                      {
                        /*printf("defined %s %d\n", lex_buf, token_number);*/
                        s->defined = 1;
                        s->token_number = token_number;
                        s->line_number = line_number;
                        s->file_number = file_number;                       
                      }
                    else if (define_locals)
                      {
                        /* must be a local of same name as global?*/
                        /*printf("Local %s masks global? %d %d\n", lex_buf,
                          symbols, s->type);*/
                        s = add_symbol(lex_buf);
                        s->type = ST_LABEL;
                        s->defined = 1;
                        s->local = define_locals;
                      }
                    else
                      {
                        fprintf(stderr, "Why? %s\n", lex_buf);
                      }
                  }
                else
                  {
                    s = add_symbol(lex_buf);
                    s->type = ST_LABEL;
                    s->defined = 1;
                    s->local = define_locals;
                  }
              }
            else if (compconst_seen)
              {
                s = add_symbol(lex_buf);
                s->type = ST_COMPCONST;
                s->defined = 1;
                s->local = define_locals;
              }
            else if (constant_seen)
              {
                s = add_symbol(lex_buf);
                s->type = ST_CONSTANT;
                s->defined = 1;
                s->local = define_locals;
              }
          }

        if (!fp) /* i.e. pass 1 */
          {
            if (procedure_seen)
              {
                /*printf("%s procedure?\n", lex_buf);*/
                /* This looks like a procedure defn/decl */
                if ( (s=lookup(lex_buf)) )
                  {
                    /* Seen it before */
                    if ( (!forward_seen) && (!s->defined) &&
                         (s->type == ST_PROCEDURE))
                      {
                        /* Reached .FORWARD .PROCEDURE's defn */
                        s->defined = 1;
                        s->token_number = token_number;
                        s->line_number = line_number;
                        s->file_number = file_number;

                        record_procedure_symbols();
                      }
                    else
                      fprintf(stderr,
                              "error: procedure already defined? %s line %d\n",
                             s->name, line_number);
                  }
                else
                  {
                    s = add_symbol(lex_buf);
                    s->type = ST_PROCEDURE;
                    s->defined = (!forward_seen);

                    record_procedure_symbols();
                  }
              }
            else if ((set_seen) && (!lsb_seen) && (!lrb_seen))
              {
                if ( (s=lookup(lex_buf)) )
                  {
                    if (s->type == ST_ARRAY)
                      {
                        /* Reached .SET of a .LATER .ARRAY */
                        s->defined = 1;
                        s->token_number = token_number;
                        s->line_number = line_number;
                        s->file_number = file_number;
                      }
                    else
                      fprintf(stderr, 
                              "Why am I here %s %d?\n", lex_buf, s->type);                      
                  }
                else
                  fprintf(stderr, 
                          "Set but no .ARRAY? %s\n", lex_buf);                      
              }
          }
        else
          { /* pass 2 */
            if (procedure_seen)
              procedure_name = strdup(lex_buf);

            if ( (s=lookup(lex_buf)) )
              {
                switch (s->type)
                  {
                  case ST_PROCEDURE:
                  case ST_CONSTANT:
                  case ST_COMPCONST:
                  case ST_INTEGER:
                  case ST_ARRAY:
                  case ST_LABEL:
                  case ST_SWITCH:
                    if (!percent_seen)
                      {
                        gen_hyper = 1;
                        if (token_number == s->token_number)
                          {
                            // LABEL is the only identifier
                            // type that can be forward referenced
                            // and local to a procedure.
                            if ((s->type == ST_LABEL) && (!s->defined))
                              gen_name = 0;
                            else
                              gen_name = 1;
                          }
                      }

                    break;

                  default:
                    abort();
                  }
              }
          }
        start_code(fp, 'I');
        if (gen_hyper)
          {
/*             force_end_code(fp); */
            if (fp)
              {
                if (gen_name)
                  fprintf(fp, "<A NAME=\"%s\">",
                          ref_name(lex_buf, s->local,
                                   procedure_name));
                else
                  fprintf(fp, "<A HREF=\"%s#%s\">",
                          ((file_number != s->file_number) ?
                           output_file_name(s->file_number) : ""),
                          ref_name(lex_buf, s->local,
                                   procedure_name));
              }
          }
        copy_token(fp);
        if (gen_hyper)
          {
/*             force_end_code(fp); */
            if (fp)
              fprintf(fp,"</A>");
          }
        end_code(fp,'I');
      }
      break;

    case NUMBER:
      start_code(fp, 0);
      copy_token(fp);
      end_code(fp, 0);
      break;

    case ASSIGN:
      start_code(fp, 0);
      copy_token(fp);
      end_code(fp, 0);
      break;

    default:
      start_code(fp, 0);
      char_sub(token, fp);
      end_code(fp, 0);

      if (token == ';')
        {
          procedure_seen = 0;
          forward_seen = 0;
          compconst_seen = 0;
          constant_seen = 0;
          integer_seen = 0;
          array_seen = 0;
          set_seen = 0;
          lsb_seen = 0; /* just in case */
          lrb_seen = 0; /* just in case */
          address_seen = 0; /* just in case */
          percent_seen = 0;
          label_seen = 0;
          goto_seen = 0;
          switch_seen = 0;
          equals_seen = 0;
        }
      else if (token == ',')
        {
          address_seen = 0;
          percent_seen = 0;
        }
      else if (token == '[')
        lsb_seen = 1;
      else if (token == ']')
        lsb_seen = 0;
      else if (token == '(')
        lrb_seen = 1;
      else if (token == ')')
        lrb_seen = 0;
      else if (token == '%')
        percent_seen = 1;
      else if (token == '=')
        equals_seen = 1;
      break;
    }

  line_start = set_line_start;
}

/**********************************************************
 * Handle output files
 *********************************************************/
static char *preamble = 
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\n"\
"   \"http://www.w3.org/TR/html4/strict.dtd\">\n"\
"<html>\n"\
"  <head>\n";

static char *css_link = "    <link rel=\"STYLESHEET\" type=\"text/css\" href=\"%s\" title=\"Style\">\n";

static char *preamble2 = 
"   <title>%s</title>\n"\
"  </head>\n"\
"  <body>\n"\
"    <DIV ID=\"header\">\n"\
"    <h1>%s</h1>\n"\
"    <p><a href=\"../../../index.html\">Home</a>&nbsp;>&nbsp;\n"\
"       <a href=\"../../index.html\">Software</a>&nbsp;>&nbsp;\n"\
"       <a href=\"../index.html\">PL516</a>&nbsp;>&nbsp;\n"\
"       %s Source</p>\n"\
"    </DIV>\n"\
"    <DIV ID=\"sitenav\">\n"\
"      <UL>\n"\
"	<li><a href=\"../../../history.html\">History</a></li>\n"\
"	<li><a href=\"../../../programming.html\">Programming</a></li>\n"\
"	<li><a href=\"../../../software/index.html\">Software</a></li>\n"\
"       <li><a href=\"../../../hardware/index.html\">Hardware</a></li>\n"\
"	<li><a href=\"../../../emulator.html\">Emulator</a></li>\n"\
"      </UL>\n"\
"    </DIV>\n"\
"\n"\
"    <DIV ID=\"content\">\n";

static char *preamble3 =
"      <div class=\"code\">";

static char *postamble1 = 
"  </div>\n";

static char *postamble = 
"  </div>\n"\
"  <div id=\"footer\">\n"\
"    <address>\n"\
"      <a href=\"mailto:adrian@adrianwise.co.uk\">Adrian Wise</a>\n"\
"    </address>\n"\
"  </div>\n"\
"  </BODY>\n"\
"</HTML>\n";

static char *output_file_name(int file_num)
{
  static char buf[40];
  if (single_file)
    sprintf(buf, "%s.html", output_root);

  else
    sprintf(buf, "%s_%d.html", output_root, file_num);

  return buf;
}

static void buttons(FILE *fp, int top)
{
  int i;
  if (!fp) return;

/*   if (!top) */
/*     fprintf(fp, "<HR>\n"); */
      
  if (!single_file)
    {
      fprintf(fp, "<TABLE WIDTH=\"100%c\" class=\"simple\">\n", '%');

      fprintf(fp, "  ");
      for (i=0; i<5; i++)
        fprintf(fp, "<col WIDTH=\"20%c\">", '%');
      fprintf(fp, "\n");

      fprintf(fp, "  <tbody>\n");
      fprintf(fp, "    <TR>\n");
      
      if (file_number == 1)
        fprintf(fp, "      <TD class=\"head\">First</TD>\n      <TD class=\"head\">Previous</TD>\n");
      else
        {
          fprintf(fp, "     <TD class=\"head\"><A HREF=%s>First</A></TD>\n", output_file_name(1));
          fprintf(fp, "     <TD class=\"head\"><A HREF=%s>Previous</A></TD>\n",
                  output_file_name(file_number-1));
        }
      fprintf(fp, "      <TD class=\"head\">&nbsp;</TD>\n");
      
      if (file_number == max_file_number)
        fprintf(fp, "      <TD class=\"head\">Next</TD>\n    <TD class=\"head\">Last</TD>\n");
      else
        {
          fprintf(fp, "      <TD class=\"head\"><A HREF=%s>Next</A></TD>\n",
                  output_file_name(file_number+1));
          fprintf(fp, "      <TD class=\"head\"><A HREF=%s>Last</A></TD>\n",
                  output_file_name(max_file_number));
        }
      
      fprintf(fp,"    </TR>\n  </tbody>\n");
      fprintf(fp, "</table>\n");
  
/*       if (top) */
/*         fprintf(fp, "<HR>\n"); */
    }

}

static void complete_file(FILE *fp)
{
  /* Close off the HTML */
  force_end_code(fp);

  if (fp)
    fprintf(fp, postamble1);

  buttons(fp, 0);

  if (fp)
    {
      time_t t;

      (void) time(&t);
      fprintf(fp, "<P>HTML generated %s</P>\n", ctime(&t));

      fprintf(fp, postamble);
      fclose(fp);
    }
}

static FILE *start_new_file(FILE *fp, int realy_open)
{
  FILE *newfp = NULL;
  static char buf[256];
  char *str;
  char *ptr;

  complete_file(fp);
  file_number++;
  if (file_number>max_file_number)
    max_file_number = file_number;

  line_number = 0;

  if (realy_open)
    {
      newfp = fopen(output_file_name(file_number), "w");

      if (!newfp) abort();

      fprintf(newfp, preamble);

      strcpy(buf, css_url);
      str = strtok_r(buf, ";", &ptr);
      while (str)
        {
          fprintf(newfp, css_link, str);
          str = strtok_r(NULL, ";", &ptr);
        }

      if (file_number == 1)
        sprintf(buf, "%s", title);
      else
        sprintf(buf, "%s continued", title);

      fprintf(newfp, preamble2, title, buf, title);

      buttons(newfp, 1);

      fprintf(newfp, preamble3);
    }
  return newfp;
}

/**********************************************************
 * main()
 *********************************************************/
static void usage(char *name)
{
  fprintf(stderr,
          "usage: %s [-s] [-lnum] [-q<l>,<r>] <input filename> <output root> <title> [<css URL>]\n",
          name);
  exit(1);
}

static void args(int argc, char **argv)
{
  int a;
  char *ptr;

  single_file = 0;
  target_lines = LINES_PER_FILE;
  strcpy(left_quote, DEFAULT_LEFT_QUOTE);
  strcpy(right_quote, DEFAULT_RIGHT_QUOTE);

  for (a=1; a<argc; a++) {
    if (argv[a][0] == '-') {
      
      if (strcmp(argv[a], "-s") == 0)
	single_file = 1;
      else if ((strncmp(argv[a], "-l", 2) == 0)) {
	target_lines = strtol(&argv[a][2], &ptr, 0);
	if (ptr == &argv[a][2])
	  usage(argv[0]);
      }
      else if (strncmp(argv[a], "-q", 2) == 0) {
	char *p, *l, *r;
	p = &argv[a][2];
	l = r = 0;
	
	while ((*p) && ((*p)!=','))
	  p++;
	
	if (*p) {
	  // Have found a ','
	  if (p > l)
	    l = &argv[a][2];
	  *p = '\0'; // Terminate left quote
	  r = p+1; // right starts in next character
	  if (!(*r))
	    r = 0;
	}
	
	if ((strlen(l)>MAX_LEN_QUOTE) || (strlen(r)>MAX_LEN_QUOTE)) {
	  fprintf(stderr, "Quote chars <%s>, <%s> are too long. (Max: %d characters)\n",
		  ((l)?l:"[none]"), ((r)?r:"[none]"), MAX_LEN_QUOTE);
	  exit(1);
	}
	
	strcpy(left_quote,  ((l) ? l : DEFAULT_LEFT_QUOTE ));
	strcpy(right_quote, ((r) ? r : DEFAULT_RIGHT_QUOTE));
      }
    } else
      break;
  }

  input_filename = NULL;
  if (a < argc)
    input_filename = argv[a++];
  else
    usage(argv[0]);

  output_root = NULL;
  if (a<argc)
    output_root = argv[a++];
  else
    usage(argv[0]);

  title = NULL;
  if (a<argc)
    title = argv[a++];
  else
    usage(argv[0]);

  css_url = "main.css;code.css";
  if (a<argc)
    css_url = argv[a++];

  if (a<argc) usage(argv[0]);
}

int main(int argc, char **argv)
{
  FILE *ifp;
  FILE *ofp = NULL;
  int token, next_token;
  int pass;
  int ident_spaces;
  int i;

  args(argc, argv);

  init_symbol_table();

  for (pass = 1; pass <= 2; pass++)
    {
      /*printf("Pass %d\n", pass);*/

      ifp = fopen(input_filename, "r");
      if (!ifp)
        {
          fprintf(stderr, "Could not open <%s> for reading\n",
                  input_filename);
          exit(1);
        }
      init_lex();
      token_number = line_number = file_number = 0;
      pending_end_code = -1;
      ofp = start_new_file(NULL, (pass>1));
      
      next_token = lex( ifp );
      do
        {
          token = next_token;

          ident_spaces = 1;
          while (ident_spaces)
            {
              next_token = lex( ifp );
              if ((token==IDENTIFIER) && (next_token==IDENTIFIER))
                {
                  /* patch up spaces in an identifier */
                  for (i=0; i<spaces2; i++)
                    strcat(lex_buf, " ");
                  strcat(lex_buf, lex_buf2);

                  strcpy(lex_buf2, lex_buf);
                  spaces2 = spaces;
                }
              else
                ident_spaces = 0;
            }

          render_token(token, next_token, &ofp);
        } while (token != EOF);
      
      complete_file(ofp);
      fclose(ifp);

      record_procedure_symbols(); /* so globals in main prog. aren't deleted in pass 2 */
    }

  exit(0);
}
