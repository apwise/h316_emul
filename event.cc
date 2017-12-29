/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005  Adrian Wise
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
#include <cstdlib>
#include <iostream>

#include "stdtty.hh"
#include "proc.hh"
#include "iodev.hh"
#include "event.hh"

Event::event_queue_t Event::event_queue;

Event::Event(IODEV *device, int reason)
  : device(device)
  , reason(reason)
{
};

Event::~Event()
{

};

void Event::call_one_device()
{
  device->event(reason);
}

void Event::initialize()
{
  event_queue.clear();
}

#define cycle_time (1.600)
#define half_cycles_per_microsecond (2.0 / cycle_time)

void Event::queue(Proc *p, unsigned long microseconds, IODEV *device, int reason)
{
  unsigned long half_cycles;
  
  half_cycles = (unsigned long) (((double) microseconds) * 
                                 half_cycles_per_microsecond);
  
  Event ev(device, reason);
  
  event_queue.insert(event_queue_t::value_type(half_cycles + p->get_half_cycles(), ev));
}

bool Event::call_devices(unsigned long long &half_cycles, bool check_time)
{
  unsigned int event_count = 0;
  bool r = false;

  event_queue_t::iterator it = event_queue.begin();

  while ((it != event_queue.end()) &&
         ((!check_time) || (it->first <= half_cycles))) {

    r = true;
    
    if ((!check_time) && (half_cycles != it->first)) {
      event_count = 0;
      half_cycles = it->first;
    }

    Event &ev(it->second);

    it = event_queue.erase(it);
    ev.call_one_device();
    
    event_count ++;
    if (event_count > MAXIMUM_EVENTS) {
      std::cerr << "More than " << MAXIMUM_EVENTS << " events at time " << half_cycles
                << ", infinite loop?" << std::endl;
      std::exit(1);
    }
  }
  
  return r;
}

void Event::flush_events(unsigned long long &half_cycles)
{
  call_devices(half_cycles, false);
}

void Event::discard_events()
{
  event_queue.clear();
}

bool Event::next_event_time(unsigned long long &half_cycles)
{
  event_queue_t::iterator it = event_queue.begin();
  if (it != event_queue.end()) {
    half_cycles = it->first;
    return true;
  }
  return false;
}

