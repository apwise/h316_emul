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
{
}

Flash::~Flash()
{
}

void Flash::write(uint8_t data, DSIZE size)
{
  //cout << __PRETTY_FUNCTION__
  //  << " data = " << hex << setw(2) << setfill('0') << static_cast<int>(data)
  //  << " size = " << dec << size << endl;
}

uint8_t Flash::read()
{
  //cout << __PRETTY_FUNCTION__ << endl;
  return 0;
}

void Flash::abort()
{
}
  
void Flash::dummy(unsigned int cycles)
{
}
