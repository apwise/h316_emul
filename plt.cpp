/* Honeywell Series 16 emulator
 *
 * Copyright (C) 2008, 2026  Adrian Wise
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
#include "plt.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sstream>

#include "iodev.hpp"
#include "stdtty.hpp"
#include "proc.hpp"

using namespace h16;

const char *PLT::pd_names[16] = {
  "(null)", "E",       "W",  "(error)",
  "N",      "NE",      "NW", "(error)",
  "S",      "SE",      "SW", "(error)",
  "DN",     "(error)", "UP", "(error)" 
};

bool PLT::is_legal(PLT_DIRN dirn)
{
  return ((dirn != PD_NULL) && 
          ((!(((dirn & PD_W) && (dirn & PD_E)) ||   // Can't have both East and West
              ((dirn & PD_N) && (dirn & PD_S)))) || // nor both North and South
           is_pen(dirn)));                          // except for the pen up/down
}

bool PLT::is_pen(PLT_DIRN dirn) {
  return ((dirn == PD_UP) || (dirn == PD_DN));
}

bool PLT::is_limit() {
  return ((x_pos < 0) || (x_pos >= x_limit));
}


PLT::PLT(IoToPIntf &p)
  : IoDev(p),
    fp(NULL),
    ascii_file(false),
    phase(LIMIT),
    x_pos(INITIAL_X_POS),
    y_pos(0),
    pen(false),
    x_limit(X_LIMIT),
    mask(0),
    not_busy(true),
    current_direction(PD_NULL),
    current_count(0)
{
  master_clear();
}

void PLT::master_clear()
{
  // Complete last plot
  if (fp) {
    plot_data();
    fclose(fp);
  }

  fp = NULL;
  ascii_file = false;
  filename.clear();
  phase = LIMIT;

  // Reset pen and notional y position,
  // but x position is physical and not cleared here.
  y_pos = 0;
  pen = false;

  // Clear ready for start of file
  current_direction = PD_NULL;
  current_count = 0;

  // Clear mask and busy flip-flops
  mask = 0;
  not_busy = true;
};

void PLT::ensure_file_open()
{
  std::string str;

  if (!fp) {
    while (!fp) {
      if (filename.size() != 0) {
        str = filename;
      } else {
        str = p.get_file_name("PLT", "plt", "");
      }
      
      if (str[0]=='&') {
        ascii_file = true;
        str = str.substr(1);
      }
    
      fp = fopen(str.c_str(), ((ascii_file) ? "w" : "wb") );
      if (!fp) {
        std::stringstream ss;
        ss << "Could not open <" << str << "> for writing";
        
        IoToPIntf::Level level = ((filename.size()) ? IoToPIntf::Level::FATAL : IoToPIntf::Level::ERROR);
        
        p.anomaly(level, ss.str());
      }

      filename.clear();
    }
  }
}

void PLT::plot_data()
{
  int data;
  int bit, limit;

  if (current_direction != PD_NULL) {
    if (ascii_file) {
      fprintf(fp, "%s", pd_names[current_direction]);
      if (current_count > 0)
        fprintf(fp, " %d", current_count);
      fprintf(fp, "\n");
    } else {
      if (current_count > 7) {
        // Figure out the limit that can be represented
        // by the minimum number of prefix bytes
        bit = 3;
        do {
          bit += 7;
          limit = 1 << bit;
        } while (limit <= current_count);

        // Output those prefix bytes
        do {          
          bit -= 7;

          data = ((current_count >> bit) & 127) | 128;

          putc(data, fp);
        } while (bit > 3);
      }
      data = (((current_direction & 15) << 3) |
              (current_count & 7));
      putc(data, fp);
    }
    
    current_count = 0;
    current_direction = PD_NULL;
  }
}

void PLT::ocp(uint16_t instr)
{
  // Don't really need to do this here, but
  // it's better to ask for the filename at the start
  // rather than leave it to witter a while before
  // asking for the filename when the user's moved on
  ensure_file_open();

  PLT_DIRN direction = (PLT_DIRN) ((instr >> 6) & 017);

  if (is_legal(direction)) {
    
    //fprintf(stderr, "PLT: OCP '%04o\n", instr & 0x3ff);
    //fprintf(stderr, "PLT: OCP '%04o, x_pos = %d %d\n", instr & 0x3ff, x_pos, is_limit());

    // As soon as the pen goes down we must produce data
    if (direction == PD_DN)
      phase = SHOWTIME;

    if (phase == SHOWTIME) {
      if (direction == current_direction)
        current_count++;
      else {
        plot_data();
        current_direction = direction;
      }
    }

    // Keep track of location
    switch(direction) {
    case PD_N:  y_pos++;          break;
    case PD_NE: y_pos++; x_pos++; break;
    case PD_E:           x_pos++; break;
    case PD_SE: y_pos--; x_pos++; break;
    case PD_S:  y_pos--;          break;
    case PD_SW: y_pos--; x_pos--; break;
    case PD_W:           x_pos--; break;
    case PD_NW: y_pos++; x_pos--; break;
    case PD_UP: pen = false; break;
    case PD_DN: pen = true;  break;
    default:
      abort();
    }

    // Normally wait until we've hit a limit
    // switch and backed off it before starting to 
    // produce data

    switch (phase) {
    case SHOWTIME:                                   break;
    case LIMIT:   if ( is_limit()) phase = UNLIMIT;  break;
    case UNLIMIT: if (!is_limit()) phase = SHOWTIME; break;
    default:
      abort();
    }

    unsigned long microseconds = (is_pen(direction) ?
                                  PEN_TIME * 1000 : (1000000 / SPEED));

    not_busy = false;
    if (mask) {
      p.clear_interrupt(SMK_MASK);
    }
    
    p.queue(microseconds, *this, static_cast<int>(Event::NOT_BUSY) );
  } else {
    p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  }
}

IoStatus PLT::sks(uint16_t instr)
{
  bool r = 0;
  
  switch(instr & 0700) {
  case 0100: r = not_busy; break;
  case 0200: r = !is_limit(); break;
  case 0400: r = !(not_busy && mask); break;
    
  default:
    p.anomaly(IoToPIntf::Level::ERROR, message(instr));
  }
  
  return status(r);
}

void PLT::smk(uint16_t new_mask)
{
  mask = new_mask & SMK_MASK;
  
  if (not_busy && mask)
    p.set_interrupt(mask);
  else
    p.clear_interrupt(SMK_MASK);
}

void PLT::event(int reason)
{
  const Event event {static_cast<Event>(reason)};

  switch(event) {
  case Event::MASTER_CLEAR:
    master_clear();
    break;
      
  case Event::NOT_BUSY:
    not_busy = true;
    if (mask) {
      p.set_interrupt(mask);
    }
    break;
    
  default:
    p.anomaly(IoToPIntf::Level::FATAL, uxReason(reason));
    break;
  }
}

void PLT::set_filename(const std::string &filename, unsigned subdevice) {
  master_clear();
  this->filename = filename;
}


DEFINE_UNEXPECTED_INA(PLT)
DEFINE_UNEXPECTED_OTA(PLT)
DEFINE_UNEXPECTED_DMC(PLT)

DEFINE_STD_NAME(PLT)
