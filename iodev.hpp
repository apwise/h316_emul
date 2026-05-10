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

namespace h16 {
  
class IoDev
{
public:
  virtual const char *name() const = 0;
  
protected:
  IoToPIntf &p;

  IoDev(IoToPIntf &p)
    : p(p) {
  }
  virtual ~IoDev() {
  }

  static uint16_t device_addr(uint16_t instr) {
    return instr & 077;
  }

  static uint16_t function_code(uint16_t instr) {
    return (instr >> 6) & 017;
  }

  static IoStatus status(bool b) {
    return (b) ? IoStatus::READY : IoStatus::WAIT;
  };

  std::string message(const std::string &text);
  std::string message(uint16_t instr, const std::string &additional="");
  std::string uxReason(int n);
  
};
}
#define DEFINE_STD_NAME(ClassName) const char *h16::ClassName::name() const { return #ClassName; }


#endif //_IODEV_HPP_
