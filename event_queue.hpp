/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 2005, 2017, 2026  Adrian Wise
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

#ifndef _EVENT_QUEUE_HPP_
#define _EVENT_QUEUE_HPP_

#include <cstdint>
#include <map>

class IoToPIntf;
class PToIoIntf;

class EventQueue
{
public:
  EventQueue(IoToPIntf &p);

  void queue(uint64_t event_time, PToIoIntf &device, int reason = 0);
  bool call_devices(uint64_t event_time);
  void flush_events(uint64_t &event_time);
  void discard_events();
  bool next_event_time(uint64_t &event_time);
  
private:
  IoToPIntf &p;
  
  typedef std::pair<PToIoIntf &, int> Event;
  typedef std::multimap<uint64_t, Event> event_queue_t;
  event_queue_t event_queue;
  
  static const unsigned int MAXIMUM_EVENTS = 10000;
};

#endif // _EVENT_QUEUE_HPP_
