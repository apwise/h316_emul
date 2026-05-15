/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 2005, 2010, 2011, 2026  Adrian Wise
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
 */

#ifndef _PROC_HPP_
#define _PROC_HPP_

#include "proc.hpp"

#include <vector>
#include <cstdint>

#include "cpu.hpp"
#include "instr.hpp"
#include "event_queue.hpp"
#include "io_to_p_intf.hpp"
#include "io_dispatch.hpp"
#include "mfm.hpp"

namespace h16 {

  class IoDev;

  class Proc : public CPU, public IoToPIntf {
  public:
    enum class DI { // do_instr return codes
      OK,   // No problems
      HALT, // Halt instruction cleared runff
      EXIT, // exit() has been called
      MON,  // Request to (re)enter the monitor
      FNAM, // Device requesting filename
    };
    
    Proc(bool HasEa);
    virtual ~Proc();

    DI do_instr();
 
    void exit(int code);
    // TODO change this to get_exit_code()
    //bool get_exit_called(int &code){code = exit_code; return exit_called;}
    bool get_exit_code(){return exit_code;}

    void start_button();
    void goto_monitor() { goto_monitor_flag = true; }
    void set_limit(uint64_t half_cycles);
    void queue_sbi(uint64_t half_cycles);
  
    std::string dis();
    void flush_events();
  
    void set_filename(IoDispatch::Device dev, const std::string &filename, int subdevice = 0); 
    void send_event(IoDispatch::Device dev, unsigned reason);

    bool special(char k);

    /* IoToPIntf */
    void set_interrupt(uint16_t mask) { CPU::set_interrupt(mask); }
    void clear_interrupt(uint16_t mask) { CPU::clear_interrupt(mask); }
    void set_break(unsigned n, bool v) { CPU::set_break(n, v); }
    void queue(uint64_t microseconds, PToIoIntf &device, int reason) {
      uint64_t half_cycles = (half_cycles_per_microsecond * microseconds);
      queue_hc(half_cycles, device, reason);
    }
    void queue_hc(uint64_t half_cycles, PToIoIntf &device, int reason)
    {
      uint64_t event_time = get_half_cycles() + half_cycles;
      event_queue.queue(event_time, device, reason);
    }
    uint64_t get_half_cycles() {
      return CPU::get_half_cycles();
    }

    // Events for the Processor itself - called from MFM::event()
    void event(int reason);
  
    std::string get_file_name(const std::string &device_name,
                              const std::string &extension,
                              const std::string &description);

    virtual void anomaly(Level level, const std::string &message);
    /* end IoToPIntf */

    virtual void master_clear();

    virtual IoStatus ina(uint16_t instr, int16_t &data);
    virtual IoStatus sks(uint16_t instr);
    virtual IoStatus ota(uint16_t instr, int16_t data);
    virtual void ocp(uint16_t instr);
    virtual void smk(uint16_t mask);
    virtual void event(IoDevice dev, int reason);
    virtual bool jump_time_to_event(uint64_t &half_cycles);
    virtual void io_polling(uint16_t instr);
    virtual void dmc(unsigned dmc_dev, // 0 to 15
                     int16_t &data, bool erl);

    bool dump_trace(const std::string &filename, unsigned n);
    bool dump_disassemble(const std::string &filename, unsigned first, unsigned last);
    bool dump_vmem(const std::string &, unsigned exec_addr, bool octal=false);
    bool dump_coemem(const std::string &, unsigned exec_addr);

    bool is_booting() const;
    std::string &get_device_name() {return device_name;}
    std::string &get_extension() {return extension;}
    std::string &get_description() {return description;}
    void put_filename(const std::string &fn) {
      filename = fn;
      filename_valid = true;
    }

  private:
    Mfm *mfm;

    bool goto_monitor_flag;
    int exit_code;
    bool exit_called;
  
    /*
     * Instruction decode & dispatch
     */
    InstrTable instr_table;

    /*
     * Devices dispatch
     */
    IoDispatch ioDispatch;
    EventQueue event_queue;
    static constexpr double cycle_time = 1.60; // Microseconds
    static constexpr double half_cycles_per_microsecond = (2.0 / cycle_time);

    /*
     * get_filename() variables
     *
     * Save values from the caller.
     */
    std::string device_name;
    std::string extension;
    std::string description;

    bool filename_valid;
    std::string filename; // returned filename
  };
}
#endif // _PROC_HPP_
