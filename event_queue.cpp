/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 2005, 2026  Adrian Wise
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
#include "event_queue.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>

#include "p_to_io_intf.hpp"
#include "io_to_p_intf.hpp"

EventQueue::EventQueue(IoToPIntf &p)
  : p(p)
{
}

void EventQueue::queue(uint64_t event_time,
                       PToIoIntf &device,
                       int reason)
{
  Event ev(device, reason);
  
  event_queue.insert(event_queue_t::value_type(event_time, ev));
}

bool EventQueue::call_devices(uint64_t event_time)
{
  unsigned int event_count = 0;
  bool r = false;

  event_queue_t::iterator it = event_queue.begin();

  while ((it != event_queue.end()) && (it->first <= event_time)) {

    r = true;
    
    Event &ev(it->second);

    ev.first.event(ev.second);
      
    it = event_queue.erase(it);
    
    if (++event_count > MAXIMUM_EVENTS) {
      std::stringstream ss;
      ss << "More than " << MAXIMUM_EVENTS << " events at time " << event_time
         << ", infinite loop?" << std::endl;
      p.anomaly(IoToPIntf::Level::FATAL, ss.str());
    }
  }
  
  return r;
}

void EventQueue::flush_events(uint64_t &event_time)
{
  while (!event_queue.empty()) {
    event_time = event_queue.begin()->first;
    (void) call_devices(event_time);
  }
}

void EventQueue::discard_events()
{
  event_queue.clear();
}

bool EventQueue::next_event_time(uint64_t &event_time)
{
  bool r = false;
  if (!event_queue.empty()) {
    event_time = event_queue.begin()->first;
    r = true;
  }
  return r;
}
