/* Honeywell Series 16 emulator
 * Copyright (C) 1998, 2004, 2005, 2026  Adrian Wise
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

#include "monitor.hpp"

#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cctype>
#include <cassert>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <utility>
#include <iostream>
#include <format>

#include "proc.hpp"
#include "stdtty.hpp"
#include "gpl.h"
#include "version.h"
#include "asr_intf.hpp"

#define PROMPT "MON"

namespace h16 {
  static Monitor *monitor = 0;
  static void sig_handler(int signo);
}

using namespace h16;

const std::vector<std::string> Monitor::instructions_text {
  "Type \"license\" or \"warranty\" for details\n"
};

const std::vector<Monitor::CmdTab> Monitor::commands {
  {"a",          CmdTab::RNA, 0, 1, "[data] : Get/Set A register",                  &Monitor::a},
  {"b",          CmdTab::RNA, 0, 1, "[data] : Get/Set B register",                  &Monitor::b},
  {"x",          CmdTab::RNA, 0, 1, "[data] : Get/Set X register",                  &Monitor::x},
  {"m",          CmdTab::ANY, 1, 2, "addr [data] : Get/Set memory",                 &Monitor::m},
  {"quit",       CmdTab::ANY, 0, 0, "Quit emulation",                               &Monitor::quit},
  {"continue",   CmdTab::ANY, 0, 0, "Continue",                                     &Monitor::cont},
  {"stop",       CmdTab::ANY, 0, 0, "Stop",                                         &Monitor::cont},
  {"limit",      CmdTab::ANY, 1, 1, "half_cycles : Set limit on simulated time",    &Monitor::limit},
  {"sbi",        CmdTab::ANY, 1, 1, "half_cycles : Schedule startbutton interrupt", &Monitor::sbi},
  {"go",         CmdTab::HLT, 0, 1, "[addr] Start execution",                       &Monitor::go},
  {"ss",         CmdTab::ANY, 1, 2, "num [0/1] Get/Set Sense Switch",               &Monitor::ss},
  {"ptr",        CmdTab::ANY, 1, 1, "filename : Set Papertape Reader filename",     &Monitor::ptr},
  {"ptp",        CmdTab::ANY, 1, 1, "filename : Set Papertape Punch filename",      &Monitor::ptp},
  {"plt",        CmdTab::ANY, 1, 1, "filename : Set Plotter filename",              &Monitor::plt},
  {"lpt",        CmdTab::ANY, 1, 1, "filename : Set Lineprinter filename",          &Monitor::lpt},
  {"asr_ptr",    CmdTab::ANY, 1, 1, "filename : Set ASR Papertape Reader filename", &Monitor::asr_ptr},
  {"asr_ptr_on", CmdTab::ANY, 0, 1, "[filename] : Turn on ASR Papertape Reader",    &Monitor::asr_ptr_on},
  {"asr_ptp",    CmdTab::ANY, 1, 1, "filename : Set ASR Papertape Punch filename",  &Monitor::asr_ptp},
  {"asr_ptp_on", CmdTab::ANY, 0, 1, "[filename] : Turn on ASR Papertape Punch",     &Monitor::asr_ptp_on},
  {"clear",      CmdTab::ANY, 0, 0, "Master clear",                                 &Monitor::clear},
  {"help",       CmdTab::ANY, 0, 0, "Print this help",                              &Monitor::help},
  {"trace",      CmdTab::ANY, 0, 2, "[filename] [,lines] : Save trace file",        &Monitor::trace},
  {"disassemble",CmdTab::ANY, 1, 3, "[filename] first [,last] : Save disassembly",  &Monitor::disassemble},
  {"vmem",       CmdTab::ANY, 1, 2, "filename [, exec-addr] : Save Verilog Mem.",   &Monitor::vmem},
  {"omem",       CmdTab::ANY, 1, 2, "filename [, exec-addr] : Save Octal Mem.",     &Monitor::omem},
  {"coemem",     CmdTab::ANY, 1, 2, "filename [, exec-addr] : Save Xilinx .coe",    &Monitor::coemem},
  {"license",    CmdTab::ANY, 0, 0, "Print license information",                    &Monitor::license},
  {"warranty",   CmdTab::ANY, 0, 0, "Statement of no warranty",                     &Monitor::warranty},
};

static void h16::sig_handler(int signo)
{
  if (monitor) {
    monitor->sig_handler(signo);
  }
}

void Monitor::sig_handler(int signo)
{
  if (signo == SIGINT) {
    p.goto_monitor();
  }
}

Monitor::Monitor(Proc &p, int argc, char **argv)
  : p(p)
  , argc(argc)
  , argv(argv)
  , stdTty(StdTty::getInstance())
  , first_time(true)
  , doing_commands(false)
  , run(false)
{
  if (!monitor) {
    struct sigaction sa;

    bzero(&sa, sizeof(struct sigaction));
           
    sa.sa_handler = &::sig_handler;
    
    if (sigaction(SIGINT, &sa, 0)) {
      abort();
    }
    monitor = this;
  }

  // Populate the command_map
  for (auto &cmd: commands) {
    command_map.insert({cmd.name,cmd});
    assert(cmd.name.size() > 0);
    for (unsigned l = (cmd.name.size()-1); l > 0; l--) {
      // Form an abbreviated name of l characters
      std::string sname(cmd.name.substr(0, l));
      // Is it different to all of the other commands?
      bool unique = true;
      for (auto &cmd2: commands) {
        if (&cmd != &cmd2) {
          std::string tname(cmd2.name.substr(0, l));
          if (tname == sname) {
            l = 1;
            unique = false;
            break;
          }
        }
      }
      if (unique) {
        command_map.insert({sname,cmd});
      }
    }
  }

}

void Monitor::get_line(std::string &buffer, const std::string prompt, std::ifstream &is) {
  if (is) {
    std::getline(is, buffer);
    
    if (is) {
      // Nibble off trailing space
      buffer.erase(std::find_if(buffer.rbegin(), buffer.rend(),
                                [](unsigned char ch) {return !std::isspace(ch);}).base(),
                   buffer.end());

      std::cout << prompt << buffer << '\n';
    } else {
      is.close();
    }
  }
  
  if (!is) {
    stdTty.get_input(prompt, buffer, true);
  }
}

void Monitor::do_commands(bool &run, std::ifstream &is)
{
  std::string prompt(std::format("{}> ", PROMPT));
  
  if (!run) {
    p.flush_events();
  }
  
  if (is.is_open()) {
    std::cout << '\n';
  } else {
    // don't print if reading from file
    if (first_time) {
      first_time = false;
      int i = 0;
      while (copyright_text[i]) {
        std::cout << copyright_text[i++];
      }
      for (auto &str: instructions_text) {
        std::cout << str;
      }
    }
    std::cout << std::format("\n{}: A:{:0>6o} B:{:0>6o} X:{:0>6o} {}\n", PROMPT,
                             (p.get_a() & 0xffff), (p.get_b() & 0xffff),
                             (p.get_x() & 0xffff),
                             p.dis());
  }
  
  doing_commands = 1;
  std::string buffer;
  
  while (doing_commands) {
    buffer.clear();
    get_line(buffer, prompt, is);
    
    std::vector<std::string> words;
    break_command(words, buffer);
    bool ok = do_command(run, words);
    if ((is.is_open()) && (!ok)) {
      doing_commands = false;
      run = false;
    }
  }

  std::cout.flush();
  stdTty.set_canonical(false);
}

void Monitor::break_command(std::vector<std::string> &words, const std::string &buffer) {

  words.clear();
  
  bool first = true;

  unsigned i = 0;
  while (i < buffer.size()) { 
    unsigned char c = buffer[i];

    // Skip leading whitespace
    while (std::isspace(c)) {
      if (++i < buffer.size()) {
        c = buffer[i];
      }
    }
    
    unsigned j = i;

    while ((i < buffer.size()) && (!std::isspace(c)) &&
           (c != ',') &&
           (!((std::isdigit(c) || (c=='\'')) && first))) {
      //
      // Note the special case above. The separator can be
      // omitted after the command if the argument is numerical.
      //
      if (++i < buffer.size()) {
        c = buffer[i];
      }
    }
    first = false;

    if (i > j) {
      words.push_back(buffer.substr(j,i-j));
    }

    if (i < buffer.size()) {
      if (std::isspace(c) || (c == ',')) {
        i++;
      }
    }
  }
}

bool Monitor::do_command(bool &run, std::vector<std::string> words) {

  bool ok = true;
  
  if (words.size() == 0) return ok; // Blank lines allowed in script files
  
  if (words.front()[0] == '#') return ok; // comment

  std::string &scmd {words.front()};
  if (command_map.count(scmd) != 0) {
    const CmdTab &cte (command_map.at(scmd));

    // delete the first word to leave just the arguments
    words.erase(words.begin());
    unsigned num_args = words.size();
    
    if (run && ((cte.when == CmdTab::HLT) ||
                ((cte.when == CmdTab::RNA) && (num_args > 0)))) {
      std::cerr << std::format("! {} can only be used when halted\n", cte.name);
      ok = false;
    } else  {
      if ((num_args >= cte.min_args) && (num_args <= cte.max_args)) {
        
        this->run = run;
        ok = (this->*cte.cmd)(words); // Call the command's function
        run = this->run;

      } else {
        std::cerr << std::format("Expected {:d} to {:d} arguments\n", cte.min_args, cte.max_args);
        ok = false;
      }
    }
  } else {
    ok = false;
  }

  if (! ok) {
    std::cout << "??\n";
  }
  return ok;
}

long Monitor::parse_number(const std::string &s, bool &ok) {
  const char *str = s.c_str();
  const char *ptr;
  int base=0;
  long n;

  if (str[0] == '\'') {
    str++;
    base = 8;
  }
  
  ptr = str;
  n = std::strtol(str, const_cast<char **>(&ptr), base);
  
  if ((ptr) && ((*ptr != '\0') || (ptr == str))) {
    ok = false;
  }
  
  return n;
}

unsigned long long Monitor::parse_ull(const std::string &s, bool &ok)
{
  const char *str = s.c_str();
  const char *ptr;
  int base=0;
  unsigned long long n;

  if (str[0] == '\'') {
    str++;
    base = 8;
  }

  ptr = str;
  n = std::strtoull(str, const_cast<char **>(&ptr), base);

  if ((ptr) && ((*ptr != '\0') || (ptr == str))) {
    ok = false;
  }

  return n;
}

bool Monitor::parse_bool(const std::string &s, bool &ok)
{
  bool local_ok = true;
  long n;

  n = parse_number(s, local_ok);
  if ((n<0) || (n>1))
    local_ok = 0;

  if (!local_ok) {
    if ((s == "true") || (s == "TRUE") || (s == "True")) {
      n = 1;
      local_ok = true;
    } if ((s == "false") || (s == "FALSE") || (s == "False")) {
      n = 1;
      local_ok = true;
    }
    
    ok = local_ok;
  }

  return n;
}

/***********************************************************/


bool Monitor::quit(const std::vector<std::string> &args) {
  run = false;
  doing_commands = false;
  return true;
}

bool Monitor::cont(const std::vector<std::string> &args) {
  run = true;
  doing_commands = false;
  return true;
}

bool Monitor::stop(const std::vector<std::string> &args) {
  run = false;
  p.flush_events();
  return true;
}

bool Monitor::go(const std::vector<std::string> &args) {
  bool ok = true;

  if (args.size() > 0) {
    unsigned short pc = parse_number(args.front(), ok);
    if (ok) {
        p.set_py(pc);
    }
  }
  if (ok) {
    doing_commands = false;
    run = true;
  }
  
  return ok;
}

bool Monitor::limit(const std::vector<std::string> &args) {
  bool ok = true;

  uint64_t half_cycles = parse_ull(args.front(), ok);
  if (ok) {
    p.set_limit(half_cycles);
  }

  return ok;
}

bool Monitor::sbi(const std::vector<std::string> &args) {
  bool ok = true;

  uint64_t half_cycles = parse_ull(args.front(), ok);
  if (ok) {
    p.queue_sbi(half_cycles);
  }
  
  return ok;
}

bool Monitor::ss(const std::vector<std::string> &args) {
  bool ok = true;
  int sw;
  bool v;

  sw = parse_number(args.front(), ok);
  
  if ((sw < 1) || (sw > 4))
    ok = false;

  if (ok) {
    if (args.size() > 1) {
      v = parse_bool(args[1], ok);
      if (ok) {
        p.set_ss(sw-1, v);
      }
    } else {
      std::cout << std::format("SS{:d}: {:d}\n", sw, p.get_ss(sw-1));
    }
  }
  return ok;
}

#define FNAME(fn,DEV)                                           \
bool Monitor::fn(const std::vector<std::string> &args) {        \
  p.set_filename(IoDispatch::Device::DEV, args.front());        \
  return true;                                                  \
}

FNAME(ptr, PTR)
FNAME(ptp, PTP)
FNAME(plt, PLT)
FNAME(lpt, LPT)

#define ASR_FNAME(fn,SUB)                                               \
  bool Monitor::fn(const std::vector<std::string> &args) {              \
    p.set_filename(IoDispatch::Device::ASR, args.front(), AsrIntf::SUB); \
    return true;                                                        \
  }

ASR_FNAME(asr_ptr,PTR)
ASR_FNAME(asr_ptp,PTP)

#define ASR_FNAME_EV(fn,SUB,EVENT)                              \
  bool Monitor::fn(const std::vector<std::string> &args) {      \
    if (args.size() > 0) {                                      \
      p.set_filename(IoDispatch::Device::ASR, args.front(),     \
                     AsrIntf::SUB);                             \
    }                                                           \
    p.send_event(IoDispatch::Device::ASR,                       \
                 static_cast<int>(AsrIntf::Event::EVENT));      \
    return true;                                                \
  }

ASR_FNAME_EV(asr_ptr_on,PTR,PTR_ON)
ASR_FNAME_EV(asr_ptp_on,PTP,PTP_ON)


bool Monitor::a(const std::vector<std::string> &args) {
  return reg(args, REG::A);
}

bool Monitor::b(const std::vector<std::string> &args) {
  return reg(args, REG::B);
}

bool Monitor::x(const std::vector<std::string> &args) {
  return reg(args, REG::X);
}

std::string Monitor::reg_name(REG r) {
  std::string s;
  switch (r) {
  case REG::A: s = "A"; break;
  case REG::B: s = "B"; break;
  case REG::X: s = "X"; break;
  default:     s = "?";
  }
  return s;
}

bool Monitor::reg(const std::vector<std::string> &args, REG r) {
  bool ok = true;
  std::string regname(reg_name(r));

  unsigned short val;

  if (args.size() > 0) {
    val = parse_number(args.front(), ok);
    if (ok) {
      switch (r) {
      case REG::A: p.set_a(val); break;
      case REG::B: p.set_b(val); break;
      case REG::X: p.set_x(val); break;
      default: assert(!"bad reg");
      }
    }
  } else {
    switch (r) {
    case REG::A: val = p.get_a(); break;
    case REG::B: val = p.get_b(); break;
    case REG::X: val = p.get_x(); break;
    default: assert(!"bad reg");
    }
    std::cout << std::format("{}: 0x{:0>4x} \'{:0>6o} {}\n",
                             regname, val, val, binary16(val));
  }
  return ok;
}

bool Monitor::m(const std::vector<std::string> &args) {
  bool ok = true;
  unsigned short addr, val;

  addr = parse_number(args.front(), ok);

  if (args.size() > 1) {
    val = parse_number(args[1], ok);
    if (ok) {
      p.write(addr, val);
    }
  } else {
    val = p.read(addr);
    std::cout << std::format("0x{:0>4x} \'{:0>6o} : 0x{:0>4x} \'{:0>6o} {}\n",
                             addr, addr, val, val, binary16(val));
  }
  return ok;
}

std::string Monitor::binary16(unsigned short n) {
  std::string str;
  int i = 15;
  int j = 2;

  while (i>=0) {
    if (--j <= 0) {
      str.push_back('.');
      j = 3;
    }

    str.push_back(((n >> i) & 1) ? '1':'0');
    i--;
  }
  return str;
}



bool Monitor::clear(const std::vector<std::string> &args) {
  run = 0;
  p.master_clear();
  return true;
}

bool Monitor::help(const std::vector<std::string> &args) {
  for (auto &cmd: commands) {
    std::cout << std::format("{:>12} {}\n", cmd.name, cmd.descr);
  }

  return true;
}


bool Monitor::trace(const std::vector<std::string> &args) {
  bool ok = true;
  std::string filename;
  int lines = 0;

  /*
   * if args[0] is a number then it's assumed to be the
   * number of lines rather than a filename.
   */
  if (args.size() > 0) {
    lines = parse_number(args.front(), ok);
    if (!ok) {
      filename = args.front();
      ok = true;
      lines = 0;
    }
  }

  if (args.size() > 1) {
    lines = parse_number(args[1], ok);
  }
  
  if (ok) {
    ok = p.dump_trace(filename, lines);
  }
  
  return ok;
}

bool Monitor::disassemble(const std::vector<std::string> &args) {
  bool ok = true;
  std::string filename;
  int first = 0;
  int last = 0;
  unsigned i=1;

  /*
   * if args[0] is a number then it's assumed to be the
   * address rather than a filename.
   */
  first = parse_number(args.front(), ok);
  if (!ok) {
    filename = args.front();
    ok = (args.size() > 1);
    if (ok) {
      first = parse_number(args[1], ok);
      i++; // last would be args[2] if it's present
    }
  }

  if (ok) {
    if (args.size() > i) {
      last = parse_number(args[i], ok);
    } else {
      last = first;
    }
  }

  if (ok) {
    ok = p.dump_disassemble(filename, first, last);
  }
  
  return ok;
}

#define MEM(fn, pfn, arg)                                       \
  bool Monitor::fn(const std::vector<std::string> &args) {      \
    bool ok = true;                                             \
    std::string filename(args.front());                         \
    int exec_addr = 0;                                          \
    if (args.size()>1) {                                        \
      exec_addr = parse_number(args[1], ok);                    \
    }                                                           \
    if (ok) {                                                   \
      ok = p.pfn(filename, exec_addr arg);                      \
    }                                                           \
    return ok;                                                  \
  }

#define AF ,false
#define AT ,true
#define NA

MEM(vmem, dump_vmem, AF)
MEM(omem, dump_vmem, AT)
MEM(coemem, dump_coemem, NA)

bool Monitor::license(const std::vector<std::string> &args) {
  int i;
  int first = license_sections[0];
  int last = license_sections[2];

  for (i=first; i<last; i++) {
    std::cout << full_license_text[i] << '\n';
  }

  return true;
}

bool Monitor::warranty(const std::vector<std::string> &args) {
  int i;
  int first = license_sections[2];
  int last = license_sections[3];

  for (i=first; i<last; i++) {
    std::cout << full_license_text[i] << '\n';
  }

  return true;
}
