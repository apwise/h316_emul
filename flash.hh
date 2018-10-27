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

#ifndef _FLASH_HH_
#define _FLASH_HH_

#include <cstdint>

class Flash
{
public:
  enum DSIZE { BYTE, HALF };
  
  Flash();
  ~Flash();
  void write(uint8_t data, DSIZE size = BYTE); 
  uint8_t read();
  void abort();
  void dummy(unsigned int cycles);

private:
  friend std::ostream &operator<<(std::ostream &os, const Flash::DSIZE &size);
};

#endif // _FLASH_HH_
