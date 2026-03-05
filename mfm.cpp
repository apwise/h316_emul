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

void Mfm::smk(uint16_t mask) {
  p.anomaly(IoToPIntf::Level::ERROR, message("Unexpected SMK"));
}

void Mfm::event(int reason) {
  Proc *proc = dynamic_cast<Proc *>(&p);
  if (proc) {
    proc->event(reason);
  } else {
    p.anomaly(IoToPIntf::Level::FATAL, uxReason(reason));
  }
}

DEFINE_UNEXPECTED_INA(Mfm)
DEFINE_UNEXPECTED_SKS(Mfm)
DEFINE_UNEXPECTED_OTA(Mfm)
DEFINE_UNEXPECTED_OCP(Mfm)
DEFINE_UNEXPECTED_DMC(Mfm)

DEFINE_NULL_SET_FILENAME(Mfm)
