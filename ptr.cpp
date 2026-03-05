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
#include "ptr.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <iostream>
#include <iomanip>

#include "iodev.hpp"
#include "stdtty.hpp"
#include "proc.hpp"

#define SPEED 1000 // charcters per second
#define SMK_MASK (1 << (16-9))

#define EOT_LIMIT 1000

/*
 * The file is opened by the first time (since master clear)
 * that the reader is started.
 *
 * Another filename will not be requested until either a
 * master-clear occurs, or that the reader is started and the
 * tape is at EOT (i.e. at EOF).
 *
 *
 * In the case of an ASCII file all non-zero characters read
 * from the file have the MSB forced to one. A '\n' read from
 * the file is translated into a CR-LF sequence.
 *
 */

PTR::PTR(IoToPIntf &p)
  : IoDev(p)
{
  master_clear();
}

void PTR::master_clear()
{
  mask = 0;

  tty_file.close();

  eot = false;
  eot_counter = 0;

  tape_running = false;
  ready = false;
  
  ignore_event = false;
  events_queued = 0;

  data_count = 0;
};

PTR::Status PTR::ina(uint16_t instr, int16_t &data)
{
  bool r = ready;

  if (ready) {
    data = data_buf;
    ready = false;
    p.clear_interrupt(SMK_MASK);
    data_count ++;
    //printf("\nPTR: Read character (%d) %d\n", data_buf, data_count);
  } else if (eot) {
    eot_counter++;
    
    if (eot_counter > EOT_LIMIT) {
      /*
       * This is a kludge, however...
       *
       * Some programs (well the assembler in particular)
       * don't read to the end of the tape. This is quite
       * normal; most programs should be able to tell they
       * are at the end of valid input without actually
       * running out of tape. However, the assembler needs
       * to get back to the start of the source code in order
       * to do the second pass. On the real machine, this
       * is simply done by taking the tape out of the
       * reader, rewinding it, and putting it back at the
       * start of the tape. Master clear would cause the
       * PTR to ask for the name of the file again, but then
       * the PC is lost, and the second pass cannot be easily
       * started.
       *
       * In order to deal with this, after an arbitrary
       * number of attempts to read after the EOT, close
       * the file and ask for the name of a new file.
       */
      printf("PTR: EOT\n");
      tty_file.close();
      open_file();
      start_reader();
    }
  }
  
  return status(r);
}

void PTR::open_file(void)
{
  std::string str;
  bool ascii_file = false;
  
  while (!tty_file.is_open()) {
    if (filename.size() != 0) {
      str = filename;
    } else {
      str = p.get_file_name("PTR", "ptp", "");
    }
    
    if (str[0]=='&') {
      ascii_file = true;
      str = str.substr(1);
    }
    
    tty_file.open(str.c_str(), ((ascii_file) ? TTY_file::READ_ASCII : TTY_file::READ_BINARY));
    
    if (!tty_file.is_open()) {
      std::stringstream ss;
      ss << "Could not open <" << str << "> for reading";

      IoToPIntf::Level level = ((filename.size()) ? IoToPIntf::Level::FATAL : IoToPIntf::Level::ERROR);
      
      p.anomaly(level, ss.str());
    }

    filename.clear();
  }
  
  eot = false;
  eot_counter = 0;
}

void PTR::start_reader(void)
{
  p.queue((1000000 / SPEED), *this, Event::CHARACTER );
  events_queued++;
  tape_running = true;
}

void PTR::ocp(uint16_t instr)
{
  switch(instr & 0700) {
  case 0000:
    
    if (eot) {
      tty_file.close();
    }
    
    if (! tty_file.is_open())
      open_file();
    
    if (!tape_running)
      start_reader();
    
    break;
    
  case 0100:
    
    tape_running = false;
    if (events_queued > 0)
      ignore_event = true;
    /*
     * The first event should be ignored
     * because the tape has been stopped, it is assumed
     * that the tape is stopped early enough to avoid
     * losing characters.
     */
    break;
    
  default:
    p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  }
}

PTR::Status PTR::sks(uint16_t instr)
{
  bool r = 0;
  
  switch(instr & 0700) {
  case 0000: r = ready; break;
  case 0400: r = !(ready && mask); break;
    
  default:
    p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  }
  
  return status(r);
}

void PTR::smk(uint16_t mask)
{
  this->mask = mask & SMK_MASK;
  
  if (ready && this->mask)
    p.set_interrupt(this->mask);
  else
    p.clear_interrupt(SMK_MASK);
}

void PTR::event(int reason)
{
  const Event event {static_cast<Event>(reason)};

  switch(event) {
  case Event::MASTER_CLEAR:
    master_clear();
    break;
    
  case Event::CHARACTER:
    events_queued--;
    
    if (!ignore_event) {
      if (ready) {
        std::stringstream ss;
        ss << std::dec << p.get_half_cycles() << " Character overrun";
        p.anomaly(IoToPIntf::Level::WARNING, message(ss.str()));
      }
      
      int c = tty_file.getc();

      if (c == EOF) {
        eot = true;
        eot_counter = 0;
      } else {
        data_buf = c & 0xff;
        ready = true;
        p.set_interrupt(mask);
        p.queue((1000000 / SPEED), *this, Event::CHARACTER );
        events_queued++;
      }
    }
    
    ignore_event = false;
    break;
    
  default:
    p.anomaly(IoToPIntf::Level::FATAL, uxReason(reason));
    break;
  }
}

void PTR::set_filename(const std::string &filename, unsigned subdevice) {
  tty_file.close();

  eot = false;
  eot_counter = 0;

  tape_running = false;
  ready = false;

  this->filename = filename;
}

DEFINE_UNEXPECTED_OTA(PTR)
DEFINE_UNEXPECTED_DMC(PTR)

DEFINE_STD_NAME(PTR)
