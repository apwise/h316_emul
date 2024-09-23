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
#include "fram.hh"
#include <iostream>
#include <iomanip>
#include <cassert>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>

const char *FRAM::default_filename = "h16_spi_fram.bin";

FRAM::FRAM(unsigned log2size, const char *filename)
  : log2size(log2size)
  , addr_mask((1 << log2size)-1)
  , filename((filename) ? filename : default_filename)
  , fram(0)
  , WEL(false)
  , address(0)
  , fast(false)
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
}

FRAM::~FRAM()
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void FRAM::check_file()
{
  if (fram) return;
  
  int fd=-1;
  bool close_file = true;
  off_t file_size = ((off_t) 1) << log2size;
  
  // Attempt to create the file
  fd = open(filename, (O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC),
            (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));

  if (fd < 0) {
    if (errno == EEXIST) {
      // Attempt to open the existing file
      fd = open(filename, (O_RDWR | O_CLOEXEC), 0);
      if (fd >= 0) {
        // File is opened, is it what was expected?
        struct stat statbuf;
        if (0 == fstat(fd, &statbuf)) {
          if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
            if (statbuf.st_size != file_size) {
              std::cerr << "\nFRAM file \"" << filename
                        << "\" has size " << std::dec << statbuf.st_size
                        << " (expected " << file_size << ")" << std::endl;
            } else {
              close_file = false;
            }
          } else {
            std::cerr << "\n\"" << filename << "\" is not a regular file" << std::endl;
          }
        } else {
          std::cerr << "\nfstat() failed for \"" << filename << "\"" << std::endl;
        }
      }
    }
  } else {
    // File just created - make it the size of the FRAM memory
    int e = ftruncate(fd, file_size);
    if (e == 0) {
      close_file = false;
    } else {
      std::cerr << "\nFailed to truncate \"" << filename << "\"" << std::endl;
    }
  }

  if ((fd >=0) && (!close_file)) {
    // Try to lock the file
    int e = flock(fd, LOCK_EX);
    if (e == 0) {
      void *p = mmap(0, file_size, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, 0);
      if (p == MAP_FAILED) {
        std::cerr << "\nFailed to mmap \"" << filename << "\"" << std::endl;
        close_file = true;
      } else {
        fram = static_cast<uint8_t *>(p);
      }
    } else {
      std::cerr << "Failed to lock \"" << filename << "\"" << std::endl;
      close_file = true;
    }
  }
 
  if (close_file) {
    (void) close(fd);
    fd = -1;
    exit(2);
  }

}

void FRAM::write_bytes(std::deque<uint8_t> &data)
{
  check_file();

  while(! data.empty()) {
    fram[address] = data.front();
    data.pop_front();
    address = (address + 1) & addr_mask;
  }
}

void FRAM::mode(std::deque<uint8_t> &command, Cmnd cmnd, bool half)
{
  uint8_t mask = (half) ? 0xf0 : 0xff;

  fast = ((command.front() & mask) == (0xa5 & mask));
  fast_cmnd = cmnd;
  command.pop_front();
}

void FRAM::write(bool wprot, std::deque<uint8_t> &command)
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;

  assert(!command.empty());

  Cmnd cmnd = (fast) ? fast_cmnd : static_cast<Cmnd>(command.front());
  
  //std::cout << "command.size()=" << command.size() << std::endl;
  
  switch (cmnd) {
  case Cmnd::WRITE:
    assert(!wprot);
    command.pop_front();
    addr(command);
    write_bytes(command);
  break;
    
  case Cmnd::READ:
    command.pop_front();
    addr(command);
  break;

  case Cmnd::WRDI: WEL = false; command.pop_front(); break;

  case Cmnd::WREN: WEL = true;  command.pop_front(); break;

  case Cmnd::FAST_READ:
  case Cmnd::DOR:
  case Cmnd::QOR:
    if (!fast) {
      command.pop_front();
    }
    addr(command);
    mode(command, cmnd);
    break;
    
  case Cmnd::FAST_WRITE:
  case Cmnd::DIW:
  case Cmnd::QIW:
    assert(!wprot);
    if (!fast) {
      command.pop_front();
    }
    addr(command);
    mode(command, cmnd);
    write_bytes(command);
    break;
    
  case Cmnd::DIOW:
  case Cmnd::QIOW:
    if (!fast) {
      command.pop_front();
    }
    addr(command);
    mode(command, cmnd, /* half = */ false);
    write_bytes(command);
    break;
    
  case Cmnd::DIOR:
  case Cmnd::QIOR:
    if (!fast) {
      command.pop_front();
    }
    addr(command);
    mode(command, cmnd, /* half = */ false);
    break;

  case Cmnd::WRAR:
    assert(!wprot);
    command.pop_front();
    addr(command);
    write_any_register(command.front());
    command.pop_front();
    break;
    
  default:
    std::cerr << "Unexpected command: "
              << std::hex << std::setw(2) << std::setfill('0')
              << (unsigned) command.front() << std::endl;
    exit(1);
    break;
  }

}

void FRAM::addr(std::deque<uint8_t> &bytes)
{
  unsigned a = 0;
  assert(bytes.size() >= 3);
  for (int i = 0; i<3; i++) {
    a = (a << 8) | (bytes.front() & 0xff);
    bytes.pop_front();
  }
  address = a;
  //std::cout << "address="
  //            << std::hex << std::setw(6) << std::setfill('0')
  //            << address << std::endl;
  if (address > addr_mask) {
    std::cout << "oops" << std::endl;
    exit(1);
  }
}

void FRAM::write_any_register(uint8_t data)
{
  // Just ignore...
}

uint8_t FRAM::read()
{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;

  uint8_t r;

  check_file();

  r = fram[address];
  address = (address + 1) & addr_mask;
  return r;
}
