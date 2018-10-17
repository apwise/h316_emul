/* Honeywell Series 16 emulator
 * Copyright (C) 2018  Adrian Wise
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

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "iodev.hh"
#include "stdtty.hh"

#include "proc.hh"
#include "spi.hh"

using namespace std;

SPI::SPI(Proc *p,
         unsigned int smk_bit,
         unsigned int pwr_dly,
         unsigned int dmc_chn,
         unsigned int boot_cmnd,
         unsigned int boot_ctrl,
         unsigned int boot_addr
         )
  : IODEV(p)
  , smk_mask(1 << (16-smk_bit))
  , pwr_dly(pwr_dly)
  , dmc_chn(dmc_chn)
  , boot_cmnd(boot_cmnd)
  , boot_ctrl(boot_ctrl)
  , boot_addr(boot_addr)
{
  master_clear();
}

SPI::~SPI()
{
}

void SPI::master_clear()
{
  intr_mask = false;
}

SPI::STATUS SPI::ina(unsigned short instr, signed short &data)
{
  return STATUS_WAIT;
}

SPI::STATUS SPI::ocp(unsigned short instr)
{
  return STATUS_WAIT;
}

SPI::STATUS SPI::sks(unsigned short instr)
{
  return STATUS_WAIT;
}

SPI::STATUS SPI::ota(unsigned short instr, signed short data)
{
  return STATUS_WAIT;
}

SPI::STATUS SPI::smk(unsigned short mask)
{
  return STATUS_WAIT;
}

void SPI::dmc(signed short &data, bool erl)
{
}

void SPI::event(int reason)
{
}
