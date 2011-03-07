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
#include <stdlib.h>
#include <stdio.h>


#include "stdtty.hh"
#include "proc.hh"
#include "iodev.hh"
#include "tree.hh"
#include "event.hh"

TREE_STRUCT *Event::event_queue = NULL;

Event::Event(IODEV *device, int reason)
{
  this->device = device;
  this->reason = reason;
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
  event_queue = NULL;
}

#define cycle_time (1.600)
#define half_cycles_per_microsecond (2.0 / cycle_time)

void Event::queue(Proc *p, unsigned long microseconds, IODEV *device, int reason)
{
  Event *ev;
  unsigned long half_cycles;
  
  half_cycles = (unsigned long) (((double) microseconds) * 
                                 half_cycles_per_microsecond);
  
  ev = new Event(device, reason);
  tree_add( &event_queue,
            half_cycles + p->get_half_cycles(), ev);
}

bool Event::call_devices(unsigned long long &half_cycles, bool check_time)
{
  Event *ev;
  unsigned long long ev_time;
  unsigned long event_count = 0;
  bool r = 0;
  
  while ( (ev = (Event *) tree_remove_left_most(&event_queue,
                                                ev_time, half_cycles,
                                                check_time)))
    {
      r = 1;
      if (!check_time) half_cycles = ev_time;
      
      ev->call_one_device();
      delete ev;
      
      event_count ++;
      if (event_count > MAXIMUM_EVENTS)
        {
          fprintf(stderr, "More than %d Events at time %llu, Infinite loop ?\n",
                  MAXIMUM_EVENTS, half_cycles);
          exit(1);
        }
    }
  return r;
}

void Event::flush_events(unsigned long long &half_cycles)
{
  while(call_devices(half_cycles, 0));
}

static void free_event(void *p)
{
  Event *ep = (Event *) p;
  delete ep;
}

void Event::discard_events()
{
  tree_free( &event_queue, free_event);
}

bool Event::next_event_time(unsigned long long &half_cycles)
{
  return tree_left_name(&event_queue, half_cycles);
}

