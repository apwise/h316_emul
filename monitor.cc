/* Honeywell Series 16 emulator
 * Copyright (C) 1998, 2004, 2005  Adrian Wise
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
 * Command line monitor for H316 emulator
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

#include "proc.hh"
#include "stdtty.hh"
#include "gpl.h"
#include "version.h"
#include "monitor.hh"

#define BUFSIZE 1024
#define PROMPT "MON"

static const char *instructions_text[] = 
  {
    "Type \"license\" or \"warranty\" for details\n",
    NULL
  };

struct CmdTab
{
  const char *name;
  int while_running; // 0=No, 1=Yes, 2=Yes if no arguments
  int min_args;
  int max_args;
  const char *descr;

  bool (Monitor::*cmd)(bool &run, int argc, char **argv);
};

struct CmdTab Monitor::commands[] =
  {
    {"a",          2, 0, 1, "[data] : Get/Set A register",                  &Monitor::a},
    {"b",          2, 0, 1, "[data] : Get/Set B register",                  &Monitor::b},
    {"x",          2, 0, 1, "[data] : Get/Set X register",                  &Monitor::x},
    {"m",          1, 1, 2, "addr [data] : Get/Set memory",                 &Monitor::m},
    {"quit",       1, 0, 0, "Quit emulation",                               &Monitor::quit},
    {"continue",   1, 0, 0, "Continue",                                     &Monitor::cont},
    {"stop",       1, 0, 0, "Stop",                                         &Monitor::cont},
    {"go",         0, 0, 1, "[addr] Start execution",                       &Monitor::go},
    {"ss",         1, 1, 2, "num [0/1] Get/Set Sense Switch",               &Monitor::ss},
    {"ptr",        1, 1, 1, "filename : Set Papertape Reader filename",     &Monitor::ptr},
    {"ptp",        1, 1, 1, "filename : Set Papertape Punch filename",      &Monitor::ptp},
    {"plt",        1, 1, 1, "filename : Set Plotter filename",              &Monitor::plt},
    {"lpt",        1, 1, 1, "filename : Set Lineprinter filename",          &Monitor::lpt},
    {"asr_ptr",    1, 1, 1, "filename : Set ASR Papertape Reader filename", &Monitor::asr_ptr},
    {"asr_ptr_on", 1, 0, 1, "[filename] : Turn on ASR Papertape Reader",    &Monitor::asr_ptr_on},
    {"asr_ptp",    1, 1, 1, "filename : Set ASR Papertape Punch filename",  &Monitor::asr_ptp},
    {"asr_ptp_on", 1, 0, 1, "[filename] : Turn on ASR Papertape Punch",     &Monitor::asr_ptp_on},
    {"clear",      1, 0, 0, "Master clear",                                 &Monitor::clear},
    {"help",       1, 0, 0, "Print this help",                              &Monitor::help},
    {"trace",      1, 0, 2, "[filename] [,lines] : Save trace file",        &Monitor::trace},
    {"disassemble",1, 1, 3, "[filename] first [,last] : Save disassembly",  &Monitor::disassemble},
    {"vmem",       1, 1, 2, "filename [, exec-addr] : Save Verilog Mem.",   &Monitor::vmem},
    {"omem",       1, 1, 2, "filename [, exec-addr] : Save Octal Mem.",     &Monitor::omem},
    {"coemem",     1, 1, 2, "filename [, exec-addr] : Save Xilinx .coe",    &Monitor::coemem},
    {"license",    1, 0, 0, "Print license information",                    &Monitor::license},
    {"warranty",   1, 0, 0, "Statement of no warranty",                     &Monitor::warranty},
    {NULL,         0, 0, 0, NULL, NULL}
  };

static Monitor *monitor = 0;

static void sig_handler(int signo)
{
  if (monitor) {
    monitor->sig_handler(signo);
  }
}

void Monitor::sig_handler(int signo)
{
  if (signo == SIGINT) {
    p->goto_monitor();
  }
}

Monitor::Monitor(Proc *p, STDTTY *stdtty, int argc, char **argv)
{
  this->p = p;
  this->stdtty = stdtty;
  this->argc = argc;
  this->argv = argv;

  first_time = 1;

  if (!monitor) {
    struct sigaction sa;

    bzero(&sa, sizeof(struct sigaction));
           
    sa.sa_handler = &::sig_handler;
    
    if (sigaction(SIGINT, &sa, 0)) {
      abort();
    }
    monitor = this;
  }

}

void Monitor::get_line(char *prompt, FILE **fp, char *buffer, int buf_size)
{
  char *p;
  int i;

  if (*fp)
    {
      p = fgets(buffer, buf_size, *fp);

      // Nibble off trailing newline and space
      i = strlen(buffer)-1;
      while (i && ((buffer[i]=='\n') || isspace(buffer[i])))
        buffer[i--] = '\0';

      if (p)
        printf("%s%s\n", prompt, buffer);
      else
        {
          fclose(*fp);
          *fp = NULL;
        }
    }

  if (!*fp)
    stdtty->get_input(prompt, buffer, buf_size, 1);

  /*
    p=buffer;
  while (*p)
    {
      if ((*p >= 'A') && (*p <= 'Z'))
        *p += ('a'-'A');
      p++;
    }
  */
}

void Monitor::do_commands(bool &run, FILE **fp)
{
  char buffer[BUFSIZE];
  char **cmd;
  int words;
  char prompt[20];
  
  sprintf(prompt, "%s> ", PROMPT);
  
  if (!run)
    p->flush_events();
  
  if ((*fp))
    {
      printf("\n");
    }
  else
    { // don't print if reading from file
      if (first_time)
        {
          int i = 0;
          while (copyright_text[i])
            printf("%s", copyright_text[i++]);
          i = 0;
          while (instructions_text[i])
            printf("%s", instructions_text[i++]);
        }
      first_time = 0;
      printf("\n%s: A:%06o B:%06o X:%06o %s\n", PROMPT,
             (p->get_a() & 0xffff), (p->get_b() & 0xffff),
             (p->get_x() & 0xffff),
             p->dis());
    }
  
  doing_commands = 1;
  
  while (doing_commands)
    {
      get_line(prompt, fp, buffer, BUFSIZE);
      
      cmd = break_command(buffer, words);
      do_command(run, words, cmd);
      free_command(cmd);
    }
  
  fflush(stdout);
  stdtty->set_non_cannonical();
}

char **Monitor::break_command(char *buf, int &words)
{
  char *p[128];
  int i, j, k;
  char temp_c;
  char **q;

  i = k = 0;

  //printf("Monitor::break_command(): <%s>\n", buf);

  while (buf[i])
    {
      while (isspace(buf[i])) i++; // skip over space
      j = i;
      
      while ( (buf[i] != ' ') && (buf[i] != '\n')
              && (buf[i] != '\0') && (buf[i] != ',') &&
              (!((isdigit(buf[i]) || (buf[i]=='\'')) && (k==0)))  )
        i++;
      //
      // Note special case above, separator can be
      // omited after command if argument is numerical.
      //
      
      if (k>=128)
        {
          fprintf(stderr, "Too many words\n");
          exit(1);
        }
      temp_c = buf[i];
      buf[i] = '\0';
      
      //printf("p[%d] = %s\n", k, &buf[j]);
      
      p[k++] = strdup(&buf[j]);
      buf[i] = temp_c;
  
      if ((buf[i] == ' ') || (buf[i] == ','))
        i++;
    }
  
  q = new char *[k+1];
  for (i=0; i<k; i++)
    q[i] = p[i];
  q[k] = (char *)NULL;

  words = k;

  return q;
}

void Monitor::free_command(char **q)
{
  int i=0;
  while (q[i])
    delete q[i++];
  delete q;
}

void Monitor::do_command(bool &run, int words, char **cmd)
{
  int i;
  int best_match=-1, matches;
  unsigned int len;
  //void (Monitor::*routine)(bool &run, int argc, char **argv);

  if (words <= 0) return;
  if (cmd[0][0] == '#') return; // comment

  i = matches = 0;

  //printf("Monitor::do_command(): %s words=%d\n", cmd[0], words);

  while (commands[i].name)
    {
      len = strlen(cmd[0]);
      if (strncmp(cmd[0], commands[i].name, len)==0)
        {
          if (len == strlen(commands[i].name))
            {
              // This is using all of the characters
              // of the command name from the table
              // so this wins over entries in the
              // table that start with this sequence

              best_match = i;
              matches = 1;
              break; // Don't look at later table entries
            }
          else
            {
              best_match = i;
              matches++;
            }
        }
      i++;
    }

  //printf("matches = %d\n", matches);

  if (run && (!((commands[best_match].while_running == 1) ||
                ((commands[best_match].while_running == 2) &&
                 (words <= 1)) )) )
    {
      printf("! %s can only be used when halted\n",
             commands[best_match].name);
    }
  else if (matches == 1)
    {
      if (((words-1) >= commands[best_match].min_args) &&
          ((words-1) <= commands[best_match].max_args))
        {
          if (! (this->*commands[best_match].cmd)(run, words, cmd))
            printf("??\n");
        }
      else
        printf("Expected %d to %d arguments\n",
               commands[best_match].min_args,
               commands[best_match].max_args);
    }
  else
    printf("??\n");
}

long Monitor::parse_number(char *str, bool &ok)
{
  char *ptr;
  int base=0;
  long n;

  if (str[0] == '\'')
    {
      str++;
      base = 8;
    }

  ptr = str;
  n = strtol(str, &ptr, base);

  if (ptr==str) ok = 0;
  if ((ptr) && (*ptr)) ok = 0;

  return n;
}

bool Monitor::parse_bool(char *str, bool &ok)
{
  bool local_ok = 1;
  int n;

  n = parse_number(str, local_ok);
  if ((n<0) || (n>1))
    local_ok = 0;

  if (!local_ok)
    {
      // could parser y/n type responses here
      ok = 0;
    }

  return n;
}

/***********************************************************/


bool Monitor::quit(bool &run, int words, char **cmd)
{
  run = 0;
  doing_commands = 0;
  return 1;
}

bool Monitor::cont(bool &run, int words, char **cmd)
{
  doing_commands = !run;
  return 1;
}

bool Monitor::stop(bool &run, int words, char **cmd)
{
  run = 0;
  p->flush_events();
  return 1;
}

bool Monitor::go(bool &run, int words, char **cmd)
{
  bool ok=1;

  unsigned short pc;

  if (words > 1)
    {
      pc = parse_number(cmd[1], ok);
      if (ok)
        p->set_py(pc);
    }
    
  if (ok)
    {
      doing_commands = 0;
      run = 1;
    }

  return ok;
}

bool Monitor::ss(bool &run, int words, char **cmd)
{
  bool ok = 1;
  int sw;
  bool v;

  sw = parse_number(cmd[1], ok);

  if ((sw < 1) || (sw > 4))
    ok = 0;

  if (ok)
    {
      if (words>2)
        {
          v = parse_bool(cmd[2], ok);
          if (ok)
            p->set_ss(sw-1, v);
        }
      else
        printf("SS%d: %d\n", sw, p->get_ss(sw-1));
    }
  return ok;
}

bool Monitor::ptr(bool &run, int words, char **cmd)
{
  bool ok = 1;

  p->set_ptr_filename(cmd[1]);

  return ok;
}

bool Monitor::ptp(bool &run, int words, char **cmd)
{
  bool ok = 1;

  p->set_ptp_filename(cmd[1]);

  return ok;
}

bool Monitor::plt(bool &run, int words, char **cmd)
{
  bool ok = 1;

  p->set_plt_filename(cmd[1]);

  return ok;
}

bool Monitor::lpt(bool &run, int words, char **cmd)
{
  bool ok = 1;

  p->set_lpt_filename(cmd[1]);

  return ok;
}

bool Monitor::asr_ptr(bool &run, int words, char **cmd)
{
  bool ok = 1;
  
  p->set_asr_ptr_filename(cmd[1]);

  return ok;
}

bool Monitor::asr_ptp(bool &run, int words, char **cmd)
{
  bool ok = 1;
  
  p->set_asr_ptp_filename(cmd[1]);

  return ok;
}

bool Monitor::asr_ptr_on(bool &run, int words, char **cmd)
{
  bool ok = 1;
  
  p->asr_ptr_on(((words>0) ? cmd[1] : 0));

  return ok;
}

bool Monitor::asr_ptp_on(bool &run, int words, char **cmd)
{
  bool ok = 1;
  
  p->asr_ptp_on(((words>0) ? cmd[1] : 0));

  return ok;
}

#define REG_A 0
#define REG_B 1
#define REG_X 2

bool Monitor::a(bool &run, int words, char **cmd)
{
  return reg(run, words, cmd, REG_A);
}

bool Monitor::b(bool &run, int words, char **cmd)
{
  return reg(run, words, cmd, REG_B);
}

bool Monitor::x(bool &run, int words, char **cmd)
{
  return reg(run, words, cmd, REG_X);
}

bool Monitor::m(bool &run, int words, char **cmd)
{
  bool ok = 1;
  unsigned short addr, val;

  addr = parse_number(cmd[1], ok);

  if (words > 2)
    {
      val = parse_number(cmd[2], ok);
      if (ok)
        p->write(addr, val);
    }
  else
    {
      val = p->read(addr);
      printf("0x%04x \'%06o : 0x%04x \'%06o %s\n",
             addr, addr, val, val, binary16(val));
    }
  return ok;
}

char *Monitor::binary16(unsigned short n)
{
  static char buf[32];
  int i, j, k;
  i=15; j=2; k=0;

  while (i>=0)
    {
      if (--j <= 0)
        {
          buf[k++] = '.';
          j = 3;
        }

      buf[k++] = ((n >> i) & 1) ? '1':'0';
      i--;
    }
  buf[k++] = '\0';
  return buf;
}

bool Monitor::reg(bool &run, int words, char **cmd, int n)
{
  bool ok = 1;
  static const char *regname[]={"A", "B", "X"};

  unsigned short val;

  if (words > 1)
    {
      val = parse_number(cmd[1], ok);
      if (ok)
        {
          switch (n)
            {
            case REG_A: p->set_a(val); break;
            case REG_B: p->set_b(val); break;
            case REG_X: p->set_x(val); break;
            default:
              fprintf(stderr, "bad reg\n");
              exit(1);
            }
        }
    }
  else
    {
      switch (n)
        {
        case REG_A: val = p->get_a(); break;
        case REG_B: val = p->get_b(); break;
        case REG_X: val = p->get_x(); break;
        default:
          fprintf(stderr, "bad reg\n");
          exit(1);
        }
      printf("%s: 0x%04x \'%06o %s\n", regname[n],
             val, val, binary16(val));
    }
  return ok;
}


bool Monitor::clear(bool &run, int words, char **cmd)
{
  run = 0;
  p->master_clear();
  return 1;
}

bool Monitor::help(bool &run, int words, char **cmd)
{
  struct CmdTab *p;

  p = commands;
  while (p->name)
    {
      printf("%12s %s\n", p->name, p->descr);
      p++;
    }

  return 1;
}


bool Monitor::trace(bool &run, int words, char **cmd)
{
  bool ok = 1;
  char *filename = NULL;
  int lines = 0;

  /*
   * if cmd[1] is a number then it's assumed to be the
   * number of lines rather than a filename.
   * else treat it as the filename
   */
  if (words > 1)
    {
      if (strlen(cmd[1]) > 0)
        {
          lines = parse_number(cmd[1], ok);
          if (!ok)
            {
              filename = cmd[1];
              ok = 1;
              lines = 0;
            }
        }
    }

  if (words>2)
    lines = parse_number(cmd[2], ok);

  if (ok)
    p->dump_trace(filename, lines);

  return ok;
}

bool Monitor::disassemble(bool &run, int words, char **cmd)
{
  bool ok = true;
  char *filename = NULL;
  int first = 0;
  int last;
  int i=1;
  
  /*
   * if cmd[1] is a number then it's assumed to be the
   * number of lines rather than a filename.
   * else treat it as the filename
   */
  if (words > 1)
    {
      if (strlen(cmd[i]) > 0)
        {
          first = parse_number(cmd[i], ok);
          if (!ok)
            {
              filename = cmd[i];
              ok = 1;
              first = parse_number(cmd[++i], ok);
            }
        }
    }

  
  if (words>2)
    last = parse_number(cmd[++i], ok);
  else
    last = first;

  if (ok)
    p->dump_disassemble(filename, first, last);

  return ok;
}

bool Monitor::vmem(bool &run, int words, char **cmd)
{
  bool ok = true;
  char *filename = NULL;
  int exec_addr = 0;
  
  /*
   * if cmd[1] is a number then it's assumed to be the
   * number of lines rather than a filename.
   * else treat it as the filename
   */
  if (words > 1)
    filename = cmd[1];

  if (words>2)
    exec_addr = parse_number(cmd[2], ok);

  if (ok)
    p->dump_vmem(filename, exec_addr);
  
  return ok;
}

bool Monitor::omem(bool &run, int words, char **cmd)
{
  bool ok = true;
  char *filename = NULL;
  int exec_addr = 0;
  
  /*
   * if cmd[1] is a number then it's assumed to be the
   * number of lines rather than a filename.
   * else treat it as the filename
   */
  if (words > 1)
    filename = cmd[1];

  if (words>2)
    exec_addr = parse_number(cmd[2], ok);

  if (ok)
    p->dump_vmem(filename, exec_addr, true);
  
  return ok;
}

bool Monitor::coemem(bool &run, int words, char **cmd)
{
  bool ok = true;
  char *filename = NULL;
  int exec_addr = 0;
  
  /*
   * if cmd[1] is a number then it's assumed to be the
   * number of lines rather than a filename.
   * else treat it as the filename
   */
  if (words > 1)
    filename = cmd[1];

  if (words>2)
    exec_addr = parse_number(cmd[2], ok);

  if (ok)
    p->dump_coemem(filename, exec_addr);
  
  return ok;
}


bool Monitor::license(bool &run, int words, char **cmd)
{
  bool ok = true;
  int i;
  int first = license_sections[0];
  int last = license_sections[2];

  for (i=first; i<last; i++)
    printf("%s\n", full_license_text[i]);

  return ok;
}

bool Monitor::warranty(bool &run, int words, char **cmd)
{
  bool ok = true;
  int i;
  int first = license_sections[2];
  int last = license_sections[3];

  for (i=first; i<last; i++)
    printf("%s\n", full_license_text[i]);

  return ok;
}
