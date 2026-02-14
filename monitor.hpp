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
 */

#ifndef _MONITOR_HPP_
#define _MONITOR_HPP_

#include <string>
#include <vector>
#include <map>
#include <fstream>

namespace h16 {
        
  class Proc;
  class StdTty;

  class Monitor {
  public:
    Monitor(Proc &p, int argc, char **argv);
    void do_commands(bool &run, std::ifstream &is);
    void sig_handler(int signo);
    
  private:
    struct CmdTab {
      enum WhenValid {
        HLT, // Only when halted
        ANY, // Always valid
        RNA  // When halted, or Running if No Args
      };
      const std::string name;
      WhenValid when;
      unsigned min_args;
      unsigned max_args;
      const std::string descr;
      
      bool (Monitor::*cmd)(const std::vector<std::string> &args);
    };

    enum class REG {
      A, B, X
    };

    Proc &p;
    int argc;
    char **argv;
    StdTty &stdTty;
  
    bool first_time;
    bool doing_commands;
    bool run;

    static const std::vector<std::string> instructions_text;
    static const std::vector<CmdTab> commands;

    std::map<std::string, const CmdTab &> command_map;
  
    void get_line(std::string &buffer, const std::string prompt, std::ifstream &is);
    void break_command(std::vector<std::string> &words, const std::string &buffer);
    bool do_command(bool &run, std::vector<std::string> words);
    long parse_number(const std::string &s, bool &ok);
    unsigned long long parse_ull(const std::string &s, bool &ok);
    bool parse_bool(const std::string &s, bool &ok);
    std::string binary16(unsigned short n);
    std::string reg_name(REG r);
    bool reg(const std::vector<std::string> &args, REG r);
  
    bool quit(const std::vector<std::string> &args);
    bool cont(const std::vector<std::string> &args);
    bool stop(const std::vector<std::string> &args);
    bool go(const std::vector<std::string> &args);
    bool limit(const std::vector<std::string> &args);
    bool sbi(const std::vector<std::string> &args);
    bool ss(const std::vector<std::string> &args);
    bool ptr(const std::vector<std::string> &args);
    bool ptp(const std::vector<std::string> &args);
    bool plt(const std::vector<std::string> &args);
    bool lpt(const std::vector<std::string> &args);
    bool asr_ptr(const std::vector<std::string> &args);
    bool asr_ptr_on(const std::vector<std::string> &args);
    bool asr_ptp(const std::vector<std::string> &args);
    bool asr_ptp_on(const std::vector<std::string> &args);
    bool a(const std::vector<std::string> &args);
    bool b(const std::vector<std::string> &args);
    bool x(const std::vector<std::string> &args);
    bool m(const std::vector<std::string> &args);
    bool clear(const std::vector<std::string> &args);
    bool help(const std::vector<std::string> &args);
    bool trace(const std::vector<std::string> &args);
    bool disassemble(const std::vector<std::string> &args);
    bool vmem(const std::vector<std::string> &args);
    bool omem(const std::vector<std::string> &args);
    bool coemem(const std::vector<std::string> &args);
    bool license(const std::vector<std::string> &args);
    bool warranty(const std::vector<std::string> &args);
  };
}
#endif // _MONITOR_HPP_
