/* Honeywell Series 16 emulator
 *
 * Copyright (C) 2026  Adrian Wise
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
#ifndef _IO_DISPATCH_HPP_
#define _IO_DISPATCH_HPP_

#include <cstdint>
#include <vector>
#include "iodev.hpp"
#include "p_to_io_intf.hpp"

namespace h16 {
  
  class IoDispatch : public IoDev
  {
  public:
    enum class Device {
      DUM = 000, // An entry that will always be the dummy device
      PTR = 001, // Paper-tape reader
      PTP = 002, // Paper-tape punch
      LPT = 003, // Lineprinter
      ASR = 004, // ASR (teletype)
#if ENABLE_SPI
      SPI = 007, // SPI flash controller
#endif
      RTC = 020, // Real-time clock
      PLT = 027, // Incremental plotter

#if ENABLE_UKC
      MTU = 010, // Magnetic tape
      DOT = 030, // DOT display
      FPR = 036, // Fast printer
      PRP = 040, // Papertape reader/punch
      DSK = 041, // Disk
      VIS = 046, // VISTS - don't know what it is...
#endif
      
#if ENABLE_VERIF
      NUL = 037, // Used as an unused I/O device in X16_08T1
      // Devices for verification purposes
      VSM = 075, // Simulation exit code
      VD1 = 076, // DMC verification - channel
      VD2 = 077, // DMC verification - central controller
#endif
    };
  
    IoDispatch(IoToPIntf &p);
    ~IoDispatch();
  
    IoStatus ina(uint16_t instr, int16_t &data);
    IoStatus sks(uint16_t instr);
    IoStatus ota(uint16_t instr, int16_t data);
    void ocp(uint16_t instr);
    void smk(uint16_t mask);

    void dmc(unsigned dmc_dev, int16_t &data, bool erl);
  
    void master_clear_devices();

    void event(Device dev, unsigned reason);

    void set_filename(Device dev, const std::string &filename, int subdevice = 0);
    bool tty_special(char k);

  private:
    std::vector<PToIoIntf *> io_table;
    std::vector<PToIoIntf *> dmc_table;
    const char *name() const;
  };
}
#endif // _IO_DISPATCH_HPP_
