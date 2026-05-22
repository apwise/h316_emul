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

#include "mtu.hpp"
#include "iodev.hpp"

using namespace h16;

MTU::MTU(IoToPIntf &p)
  : IoDev(p) {
}

DEFINE_UNEXPECTED_INA(MTU)
DEFINE_UNEXPECTED_SKS(MTU)
DEFINE_UNEXPECTED_OTA(MTU)
DEFINE_UNEXPECTED_OCP(MTU)
DEFINE_NULL_SMK(MTU)
DEFINE_NULL_EVENT(MTU)
DEFINE_UNEXPECTED_DMC(MTU)
DEFINE_NULL_SET_FILENAME(MTU)

DEFINE_STD_NAME(MTU)
