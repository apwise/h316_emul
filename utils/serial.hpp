/* Honeywell Series 16 emulator
 * Copyright (C) 2006, 2016, 2022, 2026  Adrian Wise
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

#ifndef _SERIAL_HPP_
#define _SERIAL_HPP_

struct termios;

class Serial
{
public:
  Serial(const char *device, unsigned int baud);
  ~Serial();
  int get_fd(){return fd;};
  char receive(bool &ok);
  void transmit(char c, bool &ok);

private:
  int fd;
  struct termios *oldtio, *newtio;
  unsigned int character_time;
};

#endif // _SERIAL_HPP_
