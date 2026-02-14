/* Honeywell Series 16 emulator
 * Copyright (C) 2024, 2026  Adrian Wise
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

#ifndef _SPI_DEV_HPP_
#define _SPI_DEV_HPP_

#include <cstdint>
#include <deque>

class SpiDev {

public:
  virtual void write(bool wprot, std::deque<uint8_t> &command) = 0;
  virtual uint8_t read() = 0;

private:
  
};

#endif // _SPI_DEV_HPP_
