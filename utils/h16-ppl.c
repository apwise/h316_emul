#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_LEN_QUOTE 20
#define DEFAULT_LEFT_QUOTE "`"
#define DEFAULT_RIGHT_QUOTE "'"

#define DEFAULT_HEADING "M4H_DEFAULT_HEADING"
#define DEFAULT_TOC_TITLE "M4H_DEFAULT_TOC_TITLE"

#define HEAD_TAG       "M4H_HEAD_TAG"
#define HEAD_TAG_END   "M4H_HEAD_TAG_END"
#define TABLE_START    "M4H_TABLE_START"
#define TABLE_STOP     "M4H_TABLE_STOP"
#define BUTTON_START   "M4H_BUTTON_START"
#define BUTTON_STOP    "M4H_BUTTON_STOP"
#define BUTTON_EMPTY   "M4H_BUTTON_EMPTY"
#define LINK_START     "M4H_LINK_START"
#define LINK_STOP      "M4H_LINK_STOP"

#define TOC_TABLE_START "M4H_TOC_TABLE_START"
#define TOC_TABLE_STOP "M4H_TOC_TABLE_STOP"
#define TOC_ROW_START  "M4H_TOC_ROW_START"
#define TOC_ROW_STOP   "M4H_TOC_ROW_STOP"

#define NEXT_PAGE      "M4H_NEXT_PAGE"
#define PREVIOUS_PAGE  "M4H_PREVIOUS_PAGE"
/*
  #define NEXT_FILE      "M4H_NEXT_FILE"
  #define PREVIOUS_FILE  "M4H_PREVIOUS_FILE"
  #define SINGLE_PAGE    "M4H_SINGLE_PAGE"
  #define MULTIPLE_PAGES "M4H_MULTIPLE_PAGES"
*/
#define FIRST_PAGE      "M4H_FIRST_PAGE"
#define LAST_PAGE       "M4H_LAST_PAGE"

#define HTML_HEADING   "HTML_HEADING"
#define HTML_TIME      "HTML_TIME"

#define CONTINUED " continued"
#define CONCLUDED " concluded"

#define HEAD1 "M4H_PPL_HEAD1"
#define HEAD2 "M4H_HEAD2"
#define HEAD3 "M4H_HEAD3"
#define TAIL1 "M4H_TAIL1"
#define TAIL2 "M4H_TAIL2"

static int single_file;
static int target_lines;
static char left_quote[MAX_LEN_QUOTE+1];
static char right_quote[MAX_LEN_QUOTE+1];
static char *input_filename;
static char *output_root;
static char *title;

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
static char *m4h_output_file_name(int file_num);

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

static void free_lex()
{
  free(lex_buf);
  free(lex_buf2);
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
  struct Symbol *next;
  struct Symbol *locals; // Only used in an ST_PROCEDURE
  struct Symbol *parent; // Only used in an ST_PROCEDURE

  char *name;
  enum SymbolType type;
  bool defined;

  int token_number; 
  int line_number;
  int file_number;
};

static struct Symbol *globals = NULL;
static int procedure_depth = 0;
static char *procedure_name = NULL;
static struct Symbol *procedure = NULL;

static struct Symbol *add_symbol(char *name, enum SymbolType type)
{
  //printf("add_symbol(%s) %d %s\n", name, procedure_depth, procedure_name);
  struct Symbol *s = malloc(sizeof(struct Symbol));
  if (!s) abort();
  
  s->next   = NULL;
  s->locals = NULL;
  s->parent = NULL;
  s->name = strdup(name);
  s->type = type;
  s->defined = true;
  s->token_number = token_number;
  s->line_number = line_number;
  s->file_number = file_number;

  struct Symbol **head = (procedure_depth == 0) ? &globals : &procedure->locals;
  s->next = *head;
  *head = s;

  return s;
}

static struct Symbol *lookup_full(char *name, bool *global)
{
  int i = (procedure_depth == 0);

  while (i < 2) {
    // In a procedure lookup locals first, then globals
    struct Symbol *s = (i == 0) ? procedure->locals : globals;

    while (s) {
      if (strcmp(s->name, name)==0) {
        if (global) {
          *global = (i!=0);
        }
        return s;
      }
      s = s->next;
    }
    
    ++i;
  }

  return NULL;
}

static struct Symbol *lookup(char *name)
{
  return lookup_full(name, NULL);
}

static void enter_procedure(const char *name, struct Symbol *s)
{
  s->parent = procedure;
  procedure = s;
  ++procedure_depth;

  if (!procedure_name) {
    procedure_name = strdup(name);
  } else {
    char *tmp = malloc(strlen(procedure_name) + strlen(name) + 2);
    strcpy(tmp, procedure_name);
    strcat(tmp, "_");
    strcat(tmp, name);
    free(procedure_name);
    procedure_name = tmp;
  }
  //printf("enter_procedure: %s\n", procedure_name);
}

static void exit_procedure()
{
  if (procedure_depth>0) {
    --procedure_depth;

    if (procedure_depth == 0) {
      free(procedure_name);
      procedure_name = NULL;
    } else {
      int l = strlen(procedure_name);
      int i = l - strlen(procedure->name);

      // Sanity testing
      if ((i>1) && (strcmp(procedure->name, &procedure_name[i]) == 0) &&
          (procedure_name[i-1] == '_')) {
        char *tmp = malloc(i);
        strncpy(tmp, procedure_name, i-1);
        tmp[i-1] = '\0';
        free(procedure_name);
        procedure_name = tmp;
      }
    }
    //printf("exit_procedure: %s\n", procedure_name);
  }

  procedure=procedure->parent;
}


/**********************************************************
 * Rendering routine
 *********************************************************/
void char_sub(int c, FILE *fp)
{
  if (!fp) return;

  switch(c) {
  case  ' ': fprintf(fp, "&nbsp;"); break;
  case '\t': fprintf(fp, "&nbsp;"); break;
  case  '<': fprintf(fp, "&lt;");   break;
  case  '>': fprintf(fp, "&gt;");   break;
  case  '&': fprintf(fp, "&amp;");  break;
  case  '"': fprintf(fp, "&quot;"); break;
  default  : putc(c, fp);
  }
}

static void copy_str(FILE *fp, char *str)
{
  int i = 0;
  while (str[i]) {
    char_sub(str[i] & 0xff, fp);
    i++;
  }
}

static void copy_token(FILE *fp)
{
  copy_str(fp, lex_buf);
}

static bool doindent(int token, char *buf)
{
  if (token == '(')
    return true;

  if ((token == RESERVED) && (strcmp(buf, ".BEGIN")==0))
    return true;

  return false;
}

static bool unindent(int token, char *buf)
{
  if (token == ')')
    return true;

  if ((token == RESERVED) && (strcmp(buf, ".END")==0))
    return true;

  return false;
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
       (class != pending_end_code) ) {
    force_end_code(fp);
  }

  if ( (pending_end_code == (-1)) ||
       (class != pending_end_code) ) {
    if (fp) {
      if (class) {
        fprintf(fp, "<span CLASS=%c>", class);
      }
    }
  }
  pending_end_code = -1;
}

static void end_code(FILE *fp, int class)
{
  pending_end_code = class;
}


static bool good_file_break(int token, int token2)
{
  int r = false;
  
  if ((token == RESERVED) && (strcmp(lex_buf, ".ORIGIN")==0)) {
    r = true;
  } else if ((token == RESERVED) && (strcmp(lex_buf, ".PROCEDURE")==0)) {
    r = true;
  } else if ((token2 == RESERVED) && (strcmp(lex_buf2, ".PROCEDURE")==0)) {
    if ((token == '@') ||
        ((token == RESERVED) && (strcmp(lex_buf, ".CONDITIONAL")==0))) {
      r = true;
    }
  } else if ((token == '@') &&
             ((token2 == RESERVED) && (strcmp(lex_buf2, ".CONDITIONAL")==0))) {
    r = true;
  }
  
  return r;
}

char *ref_name(char *name, bool global, char *pn)
{
  static char buf[256];
  int i;

  if (global) {
    sprintf(buf, "%s", name);
  } else {
    sprintf(buf, "%s_%s", pn, name);
  }
  
  for (i=0; buf[i]; i++) {
    if (buf[i] == ' ') {
      buf[i] = '_';
    }
  }
  return buf;
}

void render_token(int token, int next_token, FILE **pfp)
{
  int i;
  
  bool set_line_start         = false;
  static bool line_start      = false;
  static bool procedure_seen  = false;
  static bool forward_seen    = false;
  static bool compconst_seen  = false;
  static bool constant_seen   = false;
  static bool for_seen        = false;
  static bool integer_seen    = false;
  static bool array_seen      = false;
  static bool set_seen        = false;
  static bool lsb_seen        = false;
  static bool lrb_seen        = false;
  static bool address_seen    = false;
  static bool percent_seen    = false;
  static bool label_seen      = false;
  static bool goto_seen       = false;
  static bool switch_seen     = false;
  static bool equals_seen     = false;

  static int indent           = 0;
  static int procedure_begins = 0;

  struct Symbol *s;
  FILE *fp = *pfp;

  token_number ++;

  if (line_start) {
    /* Start a new file ? */
    if ((line_number > target_lines) &&
        good_file_break(token, next_token) &&
        (!single_file)) {
      fp = (*pfp) = start_new_file(fp, (fp!=0));
    }
    
    /* If the first token on this line
     * un-indents then do it now, before printing
     * the indenting spaces */
    
    if (unindent(token, lex_buf))
      indent--;
    
    for (i=0; i<indent; i++) {
      char_sub(' ', fp);
      char_sub(' ', fp);
    }
  } else {
    if (unindent(token, lex_buf)) {
      indent--;
    }
    
    /* render any leading spaces */
    if (spaces > 0) {
      if (pending_end_code == -1) {
        start_code(fp, 0);
      }
      
      for (i=0; i<spaces; i++) {
        char_sub(' ', fp);
      }
    }
  }

  if (doindent(token, lex_buf)) {
    indent++;
  }
  
  switch (token) {
  case '\n':
    if (fp) fprintf(fp, "\n");
    set_line_start = 1;
    line_number++;
    break;
    
  case EOF:
    break;
    
  case RESERVED:
    start_code(fp, 'R');
    copy_token(fp);
    end_code(fp,'R');
    
    if (strcmp(".PROCEDURE",        lex_buf)==0) {
      procedure_seen = true;
    } else if (strcmp(".FORWARD",   lex_buf)==0) {
      forward_seen = true;
    } else if (strcmp(".COMPCONST", lex_buf)==0) {
      compconst_seen = true;
    } else if (strcmp(".CONSTANT",  lex_buf)==0) {
      constant_seen = true;
    } else if (strcmp(".ARRAY",     lex_buf)==0) {
      array_seen = true;
    } else if (strcmp(".SET",       lex_buf)==0) {
      set_seen = true;
    } else if (strcmp(".ADDRESS",   lex_buf)==0) {
      address_seen = true;
    } else if (strcmp(".BEGIN",     lex_buf)==0) {
      if ((procedure_begins==0) && (!procedure)) {
        //printf("start of main\n");
        if (fp) {
          s = lookup("");
        } else {
          s = add_symbol("", ST_PROCEDURE);
        }
        enter_procedure("",s);
      }
      procedure_begins++;
    } else if (strcmp(".END", lex_buf)==0) {
      //printf(".END procedure_begins = %d\n", procedure_begins);
      if (procedure_begins>0) {
        procedure_begins--;
        if (procedure_begins <= 0) {
          exit_procedure();
        }
      }
      procedure_seen = false;
      forward_seen   = false;
      compconst_seen = false;
      constant_seen  = false;
      for_seen       = false;
      integer_seen   = false;
      array_seen     = false;
      set_seen       = false;
      lsb_seen       = false;
      lrb_seen       = false;
      address_seen   = false;
      percent_seen   = false;
      label_seen     = false;
      goto_seen      = false;
      switch_seen    = false;
      equals_seen    = false;
    } else if (strcmp(".FOR", lex_buf)==0) {
      for_seen = true;
    } else if (strcmp(".INTEGER", lex_buf)==0) {
      integer_seen = true;
    } else if (strcmp(".LABEL", lex_buf)==0) {
      label_seen = true;
    } else if (strcmp(".GOTO", lex_buf)==0) {
      goto_seen = true;
    } else if (strcmp(".SWITCH", lex_buf)==0) {
      switch_seen = true;
    }
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
    
  case IDENTIFIER: {
    bool gen_hyper = false;
    bool gen_name  = false;
    bool global    = false;

    if (!fp) { // i.e. on first pass

      if (integer_seen) {
        s = add_symbol(lex_buf, ST_INTEGER);
      } else if ((for_seen) &&
                 ((!lookup_full(lex_buf, &global)) || global)) {
        // Implicitly defined INTEGER as a FOR control variable
        //printf(".FOR %s\n", lex_buf);
        s = add_symbol(lex_buf, ST_INTEGER);
      } else if ((array_seen) && (!address_seen) &&
                 (!lsb_seen) && (!lrb_seen)) {
        s = add_symbol(lex_buf, ST_ARRAY);
        if ((next_token==RESERVED) && (strcmp(lex_buf2, ".LATER")==0)) {
          s->defined = false;
        }
      } else if ((label_seen) || (goto_seen) ||
                 (switch_seen && equals_seen)) {
        //printf("label declaration: %s (%s)\n", lex_buf, procedure_name);
        if ((label_seen) || (!lookup(lex_buf))) {
          s = add_symbol(lex_buf, ST_LABEL);
          s->defined = false; // Only a declaration
        }
      } else if (switch_seen && (!equals_seen)) {
        /* This is a switch identifer
         * (rather than a label implicitly declared
         * in a switch statement) */
        s = add_symbol(lex_buf, ST_SWITCH);
      } else if (next_token==':') {
        // This looks like a label definition
        if ( (s=lookup_full(lex_buf, &global)) ) {
          if (!s->defined) {
            //printf("defined %s %d\n", lex_buf, token_number);
            s->defined = true;
            s->token_number = token_number;
            s->line_number = line_number;
            s->file_number = file_number;                       
          } else if (global) {
            /* must be a local of same name as global?*/
            //printf("Local %s masks global? %d\n", lex_buf, s->type);
            s = add_symbol(lex_buf, ST_LABEL);
          } else {
            fprintf(stderr, "Why? %s\n", lex_buf);
          }
        } else {
          s = add_symbol(lex_buf, ST_LABEL);
        }
      } else if (compconst_seen) {
        s = add_symbol(lex_buf, ST_COMPCONST);
      } else if (constant_seen) {
        s = add_symbol(lex_buf, ST_CONSTANT);
      } else if (procedure_seen && (!forward_seen)) {
        s = add_symbol(lex_buf, ST_PROCEDURE);
        enter_procedure(lex_buf, s);
      } else if ((set_seen) && (!lsb_seen) && (!lrb_seen)) {
        if ( (s=lookup(lex_buf)) ) {
          if ((s->type == ST_ARRAY) && (!s->defined)) {
            /* Reached .SET of a .LATER .ARRAY */
            s->defined      = true;
            s->token_number = token_number;
            s->line_number  = line_number;
            s->file_number  = file_number;
          } else {
            fprintf(stderr, "Why am I here %s %d?\n", lex_buf, s->type);
          }
        } else {
          fprintf(stderr, ".SET but no .ARRAY? %s\n", lex_buf);
        }
      }
    } else { /* pass 2 */
      if (procedure_seen && (!forward_seen)) {
        s=lookup(lex_buf);
        enter_procedure(lex_buf, s);
      }
      
      if ( (s=lookup_full(lex_buf, &global)) ) {
        switch (s->type) {
        case ST_PROCEDURE:
        case ST_CONSTANT:
        case ST_COMPCONST:
        case ST_INTEGER:
        case ST_ARRAY:
        case ST_LABEL:
        case ST_SWITCH:
          if (!percent_seen) {
            gen_hyper = true;
            if (token_number == s->token_number) {
              // LABEL is the only identifier
              // type that can be forward referenced
              // and local to a procedure.
              if ((s->type == ST_LABEL) && (!s->defined))
                gen_name = false;
              else
                gen_name = true;
            }
          }
          break;
            
        default:
          abort();
        }
      }
      start_code(fp, 'I');
      if (gen_hyper) {
        if (gen_name)
          fprintf(fp, "<A NAME=\"%s\">",
                  ref_name(lex_buf, global, procedure_name));
        else
          fprintf(fp, "<A HREF=\"%s#%s\">",
                  ((file_number != s->file_number) ?
                   output_file_name(s->file_number) : ""),
                  ref_name(lex_buf, global, procedure_name));
      }
      copy_token(fp);
      if (gen_hyper) {
        fprintf(fp,"</A>");
      }
      end_code(fp,'I');
    }
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
    for_seen = false;
    break;
    
  default:
    start_code(fp, 0);
    char_sub(token, fp);
    end_code(fp, 0);

    if (token == ';') {
      procedure_seen = false;
      forward_seen   = false;
      compconst_seen = false;
      constant_seen  = false;
      for_seen       = false;
      integer_seen   = false;
      array_seen     = false;
      set_seen       = false;
      lsb_seen       = false; /* just in case */
      lrb_seen       = false; /* just in case */
      address_seen   = false; /* just in case */
      percent_seen   = false;
      label_seen     = false;
      goto_seen      = false;
      switch_seen    = false;
      equals_seen    = false;
    } else if (token == ',') {
      address_seen = false;
      percent_seen = false;
    } else if (token == '[') {
      lsb_seen = true;
    } else if (token == ']') {
      lsb_seen = false;
    } else if (token == '(') {
      lrb_seen = true;
    } else if (token == ')') {
      lrb_seen = false;
    } else if (token == '%') {
      percent_seen = true;
    } else if (token == '=') {
      equals_seen = true;
    }
    break;
  }
  
  line_start = set_line_start;
}

/**********************************************************
 * Handle output files
 *********************************************************/

static char *output_file_name(int file_num)
{
  static char buf[40];
  if (single_file) {
    sprintf(buf, "%s.html", output_root);
  } else {
    sprintf(buf, "%s_%d.html", output_root, file_num);
  }
  return buf;
}

static char *m4h_output_file_name(int file_num)
{
  static char buf[40];
  if (single_file) {
    sprintf(buf, "%s.m4h", output_root);
  } else {
    sprintf(buf, "%s_%d.m4h", output_root, file_num);
  }
  
  return buf;
}

static void button(FILE *fp, const char *legend, const char *filename)
{
  char ref[1024];
  char eref[10];

  if (filename) {
    sprintf(ref, "<a href=%s>", filename);
    sprintf(eref, "</a>");
  } else {
    ref[0] = '\0';
    eref[0] = '\0';
  }
  
  fprintf(fp, "      %s%s%s%s%s%s%s%s%s\n",
          BUTTON_START, left_quote, right_quote,
          ref, legend, eref,
          left_quote, right_quote, BUTTON_STOP);
}

static void buttons(FILE *fp, int top)
{
  if (!fp) return;
      
  if (!single_file) {
    fprintf(fp, "%s\n", TABLE_START);
    
    button(fp, FIRST_PAGE,    ((file_number == 1) ? NULL : output_file_name(1)));
    button(fp, PREVIOUS_PAGE, ((file_number == 1) ? NULL : output_file_name(file_number-1)));
    fprintf(fp, "      %s\n", BUTTON_EMPTY);
    button(fp, NEXT_PAGE,     ((file_number == max_file_number) ? NULL : output_file_name(file_number+1)));
    button(fp, LAST_PAGE,     ((file_number == max_file_number) ? NULL : output_file_name(max_file_number)));
      
    fprintf(fp, "%s\n", TABLE_STOP);
  }

}

static void complete_file(FILE *fp)
{
  /* Close off the HTML */
  force_end_code(fp);

  if (fp) {
    fprintf(fp, "%s\n", TAIL1);
    buttons(fp, 0);
    fprintf(fp, "%s\n", TAIL2);
  };
}

static FILE *start_new_file(FILE *fp, int realy_open)
{
  FILE *newfp = NULL;
  time_t t;
  char buf[100];

  complete_file(fp);
  file_number++;
  if (file_number>max_file_number)
    max_file_number = file_number;

  line_number = 0;

  if (realy_open) {
    newfp = fopen(m4h_output_file_name(file_number), "w");

    if (!newfp) abort();

    fprintf(newfp, "m4_define(%s%s%s, %s%s%s%s)\n",
            left_quote,  HTML_HEADING, right_quote,
            left_quote,  title,
            ((file_number == 1) ? "" : (file_number == max_file_number) ? CONCLUDED : CONTINUED),
            right_quote);
    
    (void) time(&t);
    ctime_r(&t, buf);
    if ((*buf) && (buf[strlen(buf)-1]=='\n'))
      buf[strlen(buf)-1]='\0';

    fprintf(newfp, "m4_define(%s%s%s, %s%s%s)\n",
            left_quote,  HTML_TIME, right_quote,
            left_quote,  buf, right_quote);

    fprintf(newfp, "%s\n", HEAD1);

    /* Links would go here if there were any */

    fprintf(newfp, "%s\n", HEAD2);
    buttons(newfp, 1);
    fprintf(newfp, "%s\n", HEAD3);

    /*
      fprintf(newfp, "%s%s%s%s%s\n",
      HEAD_TAG, left_quote, title, right_quote, HEAD_TAG_END);
    */
  }
  return newfp;
}

/**********************************************************
 * main()
 *********************************************************/
static void usage(char *name)
{
  fprintf(stderr,
          "usage: %s [-s] [-lnum] [-q<l>,<r>] <input filename> <output root> <title>\n",
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

  for (pass = 1; pass <= 2; pass++) {
    /*printf("Pass %d\n", pass);*/

    ifp = fopen(input_filename, "r");
    if (!ifp) {
      fprintf(stderr, "Could not open <%s> for reading\n",
              input_filename);
      exit(1);
    }
    init_lex();
    token_number = line_number = file_number = 0;
    pending_end_code = -1;
    ofp = start_new_file(NULL, (pass>1));
      
    next_token = lex( ifp );
    do {
      token = next_token;

      ident_spaces = 1;
      while (ident_spaces) {
        next_token = lex( ifp );
        if ((token==IDENTIFIER) && (next_token==IDENTIFIER)) {
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
    free_lex();
  }

  exit(0);
}
