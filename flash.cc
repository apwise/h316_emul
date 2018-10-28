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
 */

#include <iostream>
#include <iomanip>
#include "flash.hh"

using namespace std;

ostream &operator<<(std::ostream &os, const Flash::DSIZE &size)
{
  const char *const s = 
    ((size == Flash::BYTE) ? "BYTE" :
     (size == Flash::HALF) ? "HALF" : "????" );
  os << s;
  return os;
}

Flash::Flash()
  : state(STATE_CMND)
  , filename(DEFAULT_FILENAME)
{
}

Flash::~Flash()
{
}

void Flash::write(uint8_t data, DSIZE size)
{
  bool allow_half = false;
  
  //cout << __PRETTY_FUNCTION__
  //  << " data = " << hex << setw(2) << setfill('0') << static_cast<int>(data)
  //     << " size = " << dec << size << endl;

  //cout << "state = " << static_cast<int>(state) << endl;
  switch (state) {
  case STATE_CMND:
    cmnd = data;
    switch (cmnd) {
    case 0x0c:
      addr = 0;
      state = STATE_ADR1;
      break;
      
    default:
      cerr << "Unexpected command = "
        << hex << setw(2) << setfill('0') << static_cast<int>(data) << endl;
      exit(1);
    }

    file_open();
    break;

  case STATE_ADR1:
    addr = (addr & 0x00ffffff) | (static_cast<uint32_t>(data) << 24);
    state = STATE_ADR2;
    break;
    
  case STATE_ADR2:
    addr = (addr & 0xff00ffff) | (static_cast<uint32_t>(data) << 16);
    state = STATE_ADR3;
    break;
    
  case STATE_ADR3:
    addr = (addr & 0xffff00ff) | (static_cast<uint32_t>(data) <<  8);
    state = STATE_ADR4;
    break;
    
  case STATE_ADR4:
    addr = (addr & 0xffffff00) | static_cast<uint32_t>(data);
    state = STATE_DATA;

    //cout << "addr = " << addr << endl;
    
    if (addr < filesize) {
      fs.seekg(addr, fs.beg);
    } else {
      // Just park the file pointer at EOF
      fs.seekg(0, fs.end);
    }
    break;

  case STATE_DATA:
    break;
    
  default:
    cerr << "Unexpected state = " << static_cast<int>(state) << endl;
    exit(1);
  }

  if ((size == HALF) && (!allow_half)) {
    cerr << "Unexpected half-word write to flash" << endl;
    exit(1);
  }
}

uint8_t Flash::read()
{
  uint8_t data = 0;
  //cout << __PRETTY_FUNCTION__ << endl;
  if (fs.tellg() < filesize) {
    //cout << "tellg = " << static_cast<uint32_t>(fs.tellg()) << endl;
    char c;
    fs.get(c);
    data = c & 0xff;
    //cout << "tellg = " << static_cast<uint32_t>(fs.tellg())
    //     << " data = " << static_cast<uint32_t>(data) << endl;
    addr++;
  }
  return data;
}

void Flash::deselect()
{
  //cout << __PRETTY_FUNCTION__ << endl;
  state = STATE_CMND;
}
  
void Flash::dummy(unsigned int cycles)
{
}

void Flash::file_open()
{
  const ios_base::openmode mode = (ios_base::in    |
                                   ios_base::out   |
                                   ios_base::binary);
  
  if (! fs.is_open()) {
    fs.open(filename.c_str(), mode);
    if (fs.is_open()) {
      fs.seekg (0, fs.end);
      filesize = fs.tellg();
      fs.seekg (0, fs.beg);

      if (filesize == 0) {
        cerr << "File <"
             <<  filename << "> of zero size" << endl;
        exit(1);
      }
      //cout << "filesize = " << static_cast<uint32_t>(filesize) << endl;
      
    } else {
      cerr << "Failed to open file <"
           << filename << ">" << endl;
      exit(1);
    }
  }
}
