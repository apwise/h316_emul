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
#ifndef _P_TO_IO_INTF_HPP_
#define _P_TO_IO_INTF_HPP_

#include <cstdint>
#include <string>

class PToIoIntf
{
public:
  
  enum class Status {
    WAIT,  // I/O not yet ready
    READY, // I/O ready (for INA, OTA, SKS)
  };

  static const int EVENT_MASTER_CLEAR = -1;

  virtual ~PToIoIntf() = 0;

  virtual Status ina(uint16_t instr, int16_t &data) = 0;
  virtual Status sks(uint16_t instr) = 0;
  virtual Status ota(uint16_t instr, int16_t data) = 0;
  virtual void ocp(uint16_t instr) = 0;
  virtual void smk(uint16_t mask) = 0;
  
  virtual void event(int reason) = 0;

  virtual void set_filename(const std::string &filename, unsigned subdevice) = 0; 

  virtual void dmc(unsigned dmc_dev, // 0 to 15
                   int16_t &data, bool erl) = 0;
};

#define DEFINE_UNEXPECTED_INA(ClassName) PToIoIntf::Status ClassName::ina(uint16_t instr, int16_t &data) { \
  p.anomaly(IoToPIntf::Level::ERROR, message(instr)); \
  return Status::WAIT; \
}

#define DEFINE_UNEXPECTED_SKS(ClassName) PToIoIntf::Status ClassName::sks(uint16_t instr) { \
  p.anomaly(IoToPIntf::Level::ERROR, message(instr)); \
  return Status::WAIT; \
}

#define DEFINE_UNEXPECTED_OTA(ClassName) PToIoIntf::Status ClassName::ota(uint16_t instr, int16_t data) { \
  p.anomaly(IoToPIntf::Level::ERROR, message(instr)); \
  return Status::WAIT; \
}

#define DEFINE_UNEXPECTED_OCP(ClassName) void ClassName::ocp(uint16_t instr) { \
  p.anomaly(IoToPIntf::Level::ERROR, message(instr)); \
}

#define DEFINE_NULL_SMK(ClassName) void ClassName::smk(uint16_t mask) { }

#define DEFINE_NULL_EVENT(ClassName) void ClassName::event(int reason) { \
  if (reason != EVENT_MASTER_CLEAR) { \
    p.anomaly(IoToPIntf::Level::FATAL, uxReason(reason)); \
  } \
}

#define DEFINE_UNEXPECTED_DMC(ClassName) void ClassName::dmc(unsigned dmc_dev, int16_t &data, bool erl) { \
  p.anomaly(IoToPIntf::Level::ERROR, message("Unexpected DMC")); \
}

#define DEFINE_NULL_SET_FILENAME(ClassName) void ClassName::set_filename(const std::string &filename, unsigned subdevice) {}

#endif // _P_TO_IO_INTF_HPP_
