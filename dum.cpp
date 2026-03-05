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

DUM::DUM(IoToPIntf &p)
  : IoDev(p) {
}

DEFINE_UNEXPECTED_INA(DUM)
DEFINE_UNEXPECTED_SKS(DUM)
DEFINE_UNEXPECTED_OTA(DUM)
DEFINE_UNEXPECTED_OCP(DUM)
DEFINE_NULL_SMK(DUM)
DEFINE_NULL_EVENT(DUM)
DEFINE_UNEXPECTED_DMC(DUM)
DEFINE_NULL_SET_FILENAME(DUM)

DEFINE_STD_NAME(DUM)
