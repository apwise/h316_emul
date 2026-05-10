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

#ifndef _IO_TO_P_INTF_HPP_
#define _IO_TO_P_INTF_HPP_

#include <cstdint>
#include <string>

class PToIoIntf;

class IoToPIntf {

public:

  enum class Level {
    FATAL,
    ERROR,
    WARNING
  };
  
  typedef uint64_t EventTime;

  virtual ~IoToPIntf() = 0;

  virtual void set_interrupt(uint16_t mask) = 0;
  virtual void clear_interrupt(uint16_t mask) = 0;

  /*
   * Request a break
   * 0 = RTC
   * 1-16 = DMC channel
   * 17-20 = DMA channel
   */
  virtual void set_break(unsigned n, bool v) = 0;
  
  virtual void queue(EventTime microseconds, PToIoIntf &device, int reason) = 0;
  virtual void queue_hc(EventTime half_cycle, PToIoIntf &device, int reason) = 0;
  virtual uint64_t get_half_cycles() = 0;
 
  virtual std::string get_file_name(const std::string &device_name,
                                    const std::string &extension,
                                    const std::string &description) = 0;

  virtual void anomaly(Level level, const std::string &message) = 0;
};

#endif // _IO_TO_P_INTF_HPP_
