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

#include "prp.hpp"
#include "iodev.hpp"

using namespace h16;

PRP::PRP(IoToPIntf &p)
  : IoDev(p) {
}

DEFINE_UNEXPECTED_INA(PRP)

IoStatus PRP::sks(uint16_t instr)
{
  bool r = false;
  
  switch(instr & 0700) {

  case 0100: r = true; break; // Assumed to be skip if reader not interrupting
  case 0300: r = true; break; // Assumed to be skip if punch not interrupting
    
  default:
    p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  }
  
  return status(r);
}

DEFINE_UNEXPECTED_OTA(PRP)

void PRP::ocp(uint16_t instr)
{
  // Silently ignore all OCP
}

DEFINE_NULL_SMK(PRP)
DEFINE_NULL_EVENT(PRP)
DEFINE_UNEXPECTED_DMC(PRP)
DEFINE_NULL_SET_FILENAME(PRP)

DEFINE_STD_NAME(PRP)
