/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 1999, 2004, 2005, 2026  Adrian Wise
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
#include "ptp.hpp"

//#include <cstdlib>
//#include <cstdio>
//#include <cstring>
#include <sstream>

#include "iodev.hpp"
#include "stdtty.hpp"
#include "proc.hpp"

#define SPEED 110 // characters per second
#define SMK_MASK (1 << (16-10))

PTP::PTP(IoToPIntf &p)
  : IoDev(p)
{
  master_clear();
}

void PTP::master_clear()
{
  mask = 0;

  tty_file.close();
  filename.clear();

  power_on = false;
  ready = false;
};

PTP::Status PTP::ina(uint16_t instr, int16_t &data)
{
  p.anomaly(IoToPIntf::Level::ERROR, message(instr, "Input from punch"));
  return Status::WAIT;
}

PTP::Status PTP::ota(uint16_t instr, int16_t data)
{
  bool r = false;
  
  switch(instr & 0700) {
  case 0000:
    if (!power_on)
      turn_power_on();
    
    r = ready;
    
    if (ready) {
      tty_file.putc(data & 0xff);
      
      ready = false;
      
      p.queue((1000000 / SPEED), *this, Event::CHARACTER );
    }
    break;
    
  default:
    p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  }
  
  return status(r);
}

void PTP::turn_power_on()
{
  std::string str;
  bool ascii_file = false;
  
  while (!tty_file.is_open()) {
    
    if (filename.size()) {
      str = filename;
    } else {
      str = p.get_file_name("PTP", "ptp", "");
    }

    if (str[0]=='&') {
      ascii_file = true;
      str = str.substr(1);
    }
    
    tty_file.open(str.c_str(), ((ascii_file) ? TTY_file::WRITE_ASCII : TTY_file::WRITE_BINARY));

    if (!tty_file.is_open()) {
      std::stringstream ss;
      ss << "Could not open <" << str << "> for writing";

      IoToPIntf::Level level = ((filename.size()) ? IoToPIntf::Level::FATAL : IoToPIntf::Level::ERROR);
      
      p.anomaly(level, ss.str());
    }

    filename.clear();
  }

  if (!power_on) {
    // 5 second turn-on delay
    p.queue(5000000, *this, Event::CHARACTER );
    power_on = true;
  }   
}

PTP::Status PTP::sks(uint16_t instr)
{
  bool r = 0;
  
  switch(instr & 0700) {
  case 0000: r = ready; break;
  case 0100: r = power_on; break;
  case 0400: r = !(ready && mask); break;
    
  default:
    p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  }
  
  return status(r);
}

void PTP::ocp(uint16_t instr)
{
  switch(instr & 0700) {
  case 0000:
    turn_power_on();
    break;
    
  case 0100:
    power_on = false;
    
    /*
     * Don't close the file; some programs (like the
     * assembler) stop the punch, and then restart
     * it. However, fflush() it to disk.
     */

    tty_file.flush();
    
    break;
    
  default:
    p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  }
}

void PTP::smk(uint16_t mask)
{
  this->mask = mask & SMK_MASK;
  
  if (ready && this->mask)
    p.set_interrupt(this->mask);
  else
    p.clear_interrupt(SMK_MASK);
}

void PTP::event(int reason)
{
  const Event event {static_cast<Event>(reason)};

  switch(event) {
  case Event::MASTER_CLEAR:
    master_clear();
    break;
    
  case Event::CHARACTER:
    ready = true;
    break;
    
  default:
    p.anomaly(IoToPIntf::Level::FATAL, uxReason(reason));
    break;
  }
}

void PTP::set_filename(const std::string &filename, unsigned subdevice) {
  master_clear();
  this->filename = filename;
}

DEFINE_UNEXPECTED_DMC(PTP)

DEFINE_STD_NAME(PTP)
