/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 1999, 2005, 2010, 2011, 2018, 2026  Adrian Wise
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
 * This file constitutes the main simulation kernel of the processor.
 */

#include "cpu_rtl.hpp"

using namespace h16;

CpuRtl::CpuRtl(bool hasEa)
  : CPU(hasEa)
  , drlin(false)
  , inb(0)
{
  master_clear();
  drlin = false;
}

CpuRtl::~CpuRtl()
{
}

void CpuRtl::master_clear() {
  CPU::master_clear();
}

/*
 * ===================================================================================
 *
 * Interface from CPU to call I/O devices
 */
IoStatus CpuRtl::ina(uint16_t instr, int16_t &data) {
  IoStatus r = IoStatus::WAIT;
  if (drlin) {
    data = inb;
    r = IoStatus::READY;
  }
  return r;
}

IoStatus CpuRtl::sks(uint16_t instr) {
  return (drlin) ? IoStatus::READY : IoStatus::WAIT;
}

IoStatus CpuRtl::ota(uint16_t instr, int16_t data) {
  return (drlin) ? IoStatus::READY : IoStatus::WAIT;
}

void CpuRtl::ocp(uint16_t instr) {
}

void CpuRtl::smk(uint16_t mask) {
}

void CpuRtl::event(IoDevice dev, int reason) {
}

void CpuRtl::dmc(unsigned dmc_dev, int16_t &data, bool erl) {
}

bool CpuRtl::jump_time_to_event(uint64_t &half_cycles) {
  return false;
}

void CpuRtl::io_polling(uint16_t instr) {
}

/*
 * ===================================================================================
 */
