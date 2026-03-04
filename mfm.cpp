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
 *
 */

#include "mfm.hpp"
#include "iodev.hpp"
#include "proc.hpp"

const char *Mfm::name() {
  return "MFM";
}

Mfm::Mfm(IoToPIntf &p)
  : IoDev(p) {
}

PToIoIntf::Status Mfm::ina(uint16_t instr, int16_t &data) {
  p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  return Status::WAIT;
}

void Mfm::ocp(uint16_t instr) {
  p.anomaly(IoToPIntf::Level::ERROR, message(instr));
}

PToIoIntf::Status Mfm::sks(uint16_t instr) {
  p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  return Status::WAIT;
}

PToIoIntf::Status Mfm::ota(uint16_t instr, int16_t data) {
  p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  return Status::WAIT;
}

void Mfm::smk(uint16_t mask) {
  p.anomaly(IoToPIntf::Level::ERROR, message("SMK"));
}

void Mfm::event(int reason) {
  Proc *proc = dynamic_cast<Proc *>(&p);
  if (proc) {
    proc->event(reason);
  } else {
    p.anomaly(IoToPIntf::Level::FATAL, uxReason(reason));
  }
}

