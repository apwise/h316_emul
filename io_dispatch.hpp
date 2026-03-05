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

class IoDispatch : public IoDev
{
public:
  enum class DEVICE {
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
    
#if ENABLE_VERIF
    // Devices for verification purposes
    VSM = 035, // Simulation exit code
    VD1 = 036, // DMC verification - channel
    VD2 = 037, // DMC verification - central controller
#endif
  };
  
  IoDispatch(IoToPIntf &p);
  ~IoDispatch();
  
  PToIoIntf::Status ina(uint16_t instr, int16_t &data);
  PToIoIntf::Status sks(uint16_t instr);
  PToIoIntf::Status ota(uint16_t instr, int16_t data);
  void ocp(uint16_t instr);
  void smk(uint16_t mask);

  void dmc(unsigned dmc_dev, int16_t &data, bool erl);
  
  void master_clear_devices();

  void event(DEVICE dev, unsigned reason);

  void set_filename(DEVICE dev, const std::string &filename, int subdevice = 0); 

private:
  std::vector<PToIoIntf *> io_table;
  std::vector<PToIoIntf *> dmc_table;
  const char *name();
};


#endif // _IO_DISPATCH_HPP_
