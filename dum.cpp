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

#include "dum.hpp"
#include "iodev.hpp"

const char *Dum::name() {
  return "DUM";
}

Dum::Dum(IoToPIntf &p)
  : IoDev(p) {
}

PToIoIntf::Status Dum::ina(uint16_t instr, int16_t &data) {
  p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  return Status::WAIT;
}

PToIoIntf::Status Dum::sks(uint16_t instr) {
  p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  return Status::WAIT;
}

PToIoIntf::Status Dum::ota(uint16_t instr, int16_t data) {
  p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  return Status::WAIT;
}

void Dum::ocp(uint16_t instr) {
  p.anomaly(IoToPIntf::Level::ERROR, message(instr));
}

void Dum::smk(uint16_t mask) {
  // Just ignore it
}

void Dum::event(int reason) {
  if (reason != EVENT_MASTER_CLEAR) {
    p.anomaly(IoToPIntf::Level::FATAL, uxReason(reason));
  }
}

void Dum::set_filename(const std::string &filename, unsigned subdevice) {
  p.anomaly(IoToPIntf::Level::FATAL, message("set_filename()"));
}

void Dum::dmc(unsigned dmc_dev, int16_t &data, bool erl) {
  p.anomaly(IoToPIntf::Level::FATAL, message("Unexpected DMC"));
}
