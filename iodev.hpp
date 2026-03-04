/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 2005, 2018, 2026  Adrian Wise
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
#ifndef _IODEV_HPP_
#define _IODEV_HPP_

#include <cstdint>
#include <string>

#include "config.h"
#include "p_to_io_intf.hpp"
#include "io_to_p_intf.hpp"

class IoDev
{
public:
  enum class Device {
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

protected:
  IoToPIntf &p;

  IoDev(IoToPIntf &p)
    : p(p) {
  }
  virtual ~IoDev() {
  }

  virtual const char *name() = 0;

  static uint16_t device_addr(uint16_t instr) {
    return instr & 077;
  }

  static uint16_t function_code(uint16_t instr) {
    return (instr >> 6) & 017;
  }

  static PToIoIntf::Status status(bool b) {
    return (b) ? PToIoIntf::Status::READY : PToIoIntf::Status::WAIT;
  };

  std::string message(const std::string &text);
  std::string message(uint16_t instr, const std::string &additional="");
  std::string uxReason(int n);
  
};

#define DEF_STD_NAME(ClassName) const char *ClassName::name() { return #ClassName; }


#endif //_IODEV_HPP_
