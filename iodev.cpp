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

#include "config.h"
#include "iodev.hpp"
#include <sstream>
#include <iomanip>

std::string IoDev::message(const std::string &text) {
  std::stringstream ss;
  ss << name() << ": " << text;
  return ss.str();
}

std::string IoDev::message(uint16_t instr, const std::string &additional) {

  std::string mnemonic;
  switch ((instr >> 10) & 0x3f) {
  case 014: mnemonic = "OCP" ; break;
  case 034: mnemonic = "SKS" ; break;
  case 054: mnemonic = "INA" ; break;
  case 074: mnemonic = "OTA" ; break;
  default: /* Do nothing */ break;
  }
  
  std::stringstream ss;
  ss << name() << ": ";
  if (mnemonic.size()) {
    ss << mnemonic << " '" << std::oct << std::setw(4) << std::setfill('0')
       << (instr & 0x3ff) << std::dec << ": ";
  }

  ss << additional;
  return ss.str();
}

std::string IoDev::uxReason(int n) {
  std::stringstream ss;
  ss << name() << ": Unexpected event reason (" << std::dec << n << ")";
  return ss.str();
}
