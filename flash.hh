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
#include <fstream>
#include <string>

class Flash
{
public:
  enum DSIZE { BYTE, HALF };
  
  Flash();
  ~Flash();
  void write(uint8_t data, DSIZE size = BYTE); 
  uint8_t read();
  void deselect();
  void dummy(unsigned int cycles);

private:
  friend std::ostream &operator<<(std::ostream &os, const Flash::DSIZE &size);

  enum FLASH_STATE {
    STATE_CMND,
    STATE_ADR1,
    STATE_ADR2,
    STATE_ADR3,
    STATE_ADR4,
    STATE_DATA
  };
  
  FLASH_STATE state;
  std::uint8_t cmnd;
  std::uint32_t addr;
  
  std::fstream fs;
  std::string filename;
  std::streampos filesize;
  
  void file_open();

  static constexpr const char *DEFAULT_FILENAME = "flash.img";
};

#endif // _FLASH_HH_
