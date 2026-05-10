/* Honeywell Series 16 emulator
 *
 * Copyright (C) 2018, 2026  Adrian Wise
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

#include "proc.hpp"
#include "vsim.hpp"

using namespace h16;

VSIM::VSIM(IoToPIntf &p)
  : IoDev(p)
{
}

const char *VSIM::name() const {
  return "VSIM";
}

IoStatus VSIM::ota(uint16_t instr, int16_t data)
{
  // This needs to set the exit code to data
  Proc *proc = dynamic_cast<Proc *>(&p);
  if (proc) {
    proc->exit(data);
  }
  return IoStatus::READY;
}

void VSIM::smk(uint16_t mask) {
  // Just ignore
}

void VSIM::event(int reason) {
  if (reason != EVENT_MASTER_CLEAR) {
    p.anomaly(IoToPIntf::Level::FATAL, uxReason(reason));
  }
}


DEFINE_UNEXPECTED_INA(VSIM)
DEFINE_UNEXPECTED_SKS(VSIM)
DEFINE_UNEXPECTED_OCP(VSIM)
DEFINE_UNEXPECTED_DMC(VSIM)

DEFINE_NULL_SET_FILENAME(VSIM)
