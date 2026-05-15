/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 1999, 2005, 2010, 2011, 2018, 2026  Adrian Wise
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
 * This file constitutes the main simulation kernel of the processor.
 */

#include "proc.hpp"

#include <cassert>
#include <iostream>
#include <format>
#include <iostream>
#include <fstream>

#include "iodev.hpp"
#include "rtc.hpp"
#include "ptr.hpp"
#include "ptp.hpp"
#include "plt.hpp"
#include "asr_intf.hpp"
#include "stdtty.hpp"
#include "lpt.hpp"
#include "plt.hpp"

#include "instr.hpp"
#include "file_exception.hpp"

// How long is the start button depressed for (half_cycles)
#define START_BUTTON_DOWN_TIME 1000

using namespace h16;

Proc::Proc(bool hasEa)
  : CPU(hasEa)
  , goto_monitor_flag(false)
  , exit_code(0)
  , exit_called(false)
  , ioDispatch(*this)
  , event_queue(*this)
{
  master_clear();

  mfm = new Mfm(*this);
}

Proc::~Proc()
{
  event_queue.discard_events();
  delete mfm;
}

/*
    enum class DI { // do_instr return codes
      OK,   // No problems
      HALT, // Halt instruction cleared runff
      EXIT, // exit() has been called
      MON,  // Request to (re)enter the monitor
      FNAM, // Device requesting filename
    };
*/
Proc::DI Proc::do_instr() {
  DI ret = DI::OK;
  
  bool run = false;
  
  try {
    run = CPU::do_instr();
  }
  catch (FileException &fe) {
    unwind_instr();

    // Return immediately - changing no further state
    return DI::FNAM;
  }
  
  /*
   * If we're still running then service any
   * events that are due now.
   * If we just halted then flush all pending
   * events.
   */
  if (run) {
    (void) event_queue.call_devices(get_half_cycles());
  } else {
    event_queue.flush_events(get_half_cycles_ref());
    set_sbi(false);
  }
  
  /*
   * If there is TTY input then service it
   */
  StdTty::service();

  if (exit_called) {
    ret = DI::EXIT;
  } else if (goto_monitor_flag) {
    ret = DI::MON;
    goto_monitor_flag = false;
  } else if (!run) {
    ret = DI::HALT;
  }

  return ret;
}

void Proc::exit(int code)
{
  exit_code   = code;
  exit_called = true;
  set_run(false);
}

void Proc::set_limit(uint64_t half_cycles)
{
  event_queue.queue(get_half_cycles() + half_cycles, *mfm, Mfm::Event::LIMIT);
}
void Proc::queue_sbi(uint64_t half_cycles)
{
  event_queue.queue(get_half_cycles() + half_cycles, *mfm, Mfm::Event::START_DOWN);
}

void Proc::event(int reason)
{
  switch (reason) {
  case Mfm::Event::LIMIT:
    std::cout << std::format("\n{:0>10d}: limit reached\n", get_half_cycles());
    goto_monitor();
    break;

  case Mfm::Event::START_DOWN:
    if (get_run()) {
      start_button();
    }
    break;

  case Mfm::Event::START_UP:
    set_sbi(false);
    // If a HLT occurred after start went down (causing the start-button
    // interrupt) then as the start button goes up the processor starts
    // again.
    set_run(true);
    break;

  default:
    std::cerr << std::format("Bad event reason <{:d}", reason) << std::endl;
    abort();
  }
}

/*****************************************************************
 * Master clear
 *****************************************************************/
void Proc::master_clear()
{
  CPU::master_clear();

  goto_monitor_flag = false;
  ioDispatch.master_clear_devices();
  event_queue.discard_events();
}

std::string Proc::get_file_name(const std::string &device_name,
                                const std::string &extension,
                                const std::string &description) {

  if (filename_valid) {
    filename_valid = false;
    return filename;
  }
  
  this->device_name = device_name;
  this->extension = extension;
  this-> description = description;
  
  FileException fe;
  throw (fe);
}

bool Proc::is_booting() const {
  uint16_t p = get_p();
  return ((p > 0) && (p < 020)); 
}

void Proc::anomaly(Level level, const std::string &message) {

  static std::map<Level, const std::string> lnames {
    {Level::FATAL, "Fatal"},
    {Level::ERROR, "Error"},
    {Level::WARNING, "Warning"}
  };
  const std::string l = lnames.count(level) ? lnames.at(level) : "???";
  
  // TODO - count how many times errors are printed and silence them

  std::cerr << l << ": " << message << std::endl;

  if (level == Level::FATAL) {
    exit(1);
  }
  // TODO - repeated FATAL should exit immediately (rather than complete the instruction)
}

bool Proc::jump_time_to_event(uint64_t &half_cycles) {

  /*
   * If there are any events in the queue, then jump time
   * forwards.
   */
  bool r = event_queue.next_event_time(half_cycles);

  if (r) {
    // call devices to actually change the state
    // of the devices
    event_queue.call_devices(half_cycles);
  }

  return r;
}

void Proc::io_polling(uint16_t instr) {

  // Currently this does nothing...
  
  // There are no events pending
  //
  // Are we waiting on the ASR for input?
  //
  // We could wait for keyboard input
  // at this point, however this won't
  // work for the case that there is a GUI
  // frontpanel since that needs to be
  // serviced too.
  //
  // Ideally we should return to the
  // GUI code, remove the idle procedure
  // and wait until a GUI event or keyboard
  // input occurs before reinvoking the
  // call to the kernel.
  //
  // However, this is a bit complicated
  // so for now just let it poll the
  // device

  if ((instr & 077) == static_cast<uint16_t>(IoDispatch::Device::ASR)) {
    // Ought to wait for keyboard or GUI
  } else {
    // Do something about apparent infinite loop?
  }
}



/*
 * start button routines
 */

void Proc::start_button()
{
  // What the real hardware does is assert the interrupt line while
  // the start button is depressed. If during this time the interrupts
  // are enabled then an interrupt occurs (the program can't tell it
  // was caused by the start button and can't acknowledge that particular
  // interrupt source).
  //
  // This is a bit of a kludge, but works from the monitor as well as
  // the graphical front-panel (for scripted runs) - pretend the button
  // was depressed for a fixed period of time.

  set_sbi(true);

  event_queue.queue(get_half_cycles() + START_BUTTON_DOWN_TIME, *mfm, Mfm::Event::START_UP);
}

std::string Proc::dis()
{
  return CPU::dis();
}

void Proc::flush_events()
{
  event_queue.flush_events(get_half_cycles_ref());
}


void Proc::set_filename(IoDispatch::Device dev, const std::string &filename, int subdevice) {
  ioDispatch.set_filename(dev, filename, subdevice);
}
  
void Proc::send_event(IoDispatch::Device dev, unsigned reason) {
  ioDispatch.event(dev, reason);
}

bool Proc::special(char k)
{
  bool r = false;

  r = ioDispatch.tty_special(k);

  if ( (k & 0x7f) == 'h' ) {
    std::cout << "ALT-m Go to the monitor\nALT-s Start button interrupt" << std::endl;
  }

  if (!r) {
    switch (k & 0x7f) {
    case 's':
      start_button();
      r = 1;
      break;
    case 'm':
      goto_monitor();
      r = 1;
      break;
    }
  }
  
  return r;
}

/*
 * ===================================================================================
 *
 * Interface from CPU to call I/O devices
 */

IoStatus Proc::ina(uint16_t instr, int16_t &data) {
  return ioDispatch.ina(instr, data);
};

IoStatus Proc::sks(uint16_t instr) {
  return ioDispatch.sks(instr);
};

IoStatus Proc::ota(uint16_t instr, int16_t data) {
  return ioDispatch.ota(instr, data);
};

void Proc::ocp(uint16_t instr) {
  return ioDispatch.ocp(instr);
}

void Proc::smk(uint16_t mask) {
  return ioDispatch.smk(mask);
}

void Proc::event(IoDevice dev, int reason) {
  return ioDispatch.event(static_cast<IoDispatch::Device>(dev), reason);
}

void Proc::dmc(unsigned dmc_dev, int16_t &data, bool erl) {
  return ioDispatch.dmc(dmc_dev, data, erl);
}

/*
 * ===================================================================================
 */

bool Proc::dump_trace(const std::string &filename, unsigned n) {

  const unsigned TRACE_BUF(btrace_buf.size());
  
  unsigned i;
  std::ofstream ofs;

  /*
   * If a filename is passed then open it.
   */
  if (filename.size() > 0) {
    ofs.open(filename);
    if (!ofs) {
      std::cerr << std::format("Could not open <{}>\n", filename);
      return false;
    }
  }
  
  std::ostream &os((ofs.is_open()) ? ofs : std::cout);
  
  i = (trace_ptr + TRACE_BUF - n) % TRACE_BUF;

  do {
    if (btrace_buf[i].v) {
      if (btrace_buf[i].brk && (btrace_buf[i].instr < 16)) {
        uint16_t dmc_addr, dmc_data;
        bool dmc_erl, dmc_wrt;
        dmc_data = btrace_buf[i].p & 0xffff;
        dmc_addr = btrace_buf[i].y & 0xffff;
        dmc_erl  = btrace_buf[i].c;

        dmc_wrt = ((dmc_addr & 0x8000) != 0);
        dmc_addr &= 0x7fff;

        os << std::format("{:0>10d}: {} {:0>6o} {} {:0>5o} {} {}\n",
                          btrace_buf[i].half_cycles,
                          ((dmc_wrt) ? "Write" : "Read"), dmc_data,
                          ((dmc_wrt) ? "to " : "from"), dmc_addr,
                          ((dmc_erl) ? "ERL" : "   "),
                          instr_table.disassemble(btrace_buf[i].p,
                                                  btrace_buf[i].instr,
                                                  btrace_buf[i].brk,
                                                  btrace_buf[i].y,
                                                  true/*y_valid*/).c_str());
      } else {
        os << std::format("{:0>10d}: A:{:0>6o} B:{:0>6o} X:{:0>6o} C:{:1d} {}\n",
                          btrace_buf[i].half_cycles,
                          (btrace_buf[i].a & 0xffff),
                          (btrace_buf[i].b & 0xffff),
                          (btrace_buf[i].x & 0xffff),
                          (btrace_buf[i].c & 1),
                          instr_table.disassemble(btrace_buf[i].p,
                                                  btrace_buf[i].instr,
                                                  btrace_buf[i].brk,
                                                  btrace_buf[i].y,
                                                  true/*y_valid*/).c_str());
      }
    }
    i = (i+1) % TRACE_BUF;
  } while (trace_ptr != i);

  if (ofs.is_open()) {
    ofs.close();
  }

  return true;
}

/*****************************************************************
 * Dump a disassembly of core between the given addresses
 *****************************************************************/

bool Proc::dump_disassemble(const std::string &filename, unsigned first, unsigned last) {
  unsigned i;
  uint16_t instr;
  std::ofstream ofs;

  /*
   * If a filename is passed then open it.
   */
  if (filename.size() > 0) {
    ofs.open(filename);
    if (!ofs) {
      std::cerr << std::format("Could not open <{}>\n", filename);
      return false;
    }
  }
  
  std::ostream &os((ofs.is_open()) ? ofs : std::cout);

  for (i=first; i<=last; i++) {
    instr = core[i];
    if (instr_table.defined(instr)) {
      os << instr_table.disassemble(i, instr, false).c_str() << '\n';
    } else {
      os << std::format("{:0>6o}  {:0>6o}    {}\n", i, instr, "???");
    }
  }

  if (ofs.is_open()) {
    ofs.close();
  }

  return true;
}

bool Proc::dump_vmem(const std::string &filename, unsigned exec_addr, bool octal) {
  unsigned i;
  uint16_t instr;
  bool mod;
  bool dac;
  bool skip = true;
  const unsigned CORE_SIZE = core.size();

  std::ofstream ofs;

  /*
   * If a filename is passed then open it.
   */
  if (filename.size() > 0) {
    ofs.open(filename);
    if (!ofs) {
      std::cerr << std::format("Could not open <{}>\n", filename);
      return false;
    }
  }
  
  std::ostream &os((ofs.is_open()) ? ofs : std::cout);

  // As a special-case, a contiguous block of zeros from the
  // top of core is treated as if it had never been written
  // (this allows scripting, such as in h16-ld.in, to clear
  // top of core where LDR-APM and PAL-AP were and not have
  // this dumped)

  unsigned core_end = CORE_SIZE-1;
  while ((core_end > 0) && (core[core_end]==0)) {
    --core_end;
  }

  for (i=0; i<=core_end; i++) {
    instr = core[i];
    mod = modified[i];
    dac = false;

    if ((i == 0) && (exec_addr != 0)) {
      if ((exec_addr & (~0777)) != 0)
        // Not sector zero
        instr = 0102020; // JMP *'20
      else
        instr = 0002000 | exec_addr; // JMP exec_addr
      mod = true;
    } else if ((i == 020) && ((exec_addr & (~0777)) != 0)) {
      instr = exec_addr;
      mod = true;
      dac = true;
    } else if ((i>0) && (i<020))
      mod = true;

    if (mod && skip) {
      // Need an @ line
      if (octal) {
        os << std::format("@{:0>6o}\n", i);
      } else {
        os << std::format("@{:0>4x}\n", i);
      }
    }

    if (mod) {
      std::string str;
      if (dac) {
        str = std::format("{:0>6o}  {:0>6o}    DAC  '{:0>6o}", i, instr, instr);
      } else if (instr_table.defined(instr)) {
        str = instr_table.disassemble(i, instr, false);
      } else {
        str = std::format("{:0>6o}  {:0>6o}    {}", i, instr, "???");
      }
      
      if (octal) {
        os << std::format("{:0>6o} // {}\n", instr, str);
      } else {
        os << std::format("{:0>4x} // {}\n", instr, str);
      }
    }

    skip = !mod;
  }

  if (ofs.is_open()) {
    ofs.close();
  }

  return true;
}

bool Proc::dump_coemem(const std::string &filename, unsigned exec_addr) {
  unsigned i, k, n, last_addr;
  uint16_t instr;
  const unsigned CORE_SIZE = core.size();
  std::ofstream ofs;

  /*
   * If a filename is passed then open it.
   */
  if (filename.size() > 0) {
    ofs.open(filename);
    if (!ofs) {
      std::cerr << std::format("Could not open <{}>\n", filename);
      return false;
    }
  }
  
  std::ostream &os((ofs.is_open()) ? ofs : std::cout);

  last_addr = 0;
  for (i=CORE_SIZE-1; i>=0; i--) {
    if (modified[i]) {
      last_addr = i;
      break;
    }
  }

  n = 1024 * 12;
  if (last_addr >= n)
    last_addr = n-1;

  os << "memory_initialization_radix=16;\n";
  os << "memory_initialization_vector=\n";

  k = 0;
  for (i=0; i<=last_addr; i++) {
    instr = core[i];

    if ((i == 0) && (exec_addr != 0)) {
      if ((exec_addr & (~0777)) != 0)
        // Not sector zero
        instr = 0102020; // JMP *'20
      else
        instr = 0002000 | exec_addr; // JMP exec_addr
    } else if ((i == 020) && ((exec_addr & (~0777)) != 0)) {
      instr = exec_addr;
    }

    os << std::format("{:0>4x}{:c}", instr, ((i == last_addr)?';':','));

    k++;
    if ((k >= 8) || (i == last_addr)) {
      os << '\n';
      k = 0;
    } else {
      os << ' ';
    }
  }

  if (ofs.is_open()) {
    ofs.close();
  }

  return true;
}
