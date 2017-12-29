/* Honeywell Series 16 emulator
 * Copyright (C) 2008  Adrian Wise
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
#include <string.h>

#include "iodev.hh"
#include "stdtty.hh"

#include "proc.hh"
#include "plt.hh"


const char *PLT::plt_reason[PLT_REASON_NUM] __attribute__ ((unused)) =
{
  "Not busy"
};

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


PLT::PLT(Proc *p, STDTTY *stdtty)
  : IODEV(p),
    p(p),
    stdtty(stdtty),
    fp(NULL),
    ascii_file(false),
    pending_filename(false),
    filename(NULL),
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
  pending_filename = false;
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

PLT::STATUS PLT::ina(unsigned short instr, signed short &data)
{
  fprintf(stderr, "%s Input from plotter\n", __PRETTY_FUNCTION__);
  exit(1);
  return STATUS_READY;
}

PLT::STATUS PLT::ota(unsigned short instr, signed short data)
{
  fprintf(stderr, "%s Output to plotter\n", __PRETTY_FUNCTION__);
  exit(1);
  return STATUS_READY;
}

void PLT::ensure_file_open()
{
  char buf[256];
  char *cp;
  if (!fp) {
    while (!fp) {
      if (pending_filename)
        strcpy(buf, filename);
      else
        stdtty->get_input("PLT: Filename>", buf, 256, 0);
         
      cp = buf;
      if (buf[0]=='&') {
        ascii_file = 1;
        cp++;
      }
        
      fp = fopen(cp, ((ascii_file) ? "w" : "wb") );
      if (!fp) {
        fprintf(((pending_filename) ? stderr : stdout),
                "Could not open <%s> for writing\n", cp);
        if (pending_filename)
          exit(1);
      }
        
      if (pending_filename)
        delete filename;
        
      pending_filename = 0;
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

PLT::STATUS PLT::ocp(unsigned short instr)
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

    unsigned long microseconds = is_pen(direction) ?
      PEN_TIME * 1000 :
      (1000000 / SPEED);

    not_busy = false;
    if (mask)
      p->clear_interrupt(SMK_MASK);
    p->queue(microseconds, this, PLT_REASON_NOT_BUSY );
  } else {
    fprintf(stderr, "PLT: OCP '%04o\n", instr & 0x3ff);
    exit(1);
  }
  return STATUS_READY;
}

PLT::STATUS PLT::sks(unsigned short instr)
{
  bool r = 0;
  
  switch(instr & 0700)
    {
    case 0100: r = not_busy; break;
    case 0200: r = !is_limit(); break;
    case 0400: r = !(not_busy && mask); break;
      
    default:
      fprintf(stderr, "PLT: SKS '%04o\n", instr&0x3ff);
      exit(1);
    }
  
  return status(r);
}

PLT::STATUS PLT::smk(unsigned short new_mask)
{
  mask = new_mask & SMK_MASK;
  
  if (not_busy && mask)
    p->set_interrupt(mask);
  else
    p->clear_interrupt(SMK_MASK);

  return STATUS_READY;
}

void PLT::event(int reason)
{
  switch(reason)
    {
    case REASON_MASTER_CLEAR:
      master_clear();
      break;
      
    case PLT_REASON_NOT_BUSY:
      not_busy = true;
      if (mask)
        p->set_interrupt(mask);
      break;
                        
    default:
      fprintf(stderr, "%s %d\n", __PRETTY_FUNCTION__, reason);
      exit(1);
      break;
    }
}

void PLT::set_filename(char *filename)
{
  master_clear();
  this->filename = strdup(filename);
  pending_filename = 1;
}
