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

#include "fpr.hpp"
#include "iodev.hpp"

using namespace h16;

FPR::FPR(IoToPIntf &p)
  : IoDev(p) {
}

DEFINE_UNEXPECTED_INA(FPR)
DEFINE_UNEXPECTED_SKS(FPR)
DEFINE_UNEXPECTED_OTA(FPR)
DEFINE_UNEXPECTED_OCP(FPR)
DEFINE_NULL_SMK(FPR)
DEFINE_NULL_EVENT(FPR)
DEFINE_UNEXPECTED_DMC(FPR)
DEFINE_NULL_SET_FILENAME(FPR)

DEFINE_STD_NAME(FPR)
