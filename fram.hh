/* Honeywell Series 16 emulator
 * Copyright (C) 2024  Adrian Wise
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
#ifndef _FRAM_HH_
#define _FRAM_HH_

#include "spi_dev.hh" // provides <deque> and <cstdint>

class FRAM : public SpiDev {

public:
  FRAM(unsigned log2size = 19, const char *filename = 0);
  virtual ~FRAM();
  virtual void write(bool wprot, std::deque<uint8_t> &command);
  virtual uint8_t read();

private:
  static const char *default_filename;

  const unsigned log2size;
  const unsigned addr_mask;
  const char * const filename;
  
  enum class Cmnd {
    WRITE      = 0x02,
    READ       = 0x03,
    WRDI       = 0x04,
    WREN       = 0x06,
    FAST_READ  = 0x0b,
    QIW        = 0x32,
    DOR        = 0x3b,
    QOR        = 0x6b,
    WRAR       = 0x71,
    DIOW       = 0xa1,
    DIW        = 0xa2,
    DIOR       = 0xbb,
    QIOW       = 0xd2,
    FAST_WRITE = 0xda,
    QIOR       = 0xeb
  };
  
  void addr(std::deque<uint8_t> &bytes);
  void mode(std::deque<uint8_t> &command, Cmnd cmnd, bool half = true);
  void write_any_register(uint8_t data);
  void write_bytes(std::deque<uint8_t> &data);
  void check_file();

  uint8_t *fram; // Pointer to storage (an mmap'd file)
  bool WEL;
  unsigned address;
  bool fast;
  Cmnd fast_cmnd;
};

#endif // _FRAM_HH_
