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

#include "vis.hpp"
#include "iodev.hpp"

using namespace h16;

VIS::VIS(IoToPIntf &p)
  : IoDev(p) {
}

DEFINE_UNEXPECTED_INA(VIS)
DEFINE_UNEXPECTED_SKS(VIS)
DEFINE_UNEXPECTED_OTA(VIS)
DEFINE_UNEXPECTED_OCP(VIS)
DEFINE_NULL_SMK(VIS)
DEFINE_NULL_EVENT(VIS)
DEFINE_UNEXPECTED_DMC(VIS)
DEFINE_NULL_SET_FILENAME(VIS)

DEFINE_STD_NAME(VIS)
