/* Convert Honeywell Series-16 DAP assembler code to HTML
 *
 * Copyright (C) 2004, 2006, 2018  Adrian Wise
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

// {{{ TODO

/*
 * Pop-up alternate forms for literals?
 */

// }}}

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
#define NEXT_FILE      "M4H_NEXT_FILE"
#define PREVIOUS_FILE  "M4H_PREVIOUS_FILE"
#define SINGLE_PAGE    "M4H_SINGLE_PAGE"
#define MULTIPLE_PAGES "M4H_MULTIPLE_PAGES"

#define HTML_HEADING   "HTML_HEADING"
#define HTML_TIME      "HTML_TIME"

#define HEAD1 "M4H_HEAD1"
#define HEAD2 "M4H_HEAD2"
#define HEAD3 "M4H_HEAD3"
#define TAIL1 "M4H_TAIL1"
#define TAIL2 "M4H_TAIL2"

// {{{ Includes

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <cstring>
#include <cctype>
#include <list>
#include <map>

#include <iostream>
using namespace std;

// }}}

// {{{ struct Instr

enum INSTR_TYPE
  {
    GA, GB, MR, IOG, SH, IO, SK,
    AD
  };

struct Instr
{
  const char *mnemonic;
  INSTR_TYPE type;
  const char *description;
};

// }}}
// {{{ static Instr instructions[] = 

static Instr instructions[] = 
{
  {"CRA", GA, "Clear A"},
  {"IAB", GB, "Interchange A and B"},
  {"IMA", MR, "Interchange memory and A"},
  {"INK", GB, "Input keys"},
  {"LDA", MR, "Load A"},
  {"LDX", MR, "Load X"},    //T=1
  {"OTK", IOG,"Output keys"},
  {"STA", MR, "Store A"},
  {"STX", MR, "Store X"},   //T=0
  {"ACA", GA, "Add C to A"},
  {"ADD", MR, "Add"},
  {"AOA", GA, "Add one to A"},
  {"SUB", MR, "Subtract"},
  {"TCA", GA, "Two's complement A"},
  {"ANA", MR, "And"},
  {"CSA", GA, "Copy sign and set sign plus"},
  {"CHS", GA, "Complement A's sign"},
  {"CMA", GA, "Complement A"},
  {"ERA", MR, "Exclusive OR"},
  {"SSM", GA, "Set sign minus"},
  {"SSP", GA, "Set sign plus"},
  {"ALR", SH, "Logical left rotate"},
  {"ALS", SH, "Arithmetic left shift"},
  {"ARR", SH, "Logical right rotate"},
  {"ARS", SH, "Arithmetic right shift"},
  {"LGL", SH, "Logical left shift"},
  {"LGR", SH, "Logical right shift"},
  {"LLL", SH, "Long left logical shift"},
  {"LLR", SH, "Long left rotate"},
  {"LLS", SH, "Long arithmetic left shift"},
  {"LRL", SH, "Long right logical shift"},
  {"LRR", SH, "Long right rotate"},
  {"LRS", SH, "Long arithmetic right shift"},
  {"INA", IO, "Input to A"},
  {"OCP", IO, "Output control pulse"},
  {"OTA", IO, "Output from A"},
  {"SMK", IO, "Set mask"},
  {"SKS", IO, "Skip if ready line set"},
  {"CAS", MR, "Compare and skip"},
  {"ENB", GB, "Enable interrupts"},
  {"HLT", GB, "Halt"},
  {"INH", GB, "Inhibit interrupts"},
  {"IRS", MR, "Increment, replace and skip"},
  {"JMP", MR, "Jump"},
  {"JST", MR, "Jump and store"},
  {"NOP", SK, "No operation"},
  {"RCB", GA, "Reset C bit"},
  {"SCB", GA, "Set C bit"},
  {"SKP", SK, "Skip"},
  {"SLN", SK, "Skip if last non-zero"},
  {"SLZ", SK, "Skip if last zero"},
  {"SMI", SK, "Skip if A minus"},
  {"SNZ", SK, "Skip if A non-zero"},
  {"SPL", SK, "Skip if A plus"},
  {"SR1", SK, "Skip if sense switch 1 reset"},
  {"SR2", SK, "Skip if sense switch 2 reset"},
  {"SR3", SK, "Skip if sense switch 3 reset"},
  {"SR4", SK, "Skip if sense switch 4 reset"},
  {"SRC", SK, "Skip if C reset"},
  {"SS1", SK, "Skip if sense switch 1 set"},
  {"SS2", SK, "Skip if sense switch 2 set"},
  {"SS3", SK, "Skip if sense switch 3 set"},
  {"SS4", SK, "Skip if sense switch 4 set"},
  {"SSC", SK, "Skip if C set"},
  {"SSR", SK, "Skip if no sense switch set"},
  {"SSS", SK, "Skip is any sense switch set"},
  {"SZE", SK, "Skip if A zero"},
  {"CAL", GA, "Clear A left half"},
  {"CAR", GA, "Clear A right half"},
  {"ICA", GA, "Interchange characters in A"},
  {"ICL", GA, "Interchange and clear left half of A"},
  {"ICR", GA, "Interchange and clear right half of A"},
  
  {"CF1", AD, "Configuration DDP-116"},
  {"CF3", AD, "Configuration H316"},
  {"CF4", AD, "Configuration DDP-416"},
  {"CF5", AD, "Configuration DDP-516"},
  {"REL", AD, "Relocatable mode"},
  {"ABS", AD, "Absolute mode"},
  {"LOAD", AD, "Load mode"},
  {"ORG", AD, "Origin"},
  {"FIN", AD, "Assemble literals"},
  {"MOR", AD, "More (source) required"},
  {"END", AD, "End of source"},
  {"EJCT", AD, "Start at top of page"},
  {"LIST", AD, "Generate assembly listing"},
  {"NLST", AD, "Generate no assembly listing"},
  {"EXD", AD, "Enter extended desectorizing"},
  {"LXD", AD, "Leave extended desectorizing"},
  {"SETB", AD, "Set base sector"},
  {"EQU", AD, "Give symbol a permanent value"},
  {"SET", AD, "Give symbol a temporary value"},
  {"DAC", AD, "Address constant"},
  {"DEC", AD, "Decimal constant"},
  {"DBP", AD, "Double precision constant"},
  {"OCT", AD, "Octal constant"},
  {"HEC", AD, "Hexadecimal constant"},
  {"BCI", AD, "Binary coded information"},
  {"VFD", AD, "Variable field constant"},
  {"BSS", AD, "Block starting with symbol"},
  {"BES", AD, "Block ending with symbol"},
  {"BSZ", AD, "Block storage of zeros"},
  {"COMN", AD, "Common"},
  {"SETC", AD, "Set common base"},
  {"ENT", AD, "Entry point"},
  {"SUBR", AD, "Entry point"},
  {"EXT", AD, "External name"},
  {"XAC", AD, "External address constant"},
  {"CALL", AD, "Call external subroutine"},
  {"IFP", AD, "Assemble only if Plus"},
  {"IFM", AD, "Assemble only if Minus"},
  {"IFZ", AD, "Assemble only if Zero"},
  {"IFN", AD, "Assemble only if Not Zero"},
  {"ENDC", AD, "End of conditional assembly"},
  {"ELSE", AD, "Else - conditional assembly"},
  {"FAIL", AD, "Statement that should never be assembled"},
  {"***", AD, "Opcode Zero"},
  {"PZE", AD, "Opcode Zero"},

  {0,GA,0}
};

// }}}

class Symbol;

// {{{ class Annotation

class Annotation
{
 public:
  enum A
    {
      A_SLN,
      A_ERROR,
      A_ADDR,
      A_OBJ,
      A_LABEL,
      A_OPC,
      A_COMMENT,
      A_SYMBOL,
      A_GSYMBOL
    };

  Annotation(enum A type, int first, int len);
  A get_type() const {return type;};
  int get_first() const {return first;};
  int get_len() const {return len;};
  Symbol *get_s() const {return s;};
  void set_s(Symbol *s) {this->s = s;};
  void shorten(int n){len-=n;};

 private:
  enum A type;
  int first;
  int len;
  Symbol *s;
};

// }}}

class File;

// {{{ class Line

class Line
{
 public:
  Line(const string src_line, class File *file, int page);
  void analyze();
  void analyze_src();
  void render(FILE *fp, bool html_per_page, bool first);

  Annotation *find_field(int pos, int len, Annotation::A annot);

  enum LINE_TYPE
    {
      LINE_BLANK,
      LINE_COMMENT,
      LINE_SRC,
      LINE_HEAD,
      LINE_SYMBOL,
      LINE_INFO
    };

  LINE_TYPE get_type() const {return type;};
  string get_src_line() const {return src_line;};

  enum SF_TYPE
    {
      SF_NORMAL,
      SF_BCI
    };
  
  int find_sub_field(int first, SF_TYPE type,
                     bool &symbol, bool &literal, int &num, bool &asterix);
  string render_link(Symbol *s, bool html_per_page);
  
 private:
  const string src_line;
  int src_line_num;

  class File *file;
  const int page;

  enum LINE_TYPE type;

  bool indirect;
  bool indexed;
  Instr *instr;

  map<int, Annotation *> annotations;

  void render_char(FILE *fp, char c);
};

// }}}
// {{{ class File

class File
{
 public:
  File(const char *filename, bool is_toc=false);
  string read_line(FILE *fp, bool &ok);
  bool get_end_read(){return end_read;};
  bool get_end_really_read(){return end_really_read;};
  bool get_is_toc(){return is_toc;};
  void set_end_read(){end_read=true;};
  void set_end_really_read(){end_really_read=true;};
  void set_is_toc(){is_toc=true;};
  void render();
  void incr_pages(){pages++;};
  Symbol *lookup(string name);
  Symbol *lookup_local(string name);
  Symbol *lookup_global(string name);
  Symbol *define(string name);
  void redefine(Symbol *s);
  Symbol *declare_local(string name, bool ext);
  Symbol *declare_global(string name, Symbol *local);
  Symbol *local_synonym(string name, Symbol *local);
  void define_symbol(Symbol *s);
  string get_html_name(int page=0);
  void file_title();
  void set_use_comment_as_title(bool tc);
  void supply_title(string title);

  enum RENDER_STATE
    { RS_WAIT_HEAD,
      RS_AFTER_HEAD,
      RS_WAIT_SRC,
      RS_SRC };

  enum RENDER_STATE get_render_state(){return render_state;};
  void set_render_state(enum RENDER_STATE state){render_state = state;};

 private:
  const string filename;
  string title;
  list<Line *> lines;
  string heading_line;

  bool end_really_read; // And lines with literals after END
  int pages;
  bool is_toc;
  bool use_comment_as_title;
  bool end_read;
  bool use_supplied_title;
  string supplied_title;

  string basename(string filename);
  enum RENDER_STATE render_state;

  map<string, Symbol *> symbol_table;
  static map<string, Symbol *> global_symbol_table;

  string find_first_file_name(bool html_per_page, string &title_str);
  string find_last_file_name(bool html_per_page, string &title_str);
  string find_file_name(bool previous, bool html_per_page, int page, string &title_str);
  void links(FILE *fp, bool html_per_page, int page);
  void link_button(FILE *fp, string filename, const char *str, const string &title_str);
  void link_buttons(FILE *fp, bool html_per_page, int page);
  void file_start(FILE *fp, bool html_per_page, int page);
  void file_stop(FILE *fp, bool html_per_page, int page);
  void quote_dollars(const string &in_str, string &out_str);
};

// }}}
// {{{ class Symbol 

class Symbol 
{
 public:
  Symbol(string name, int page, File *file, bool ext=false, bool defined=true);

  const string *get_name() const {return &name;};
  int get_page();
  File *get_file() const {return file;};
  bool get_ext() const {return ext;};
  bool get_defined() const {return defined;};
  bool get_multiply_defined() const {return multiply_defined;};
  int get_serial_number();

  void set_page(int page){this->page = page;};
  void set_defined(bool defined){this->defined = defined;};

  void redefine(int new_page);
  void md_step();

 private:
  string name;
  int    page;
  File  *file;             /* 0 = local */
  bool   ext;              /* Symbol is EXT - lookup on global */
  bool   defined;          /* Symbol defined (if false only declared) */
  bool   multiply_defined; /* e.g. by SET */

  int  serial_number;

  typedef std::pair<int , int> md_item_t;
  typedef std::list<md_item_t> md_list_t;
  md_list_t md_list;
  bool md_iterating;
  md_list_t::iterator md_itrt;

  static int next_serial_number;
};

// }}}

static File **files;
static int n_files;
static char left_quote[MAX_LEN_QUOTE+1];
static char right_quote[MAX_LEN_QUOTE+1];

// {{{ Annotation::Annotation(enum A type, int first, int len)

Annotation::Annotation(enum A type, int first, int len)
  : type(type),
    first(first),
    len(len),
    s(0)
{
  
}

// }}}

// {{{ static char *get_substr(const string st, int n, int l)

static char *get_substr(const string st, int n, int l)
{
  int len;
  string s;
  const char *str;
  char *r;

  len = st.size();

  if (n < len)
    {
      s = st.substr(n, (l==-1)?string::npos:l);
      str = s.c_str();
      r = new char [s.size() + 1];
      strcpy(r, str);
      return r;
    }
  else
    return 0;
}

// }}}
// {{{ static bool number(const string str,

static bool number(const string str,
                   unsigned long &n,
                   int first, int length, int base)
{
  bool r = false;
  const char *s = get_substr(str, first, length);
  char *ptr;

  if (s)
    {
      n = strtoul(s, &ptr, base);
      delete [] s;

      if (ptr > s)
        {
          if (length == -1)
            r = true;
          else
            r = ((ptr-s) == length);
        }
    }

  return r;
}

// }}}

// {{{ Line::Line(const string src_line, File *file)

Line::Line(const string src_line, File *file, int page)
  : src_line(src_line),
    src_line_num(-1),
    file(file),
    page(page),
    indirect(false),
    indexed(false),
    instr(0)
{
  //printf("Line constructor\n");
};

// }}}
// {{{ void Line::analyze()

#define SLN        4
#define SLN_LEN    4
#define ADDR       9
#define ADDR_LEN   5
#define OBJ        16
#define OBJ_LEN    11
#define LBL        28
#define LBL_LEN    5
#define OPC        33
#define OPC_LEN    5
#define ARG        39

#define ST_ADDR    11
#define ST_ADDR_LEN 5

void Line::analyze()
{
  bool ok;
  unsigned long n;
  int len;

  len = src_line.size();

  if (len > 0)
    {
      if (file->get_end_really_read())
        {
          // Is there an address (for a symbol-table entry)?
          
          ok = number(src_line, n, ST_ADDR, ST_ADDR_LEN, 8);
          if (ok)
            type = LINE_SYMBOL;
          else if (src_line[SLN] == ' ')
            type = LINE_HEAD;
          else
            type = LINE_INFO;
        }
      else if (file->get_end_read())
        {
          // Is there an address ?
          ok = number(src_line, n, ADDR, ADDR_LEN, 8);

          if ((src_line[SLN] == ' ') && (ok))
            {
              /*
               * No source line number, but addresses,
               * Like FIN, END, BCI, etc.
               */
              type = LINE_SRC;
            }
          else
            { 
              ok = number(src_line, n, ST_ADDR, ST_ADDR_LEN, 8);
              if (ok)
                {
                  file->set_end_really_read();
                  type = LINE_SYMBOL;
                }
              else if (src_line[SLN] == ' ')
                type = LINE_HEAD;
              else
                {
                  file->set_end_really_read();
                  type = LINE_INFO;
                }     
            }         
        }
      else
        {
          // Has it a source line number?
          
          ok = number(src_line, n, SLN, SLN_LEN, 10);
          
          if (ok)
            {
              src_line_num = n;
  
              /*
               * Add an annotation for the source line number
               */
              Annotation *a;
              a = new Annotation(Annotation::A_SLN, SLN, SLN_LEN);
              pair<int, Annotation *>pp(SLN, a);
              annotations.insert(pp);
              
              if ((len > LBL) && (src_line[LBL] == '*'))
                type = LINE_COMMENT;
              else
                type = LINE_SRC;
            }
          else
            {
              // Is there an address ?
              ok = number(src_line, n, ADDR, ADDR_LEN, 8);
              
              if (ok)
                {
                  /*
                   * No source line number, but addresses,
                   * Like FIN, END, BCI, etc.
                   */
                  type = LINE_SRC;
                }
              else
                type = LINE_HEAD;
            }
        }
    }
  else
    type = LINE_BLANK;

  //printf("type = %d\n", type);

  switch(type)
    {
    case LINE_BLANK:
      break;
      
    case LINE_COMMENT:
      (void) find_field(LBL, src_line.size(), Annotation::A_COMMENT);
      break;
      
    case LINE_SRC:
      analyze_src();
      break;
      
    case LINE_HEAD:
      file->incr_pages();
      break;
      
    case LINE_SYMBOL:
      {
        int c = 4;
        Annotation *a;

        while (c < 72)
          {
            a = find_field(c, 6, Annotation::A_SYMBOL);

            if (a)
              {
                (void) find_field(c+7, 6, Annotation::A_ADDR);
                c += 17;
              }
            else
              c=100;
          }
      }
      break;
      
    case LINE_INFO:
      break;
      
    default:
      break;
    }
}

// }}}
// {{{ Annotation *Line::find_field(int pos, int len, Annotation::A annot)

Annotation *Line::find_field(int pos, int len, Annotation::A annot)
{
  int src_len = src_line.size();
  int i, f, l;
  Annotation *a=0;
  int max = pos+len;
  if (max > src_len)
    max = src_len;

  f = -1;
  l = -1;
  for (i=pos; i<max; i++)
    {
      if (!isspace(src_line[i]))
        {
          if (f < 0)
            f = i;
          l = i;
        }
    }
  if (f >= 0)
    {
      a = new Annotation(annot, f, l+1-f);
      pair<int, Annotation *>pp(f, a);
      annotations.insert(pp);
    }
  return a;
}

// }}}
// {{{ void Line::analyze_src()

void Line::analyze_src()
{
  Annotation *a;
  string opcode;
  Symbol *multiply_defined = 0;
  bool is_assembled = false;
  
  /*
   * Is there anything in the error field?
   */
 
  (void) find_field(0, SLN, Annotation::A_ERROR);

  /*
   * Is there an address field?
   */

  if (find_field(ADDR, ADDR_LEN, Annotation::A_ADDR)) {
    is_assembled = true;
  }
  
  /*
   * Is there object code?
   */
  
  if (find_field(OBJ, OBJ_LEN, Annotation::A_OBJ)) {
    is_assembled = true;
  }

  /* If there is no address field or opcode field then
   * one of two things is happening:
   * 1) It's a pseudo-op like EJCT, etc., or
   * 2) This is conditional assembly in which case neither
   *    the label or arguments should be processed.
   *
   * is_assembled controls this.
   */
  
  /*
   * Is there a label?
   */

  a = (is_assembled) ? find_field(LBL, LBL_LEN, Annotation::A_LABEL) : 0;

  if (a)
    {
      string name = src_line.substr(a->get_first(),
                                    a->get_len());
      
      Symbol *s = file->lookup_local(name);

      if (!s)
        {
          s = file->define(name);
          //printf("Label <%s>\n", name.c_str());
        }
      else
        {
          if (s->get_defined()) {
            file->redefine(s);
            multiply_defined = s;
          } else {
            file->define_symbol(s);
          }
        }
      a->set_s(s);
    }

  /*
   * Is there an opcode?
   */
  a=find_field(OPC, OPC_LEN, Annotation::A_OPC);
  if (a)
    {
      opcode = src_line.substr(a->get_first(),
                               a->get_len());
      
      int n = opcode.size()-1;
      if ((n>2) && (opcode[n] == '*')) // n>2 because "***" is a valid mnemonic
        {
          indirect = true;
          a->shorten(1);

          opcode = src_line.substr(a->get_first(),
                                    a->get_len());
        }

      if (opcode == "SET") {
        multiply_defined = 0; // OK with a SET
      }
      
      if (opcode == "END") {
        file->set_end_read();
      }

      int i=0;
      while ((instructions[i].mnemonic) && (!instr))
        {
          if (opcode == instructions[i].mnemonic)
            instr = &instructions[i];
          i++;
        }
    }

  if (multiply_defined) {
    fprintf(stderr, "Warning: Label <%s> already defined\n", multiply_defined->get_name()->c_str());
  }
  /*
   * Are there arguments ?
   */
  int i = ARG;
  int j = i+1;
  SF_TYPE type = SF_NORMAL;
  bool symbol, literal, asterix;
  int num;
  bool comma = false;
  string subr_args[2];
  int subr_arg_count = 0;

  j = (is_assembled) ? find_sub_field(i, type, symbol, literal, num, asterix) : i;

  while (j>i)
    {
      string arg = src_line.substr(i,j-i);
      //printf("found arg <%s> symbol=%d, literal=%d, num=0x%04x asterix=%d\n",
      //       arg.c_str(), symbol, literal, num, asterix);

      if (symbol)
        {
          Annotation::A a_type=Annotation::A_SYMBOL;

          if (opcode == "EXT")
            {
              /*
               * Add symbol to the local symbol-table
               * but mark it as undefined and external
               */
              
              
            }
          else if ((opcode == "SUBR") || (opcode == "ENT"))
            {
              if (subr_arg_count < 2)
                subr_args[subr_arg_count] = arg;

              if (subr_arg_count==0)
                a_type = Annotation::A_GSYMBOL;

              subr_arg_count++;
            }
          else if ((opcode == "CALL") || (opcode=="XAC"))
            a_type=Annotation::A_GSYMBOL;
          
          a=find_field(i, j-i, a_type);
        }
      else
        {
          if ((num == 1) && (arg.size()==1) && comma)
            indexed = true;
        }

      if ((opcode == "BCI") && (!symbol))
        {
          //printf("BCI!!!!!!!!!!!!!!!!!!!!!!!!\n");
          type = SF_BCI;
        }
      else
        type = SF_NORMAL;

      if ((j < ((int)src_line.size())) && (src_line[j] != ' '))
        {
          if ((src_line[j]=='+') || (src_line[j]=='-') ||
              (src_line[j]==',') || (src_line[j]==' ')) {
            if (src_line[j] == ',')
              comma = true;
            i = j+1;
          } else {
            i = j;
          }
          //printf("separator = <%c>\n", src_line[j]);
          j = find_sub_field(i, type, symbol, literal, num, asterix);
        }
      else
        i = j;
    }

  if (subr_arg_count>0)
    {
      /*
       * Deal with any arguments to SUBR/EXT
       *
       * Done here since need to know how many arguments
       * there were
       *
       * Build/find a symbol for the local name that
       * is the second (or only) argument.
       */
      Symbol *local = file->lookup_local(subr_args[subr_arg_count-1]);
      if (!local)
        local = file->declare_local(subr_args[subr_arg_count-1], false);
      
      /*
       * Build a global symbol pointing at that
       * local symbol
       */
      Symbol *global = file->lookup_global(subr_args[0]);
      if (!global)
        file->declare_global(subr_args[0], local);
      
      /*
       * If there is no local symbol with the
       * first argument's name then point it at the
       * second argument's local symbol since they
       * synonyms within the assembler
       */
      //Symbol *local2 = file->lookup_local(subr_args[0]);
      //if (!local2)
      //  local2 = file->local_synonym(subr_args[0], local);
    }

  if (j < ((int)src_line.size()))
    {
      /*
       * Looks like a comment
       */
      (void) find_field(j, src_line.size(), Annotation::A_COMMENT);
    }
}

// }}}
// {{{ void Line::render_char(FILE *fp, char c)

void Line::render_char(FILE *fp, char c)
{
  switch (c)
    {
      //case ' ': fprintf(fp, "&nbsp;"); break;
    case '&': fprintf(fp, "&amp;");  break;
    case '<': fprintf(fp, "&lt;");   break;
    case '>': fprintf(fp, "&gt;");   break;
      //case '_': fprintf(fp, "&larr;"); break;
      //case '^': fprintf(fp, "&uarr;"); break;
    default:  fprintf(fp, "%c", c);  break;
    }
}

// }}}
// {{{ void Line::render(FILE *fp, bool html_per_page)

void Line::render(FILE *fp, bool html_per_page, bool first)
{
  int i;
  int len = src_line.size();
  bool print_line=false;
  enum File::RENDER_STATE state=file->get_render_state();
  map<int, Annotation *>::iterator ap;
  Annotation *a;
  Annotation::A a_type = Annotation::A_SLN;
  int a_first=0;
  int a_len=0;

  //printf("type = %d\n", type);
  //printf("state = %d, type=%d\n", ((int)state), ((int)type));

  switch(type)
    {
    case LINE_BLANK:
      if (state == File::RS_AFTER_HEAD)
        state = File::RS_WAIT_SRC;
      else if (state == File::RS_SRC)
        state = File::RS_WAIT_HEAD;
      break;
      
    case LINE_COMMENT:
    case LINE_SRC:
    case LINE_SYMBOL:
    case LINE_INFO:
      print_line = true;
      if (state == File::RS_WAIT_SRC)
        state=File::RS_SRC;
      break;
            
    case LINE_HEAD:
      print_line = true;
      fprintf(fp, "%s", HEAD_TAG);
      if (state == File::RS_WAIT_HEAD)
        state=File::RS_AFTER_HEAD;
      break;
      
    default:
      break;
    }
  
  if (print_line)
    {
      ap = annotations.begin();
      if (ap != annotations.end())
        {
          a = ap->second;
          a_type = a->get_type();
          a_first = a->get_first();
          a_len = a->get_len();
        }
      else
        a = 0;

      i = 0;
      while (i<len)
        {
          if ((!a) || (i<a_first))
            {
              /*
               * No more annotation or
               * Before the start of the annotation
               */
              render_char(fp, src_line[i]);
              i++;
            }
          else
            {
              string s;
              Symbol *sym = 0;

              switch(a_type)
                {
                case Annotation::A_SLN:
                  fprintf(fp, "<span class=\"N\" id=\"S%d\">", src_line_num);
                  break;

                case Annotation::A_ERROR:
                  fprintf(fp, "<span class=\"E\">");
                  break;

                case Annotation::A_ADDR:
                  fprintf(fp, "<span class=\"A\">");
                  break;

                case Annotation::A_OBJ:
                  fprintf(fp, "<span class=\"O\">");
                  break;

                case Annotation::A_LABEL:
                  if (first) {
                    a->get_s()->md_step();
                  }
                  fprintf(fp, "<span class=\"L\" id=\"L%d\">",
                          a->get_s()->get_serial_number());
                  break;

                case Annotation::A_OPC:
                  if (instr)
                    {
                      string descr = instr->description;
                      if (instr->type == MR)
                        {
                          if (indexed)
                            descr = descr + ", indexed";
                          if (indirect)
                            descr = descr + ", indirect";
                        }
                      fprintf(fp, "<span class=\"I\" title=\"%s\">", descr.c_str());
                    }
                  else
                    fprintf(fp, "<span class=\"I\">");
                  break;

                case Annotation::A_COMMENT:
                  fprintf(fp, "<span class=\"C\">");
                  break;

                case Annotation::A_SYMBOL:
                case Annotation::A_GSYMBOL:
                  s = src_line.substr(a_first,a_len);

                  if (a_type == Annotation::A_GSYMBOL)
                    sym = file->lookup_global(s);
                  else
                    sym = file->lookup(s);
                  
                  if ((sym) && (sym->get_defined())) {
                    fprintf(fp, "<span class=\"S\"><a href=\"%s\">", render_link(sym, html_per_page).c_str());
                  } else {
                    fprintf(fp, "<span class=\"U\">");
                    fprintf(stderr, "Warning: Undefined %s symbol <%s>\n",
                            (a_type == Annotation::A_GSYMBOL) ? "global" : "", s.c_str());
                  }
                  break;
                  
                default:
                  abort();
                }
              
              while (i<(a_first+a_len))
                {
                  render_char(fp, src_line[i]);
                  i++;
                }

              switch(a_type)
                {
                case Annotation::A_SLN:
                case Annotation::A_ERROR:
                case Annotation::A_ADDR:
                case Annotation::A_OBJ:
                case Annotation::A_LABEL:
                case Annotation::A_OPC:
                case Annotation::A_COMMENT:
                  fprintf(fp, "</span>");
                  break;
                case Annotation::A_SYMBOL:
                case Annotation::A_GSYMBOL:
                  fprintf(fp, "%s</span>", (sym) ? "</a>":"");
                  break;
                default:
                  abort();
                }

              ap++;
              if (ap != annotations.end())
                {
                  a = ap->second;
                  a_type = a->get_type();
                  a_first = a->get_first();
                  a_len = a->get_len();
                }
              else
                a = 0;    
            }
          
        }
    
      if (type == LINE_HEAD)
        fprintf(fp, "%s", HEAD_TAG_END);
        
      render_char(fp, '\n');
    }

  //printf("state = %d\n", state);
  file->set_render_state(state);
}

// }}}

// {{{ string Line::render_link(Symbol *s, bool html_per_page)

string Line::render_link(Symbol *s, bool html_per_page)
{
  string r="";
  ostringstream n;
  File *target = s->get_file();

  if ((target != file) ||
      ((html_per_page) && (s->get_page() != page)))
    r=target->get_html_name( (html_per_page) ? s->get_page() : 0);
  
  r = r + "#" + "L";
  
  n << s->get_serial_number();
  r = r + n.str();

  //
  // Isn't this returning a local on the stack?
  // looks dangerous...
  //
  return r;
}

// }}}

// {{{ int Line::find_sub_field(int first, SF_TYPE type,

int Line::find_sub_field(int first, SF_TYPE type,
                         bool &symbol, bool &literal, int &num, bool &asterix)
{
  int len = src_line.size();
  int i;
  int c;
  bool done = false;
  int count = 0;
  bool minus = false;

  enum STATE
  {
    ST_FIRST,
    ST_ASTERIX,
    ST_DOLLAR,
    ST_APOSTROPHE,
    ST_EQUALS,
    ST_OTHER,
    ST_HEX,
    ST_OCTAL,
    ST_DECIMAL,
    ST_ISO,
    ST_DIGIT
  } state = ST_FIRST;
  
  symbol = false;
  literal = false;
  asterix = false;

  if (type == SF_BCI)
    {
      i = first + 2*num;

      if (i < len)
        return i;
      else
        return len;
    }

  num = 0;

  i = first;
  while ((i < len) && (!done))
    {
      c = src_line[i];

      switch (state)
        {
        case ST_FIRST:
          switch (c)
            {
            case '*':
              state = ST_ASTERIX;
              asterix=true;
              break;
            case '$': state = ST_DOLLAR; break;
            case '\'': state = ST_APOSTROPHE; break;
            case '-':
              state = ST_DECIMAL;
              minus = true;
              break;
            case '=':
              state = ST_EQUALS;
              literal = true;
              break;
            case ' ': done = true; break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              state = ST_DIGIT;
              num = c-'0';
              break;
            default: state = ST_OTHER;
              symbol = true;
              break;
            }
          break;

        case ST_EQUALS:
          switch (c)
            {
            case '*': state = ST_ASTERIX; break;
            case '$': state = ST_DOLLAR; break;
            case '\'': state = ST_APOSTROPHE; break;
            case 'A':
              state = ST_ISO;
              count = 0;
              break;
            case '-':
              state = ST_DECIMAL;
              minus = true;
              break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              state = ST_DECIMAL;
              num = c-'0';
              break;
            default: done=true; break;
            }
          break;

        case ST_ASTERIX:
          if (c != '*')
            done = true;
          break;

        case ST_DOLLAR:
          /* Hex numbers */
          state = ST_HEX;
          if (c == '-')
            {
              minus = true;
              break;
            }
        case ST_HEX:
          if ((c >= '0') && (c <= '9'))
            num = (num * 16) + c-'0';
          else if ((c >= 'a') && (c <= 'f'))
            num = (num * 16) + 10+c-'a';
          else if ((c >= 'A') && (c <= 'F'))
            num = (num * 16) + 10+c-'A';
          else
            done = true;
          break;

        case ST_APOSTROPHE:
          /* Octal numbers */
          state = ST_OCTAL;
          if (c == '-')
            {
              minus = true;
              break;
            }
        case ST_OCTAL:
          if ((c >= '0') && (c <= '7'))
            num = (num * 8) + c-'0';
          else
            done = true;
          break;

        case ST_DECIMAL:
          if ((c >= '0') && (c <= '9'))
            num = (num * 10) + c-'0';
          else
            done = true;
          break;

        case ST_DIGIT:
          /* Don't know if this is a symbol with
           * a leading digit, or a number (yet)
           */
          if ((c >= '0') && (c <= '9'))
            num = (num * 10) + c-'0';
          else if ((c=='+') || (c=='-') || (c==',') || (c==' '))
            done = true;
          else
            {
              state = ST_OTHER;
              symbol = true;
            }
          break;

        case ST_ISO:
          count ++;
          if (count > 2)
            done = true;
          else
            num = (num << 8) | (c & 127) | 128;
          break;

        case ST_OTHER:
          if ((c=='+') || (c=='-') || (c==',') || (c==' '))
            done = true;
          break;

        default:
          abort();
        }

      if (!done)
        i++;
    }

  if ((!symbol) && (minus))
    num = -num;

  return i;
}

// }}}

map<string, Symbol *> File::global_symbol_table;
// {{{ File::File(const char *filename, bool is_toc)

File::File(const char *filename, bool is_toc)
  : filename(filename),
    end_really_read(false),
    pages(0),
    is_toc(is_toc),
    use_comment_as_title(false),
    end_read(false),
    use_supplied_title(false)
{
  FILE *fp;
  string str;
  Line *line;

  if (!is_toc)
    {
      fp = fopen(filename, "r");
      
      if (!fp)
        {
          cerr << "Could not open <" << filename << "> for reading" << endl;
          exit(1);
        }
      
      bool ok = true;
      while (ok)
        {
          str = read_line(fp, ok);
          line = new Line(str, this, pages);
          lines.push_back(line);
          line->analyze();
        }
      
      fclose(fp);
    }

  render_state = RS_WAIT_HEAD;
}

// }}}
// {{{ string File::basename(string filename)

string File::basename(string filename)
{
  int i;
  size_t j;
  int len = filename.size();

  j = string::npos;
  i = len-1;
  while (i>=0)
    {
      if (filename[i] == '.')
        {
          j = i;
          break;
        }
      i--;
    }
  return filename.substr(0, j);
}

// }}}
// {{{ string File::get_html_name(int page)

string File::get_html_name(int page)
{
  string r = basename(filename);
  ostringstream n;
  if (page > 0)
    {
      n << page;
      r = r + "_" + n.str();
    }
  r = r + ".html";
  return r;
}

// }}}

// {{{ string File::find_first_file_name(bool html_per_page, string &title_str)

string File::find_first_file_name(bool html_per_page, string &title_str)
{
  title_str = files[0]->title;
  return files[0]->get_html_name((html_per_page && (!files[0]->is_toc)) ? 1 : 0);
}

// }}}
// {{{ string File::find_last_file_name(bool html_per_page, string &title_str)

string File::find_last_file_name(bool html_per_page, string &title_str)
{
  int i;
  i = 0;
  while (files[i])
    i++;
  i-=1;

  title_str = files[i]->title;
  return files[i]->get_html_name((html_per_page) ? (files[i]->pages) : 0);
}

// }}}
// {{{ string File::find_file_name(bool previous, bool html_per_page, int page, string &title_str)

string File::find_file_name(bool previous, bool html_per_page, int page, string &title_str)
{
  int i, j, p;
  i = 0;
  j = -1;
  string r;
  File *f=0;

  /*
   * If html file per page then
   * may only need to change the page
   * number
   */
  if ((html_per_page) && (page > 0))
    {
      title_str = title;
      if (previous)
        {
          p = page - 1;
          if (p > 0)
            return get_html_name(p);
        }
      else
        {
          p = page + 1;
          if (p <= pages)
            return get_html_name(p);
        }
    }
  
  while ((!f) && ((j<0) || (files[j])))
    {
      if (previous)
        {
          if (files[i] == this)
            {
              if (j < 0)
                f = 0;
              else
                f = files[j];
            }
        }
      else
        {
          if ((j>=0) && (files[j] == this))
            {
              if (i)
                f = files[i];
              else
                f = 0;
            }
        }
      j = i;
      i++;
    }

  if (!f)
    {
      title_str = "";
      return "";
    }

  title_str = f->title;

  if (html_per_page && (!f->is_toc))
    {
      if (page > 0)
        {
          if (previous)
            return f->get_html_name(f->pages);
          else
            return f->get_html_name(1);
        }
      else
        return f->get_html_name(1);
    }
  else
    return f->get_html_name(0);
}

// }}}

// {{{ void File::link_button(FILE *fp, string filename, char *str, const string &title_str)

void File::link_button(FILE *fp, string filename, const char *str, const string &title_str)
{
  if (filename.size()>0)
    fprintf(fp, "%s <a href=\"%s\" title=\"%s\">%s</a> %s\n",
            BUTTON_START, filename.c_str(), title_str.c_str(), str, BUTTON_STOP);
  else
    fprintf(fp, "%s %s %s\n", BUTTON_START, str, BUTTON_STOP);
}

// }}}
// {{{ void File::links(FILE *fp, bool html_per_page, int page)

void File::links(FILE *fp, bool html_per_page, int page)
{
  string l_filename;
  string title_str;

  l_filename = find_file_name(true, html_per_page, page, title_str);
  if (l_filename.size()>0)
    fprintf(fp, "%s rel=\"prev\" href=\"%s\" title=\"%s\" %s\n",
            LINK_START, l_filename.c_str(), title_str.c_str(), LINK_STOP);

  l_filename = find_file_name(false, html_per_page, page, title_str);
  if (l_filename.size()>0)
    fprintf(fp, "%s rel=\"next\" href=\"%s\" title=\"%s\" %s\n",
            LINK_START, l_filename.c_str(), title_str.c_str(), LINK_STOP);

  l_filename = find_first_file_name(html_per_page, title_str);
  if (l_filename.size()>0)
    fprintf(fp, "%s rel=\"first\" href=\"%s\" title=\"%s\" %s\n",
            LINK_START, l_filename.c_str(), title_str.c_str(), LINK_STOP);

  l_filename = find_last_file_name(html_per_page, title_str);
  if (l_filename.size()>0)
    fprintf(fp, "%s rel=\"last\" href=\"%s\" title=\"%s\" %s\n",
            LINK_START, l_filename.c_str(), title_str.c_str(), LINK_STOP);

  l_filename = find_first_file_name(html_per_page, title_str);
  if ((files[0]->is_toc) && (l_filename.size()>0))
    fprintf(fp, "%s rel=\"contents\" href=\"%s\" title=\"%s\" %s\n",
            LINK_START, l_filename.c_str(), title_str.c_str(), LINK_STOP);

  int i;
  i = 0;
  while (files[i])
    {
      l_filename = files[i]->get_html_name((html_per_page) ? 1 : 0);
      title_str = files[i]->title;
      
      if ((l_filename.size()>0) && ((i>0) || (!files[i]->is_toc)))
        fprintf(fp, "%s rel=\"chapter\" href=\"%s\" title=\"%s\" %s\n",
                LINK_START, l_filename.c_str(), title_str.c_str(), LINK_STOP);
      
      i++;
    }

}

// }}}
// {{{ void File::link_buttons(FILE *fp, bool html_per_page, int page)

void File::link_buttons(FILE *fp, bool html_per_page, int page)
{
  string title_str;

  fprintf(fp, "%s\n", TABLE_START);

  if (n_files > 1)
    link_button(fp, find_file_name(true, html_per_page, 0, title_str), PREVIOUS_FILE, title_str);
  else
    fprintf(fp, "%s\n", BUTTON_EMPTY);

  if (html_per_page)
    link_button(fp, find_file_name(true, html_per_page, page, title_str), PREVIOUS_PAGE, title_str);
  else
    fprintf(fp, "%s\n", BUTTON_EMPTY);
  
  if (html_per_page)
    link_button(fp, get_html_name(0), SINGLE_PAGE, title);
  else
    link_button(fp, get_html_name(1), MULTIPLE_PAGES, title);

  if (html_per_page)
    link_button(fp, find_file_name(false, html_per_page, page, title_str), NEXT_PAGE, title_str);
  else
    fprintf(fp, "%s\n", BUTTON_EMPTY);
  
  if (n_files > 1)
    link_button(fp, find_file_name(false, html_per_page, 0, title_str), NEXT_FILE, title_str);
  else
    fprintf(fp, "%s\n", BUTTON_EMPTY);

  fprintf(fp, "%s\n", TABLE_STOP);
}

// }}}

// {{{ void File::file_title()

void File::file_title()
{
  char *c_comment;
  string comment;

  list<Line *>::iterator i = lines.begin();
  
  while ( (i!=lines.end()) && (*i) &&
          ( ((*i)->get_type() == Line::LINE_BLANK) ||
            ((*i)->get_type() == Line::LINE_HEAD))) {
    //std::cout << (*i)->get_src_line() << std::endl;
    i++;
  }
  
  Line *lp = (*i);
  
  if ((lp) && (lp->get_type() == Line::LINE_COMMENT))
    {
      string s = lp->get_src_line();
      int j = LBL;
      if (s[j]=='*') j++;
      while (s[j]==' ') j++;
      c_comment = get_substr(s, j, s.size());
      comment = (c_comment)?c_comment:"";
      delete [] c_comment;
    }

  if (use_supplied_title)
    {
      title = supplied_title;
    }
  else if ((heading_line.size() > 0) && (!use_comment_as_title))
    {
      if (comment.size() > 0)
        {
          string s = get_substr(comment, 0, heading_line.size());
          if (s == heading_line)
            title = comment;
          else
            title = heading_line;
        }
      else
        title = heading_line;
    }
  else
    {
      if (comment.size() > 0)
        title = comment;
      else
        title = "DEFAULT_HEADING";
    }

}

// }}}
// {{{ void File::quote_dollars(const string &in_str, string &out_str)

void File::quote_dollars(const string &in_str, string &out_str)
{
  string::const_iterator i;
  out_str.clear();
  for (i=in_str.begin(); i!=in_str.end(); i++) {
    if ((*i) == '$') {
      out_str.append(left_quote);
      out_str.append(1, (*i));
      out_str.append(right_quote);
    } else {
      out_str.append(1, (*i));
    }
  }
}

// }}}

// {{{ void File::file_start(FILE *fp, bool html_per_page, int page)

void File::file_start(FILE *fp, bool html_per_page, int page)
{
  time_t t;
  char buf[100];
  string dollars_quoted;
  quote_dollars(title, dollars_quoted);

  // TODO - quote dollar signs
  fprintf(fp, "m4_define(%s%s%s, %s%s%s)\n",
          left_quote, HTML_HEADING,  right_quote,
          left_quote, dollars_quoted.c_str(), right_quote);
  
  (void) time(&t);
  ctime_r(&t, buf);
  if ((*buf) && (buf[strlen(buf)-1]=='\n'))
    buf[strlen(buf)-1]='\0';

  fprintf(fp, "m4_define(%s%s%s, %s%s%s)\n",
          left_quote, HTML_TIME, right_quote,
          left_quote, buf,       right_quote);

  fprintf(fp, "%s\n", HEAD1);
  links(fp, html_per_page, page);
  fprintf(fp, "%s\n", HEAD2);
  if (!is_toc)
    {
      link_buttons(fp, html_per_page, page);
      fprintf(fp, "%s\n", HEAD3);
    }
}

// }}}
// {{{ void File::file_stop(FILE *fp, bool html_per_page, int page)

void File::file_stop(FILE *fp, bool html_per_page, int page)
{
  if (!is_toc)
    {    
      fprintf(fp, "%s\n", TAIL1);
      link_buttons(fp, html_per_page, page);
    }
  fprintf(fp, "%s\n", TAIL2);
}

// }}}

// {{{ void File::render()

void File::render()
{
  string out_filename = basename(filename)+".m4h";
  string out_filename2;
  char buf[12];

  FILE *fp = fopen(out_filename.c_str(), "w");
  FILE *fp2 = 0;
  int page = 0;

  if (!fp)
    {
      fprintf(stderr, "Could not open <%s> for writing\n",
              out_filename.c_str());
      exit(1);
    }

  // Heading

  file_start(fp, false, page);

  if (is_toc)
    {
      fprintf(fp, "%s\n", TOC_TABLE_START);

      int i;
      for (i=1; i<n_files; i++)
        {
          string single_filename = files[i]->get_html_name(0);
          string multi_filename = files[i]->get_html_name(1);
          string title_str = files[i]->title;

          fprintf(fp, "%s", TOC_ROW_START);
          fprintf(fp, "<td>%s</td>", title_str.c_str());
          fprintf(fp, "<td><a href=\"%s\">%s</a></td>", single_filename.c_str(), SINGLE_PAGE);
          fprintf(fp, "<td><a href=\"%s\">%s</a></td>", multi_filename.c_str(), MULTIPLE_PAGES);
          fprintf(fp, "%s\n", TOC_ROW_STOP);
        }

      fprintf(fp, "%s\n", TOC_TABLE_STOP);
    }
  else
    {
      list<Line *>::iterator i = lines.begin();
      
      while (i!=lines.end())
        {
          Line::LINE_TYPE type = (*i)->get_type();
          
          if (type == Line::LINE_HEAD)
            {
              if (fp2)
                {
                  file_stop(fp2, true, page);
                  fclose(fp2);
                }
              sprintf(buf, "%d", ++page);
              
              out_filename2 = basename(filename)+"_"+buf+".m4h";
              fp2 = fopen(out_filename2.c_str(), "w");
              
              if (!fp2)
                {
                  fprintf(stderr, "Could not open <%s> for writing\n",
                          out_filename2.c_str());
                  exit(1);
                }
              file_start(fp2, true, page);
            }
          
          (*i)->render(fp, false, true);
          if (fp2)
            (*i)->render(fp2, true, false);
          
          i++;
        }
    }
  
  if (fp2)
    {
      file_stop(fp2, true, page);
      fclose(fp2);
    }
  
  file_stop(fp, false, page);
  fclose(fp);
}

// }}}
// {{{ string File::read_line(FILE *fp, bool &ok)

string File::read_line(FILE *fp, bool &ok)
{
  string res;
  int c = getc(fp);

  if (c == EOF)
    ok = false;
  else
    while ((c != '\n') && (c != EOF))
      {
        if (c != '\f') /* ignore form feeds */
          res += c;
        c = getc(fp);
      }
  return res;
}

// }}}
// {{{ Symbol *File::lookup_local(string name)

Symbol *File::lookup_local(string name)
{
  int len = name.size();
  string str = name.substr(0, (len>4)?4:len);

  map<string, Symbol *>::iterator p;

  p = symbol_table.find(str);
  if (p != symbol_table.end())
    return p->second;
  else
    return 0;
}

// }}}
// {{{ Symbol *File::lookup_global(string name)

Symbol *File::lookup_global(string name)
{
  int len = name.size();
  string str = name.substr(0, (len>6)?6:len);

  map<string, Symbol *>::iterator p;

  p = global_symbol_table.find(str);
  if (p != global_symbol_table.end())
    return p->second;
  else
    return 0;
}

// }}}
// {{{ Symbol *File::lookup(string name)

Symbol *File::lookup(string name)
{
  Symbol *s = lookup_local(name);
  if ((!s) || (s->get_ext()))
    s = lookup_global(name);
  return s;
}

// }}}

// {{{ Symbol *File::define(string name)

Symbol *File::define(string name)
{
  int len = name.size();
  string str = name.substr(0, (len>4)?4:len);
  Symbol *s = new Symbol(name, pages, this);
  
  pair<string, Symbol *> pp(str, s);
  symbol_table.insert(pp);

  //printf("Defined <%s> on page %d\n", name.c_str(), pages);
  return s;
}

// }}}

void File::redefine(Symbol *s)
{
  s->redefine(pages);
}

// {{{ void File::define_symbol(Symbol *s)

void File::define_symbol(Symbol *s)
{
  s->set_page(pages);
  s->set_defined(true);
}

// }}}

// {{{ Symbol *File::declare_local(string name, bool ext)

Symbol *File::declare_local(string name, bool ext)
{
  int len = name.size();
  string str = name.substr(0, (len>4)?4:len);
  Symbol *s = lookup_local(name);

  if (!s)
    {
      s = new Symbol(name, pages, this, ext, false);
      
      pair<string, Symbol *> pp(str, s);
      symbol_table.insert(pp);
      
      //printf("Declared %s local <%s> on page %d\n", (ext) ? "ext" : "", name.c_str(), pages);
    }
  return s;
}

// }}}
// {{{ Symbol *File::declare_global(string name, Symbol *local)

Symbol *File::declare_global(string name, Symbol *local)
{
  int len = name.size();
  string str = name.substr(0, (len>6)?6:len);
  Symbol *s = lookup_global(name);

  if (!s)
    {
      pair<string, Symbol *> pp(str, local);
      global_symbol_table.insert(pp);
      
      //printf("Declared global <%s>\n", name.c_str());

      if (heading_line.size() > 0)
        heading_line += ", ";
      heading_line += name;
    }
  return local;
}

// }}}
// {{{ Symbol *File::local_synonym(string name, Symbol *local)

Symbol *File::local_synonym(string name, Symbol *local)
{
  int len = name.size();
  string str = name.substr(0, (len>4)?4:len);
  Symbol *s = lookup_local(name);

  if (!s)
    {
      pair<string, Symbol *> pp(str, local);
      symbol_table.insert(pp);
      
      //printf("Declared local synonym <%s>\n", name.c_str());
    }
  return local;
}

// }}}

// {{{ void File::set_use_comment_as_title(bool tc)

void File::set_use_comment_as_title(bool tc)
{
  use_comment_as_title = tc;
}

// }}}
// {{{ void File::supply_title(string title)

void File::supply_title(string title)
{
  use_supplied_title = true;
  supplied_title = title;
}

// }}}

int Symbol::next_serial_number = 0;
// {{{ Symbol::Symbol(string name, int page, bool ext)

Symbol::Symbol(string name, int page, File *file, bool ext, bool defined)
  : name(name),
    page(page),
    file(file),
    ext(ext),
    defined(defined),
    multiply_defined(false),
    md_iterating(false)
{
  serial_number = next_serial_number++;
}

// }}}

void Symbol::redefine(int new_page)
{
  md_list.push_back(make_pair(page, serial_number));
  multiply_defined = true;

  page = new_page;
  serial_number = next_serial_number++;
}

int Symbol::get_page()
{
  return (multiply_defined && md_iterating) ? md_itrt->first : page;
}

int Symbol::get_serial_number()
{
  return (multiply_defined && md_iterating) ? md_itrt->second : serial_number;
}

void Symbol::md_step()
{
  /* Called *before* a symbol is used as a label for the first time
   * on a given source line */
  if (multiply_defined) {
    if (md_iterating) {
      if (md_itrt == md_list.end()) {
        fprintf(stderr, "Multiply defined iterator for <%s> exceeded\n", name.c_str());
        abort();
      }
      ++md_itrt;
    } else {
      // Push the last definition onto the list
      md_list.push_back(make_pair(page, serial_number));
      md_itrt = md_list.begin();
      md_iterating = true;
    }
  }
  // If not multuply defined, silently ignore
}

// {{{ int main (int argc, char **argv)

int main (int argc, char **argv)
{
  int i, a;

  bool tc = false;
  char *title = 0;

  strcpy(left_quote, DEFAULT_LEFT_QUOTE);
  strcpy(right_quote, DEFAULT_RIGHT_QUOTE);

  files = new File *[argc+1]; // Can't be more files than that
  for (i=0; i<(argc+1); i++)
    files[i] = 0;
  n_files = 0;

  for (a=1; a<argc; a++)
    {
      if (argv[a][0] == '-')
        {
          if ((strncmp(argv[a], "-h", 2) == 0) || (strncmp(argv[a], "--h", 3) == 0))
            {
              cerr << "Usage : " << argv[0] << " [-h|--help] [-q<l>,<r>] [[-t <title>] -i <TOC-name>] ([-tc|-t <title>] <filename>)*" << endl;
            }
          else if (strncmp(argv[a], "-q", 2) == 0)
            {
              char *p, *l, *r;
              p = &argv[a][2];
              l = r = 0;

              while ((*p) && ((*p)!=','))
                p++;

              if (*p)
                {
                  // Have found a ','
                  if (p > l)
                    l = &argv[a][2];
                  *p = '\0'; // Terminate left quote
                  r = p+1; // right starts in next character
                  if (!(*r))
                    r = 0;
                }

              if ((strlen(l)>MAX_LEN_QUOTE) || (strlen(r)>MAX_LEN_QUOTE))
                {
                  cerr << "Quote chars <" << ((l)?l:"[none]") << ">, <"
                       << ((r)?r:"[none]") << "> are too long. (Max: "
                       << MAX_LEN_QUOTE << "characters)" << endl;
                  exit(1);
                }

              strcpy(left_quote,  ((l) ? l : DEFAULT_LEFT_QUOTE ));
              strcpy(right_quote, ((r) ? r : DEFAULT_RIGHT_QUOTE));
            }
          else if (strcmp(argv[a], "-t") == 0)
            {
              tc = false;
              a++;
              title = argv[a];
            }
          else if (strcmp(argv[a], "-tc") == 0)
            {
              tc = true;
              title = 0;
            }
          else if (strcmp(argv[a], "-i") == 0)
            {
              if (n_files > 0)
                {
                  cerr << "-i must occur before other filenames" << endl;
                  exit(1);
                }
              a++;

              files[n_files] = new File(argv[a], true);

              if (title)
                files[n_files]->supply_title(title);
              else
                files[n_files]->supply_title(DEFAULT_TOC_TITLE);

              tc = false;
              title = 0;

              n_files++;
            }
        }
      else
        {
          files[n_files] = new File(argv[a]);
          if (tc)
            files[n_files]->set_use_comment_as_title(tc);
          else if (title)
            files[n_files]->supply_title(title);

          title = 0;
          tc = false;

          n_files++;
        }
    }

  for (i=0; i<n_files; i++)
    files[i]->file_title();

  for (i=0; i<n_files; i++)
    files[i]->render();

  exit(0);
}

// }}}
