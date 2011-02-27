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
 * $Log: event.hh,v $
 * Revision 1.2  1999/06/04 07:57:21  adrian
 * *** empty log message ***
 *
 * Revision 1.1  1999/02/20 00:06:35  adrian
 * Initial revision
 *
 *
 * This implements a simple event queue.
 * 
 * initialize() must be called to initialize the
 * event queue.
 *
 * queue() adds an event to the queue. The "module"
 * will be called with the reason code "reason"
 * at time "time"
 *
 * Events may be added at any time.
 *
 * Where two events are queued to occur at the same
 * time as one another they are guaranteed to be called
 * in the order that they were added to the queue.
 *
 * call_modules() calls all of the modules that are
 * queued for time "time". An error will occur if there
 * are events in the queue which were due to be called
 * a time prior to "time".
 * Simulation modules may queue events when they are
 * called by call_modules(). They may even queue an
 * event for the current time. In this case that event
 * will not be called until after all the events for
 * the current time which are already in the queue
 * have been called.
 */

struct TREE_STRUCT;
class Proc;
class IODEV;

#define MAXIMUM_EVENTS 10000


class Proc;

class Event
{
public:
  Event(IODEV *device = NULL, int reason = 0);
  ~Event();
  
  static void initialize();
  static void queue(Proc *p, unsigned long microseconds, IODEV *device, int reason = 0);
  static bool call_devices(unsigned long long &half_cycles, bool check_time);
  static void flush_events(unsigned long long &half_cycles);
  static void discard_events();
  static bool next_event_time(unsigned long long &half_cycles);

private:
  Proc *p;
  IODEV *device; // pointer to the device
  int reason; // Why the device was called.
  
  static TREE_STRUCT *event_queue;
  
  void call_one_device();
};
