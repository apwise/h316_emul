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
#ifndef _PLT_HPP_
#define _PLT_HPP_

#include "p_to_io_intf.hpp"
#include "iodev.hpp"
#include <cstdio>

class PLT : public PToIoIntf, public IoDev
{
 public:
  PLT(IoToPIntf &p);

  Status ina(uint16_t instr, int16_t &data);
  Status sks(uint16_t instr);
  Status ota(uint16_t instr, int16_t data);
  void ocp(uint16_t instr);
  void smk(uint16_t mask);

  void event(int reason);
  void set_filename(const std::string &filename, unsigned subdevice); 

  void dmc(unsigned dmc_dev, int16_t &data, bool erl);

 private:
  static const int SPEED = 300; // steps per second
  static const int PEN_TIME = 20; // ms
  static const int SMK_MASK = (1 << (16-13));
  static const int INITIAL_X_POS = 42;
  static const int X_LIMIT = 3400;

  enum class Event {
    MASTER_CLEAR = EVENT_MASTER_CLEAR,
    NOT_BUSY
  };
  
  enum PHASE {
    LIMIT,    // Waiting to reach an EW limit
    UNLIMIT,  // Waiting to come out of limit
    SHOWTIME  // Normal operation
  };

  enum PLT_DIRN {
    PD_NULL = 000,
    PD_N  = 004,
    PD_NE = 005,
    PD_E  = 001,
    PD_SE = 011,
    PD_S  = 010,
    PD_SW = 012,
    PD_W  = 002,
    PD_NW = 006,
    PD_UP = 016,
    PD_DN = 014,
    
    PD_NUM
  };

  static const char *pd_names[16];

  const char *name();
  void master_clear(void);
  void turn_power_on(void);

  bool is_legal(PLT_DIRN dirn);
  bool is_pen(PLT_DIRN dirn);
  bool is_limit();
  void ensure_file_open();
  void plot_data();

  FILE *fp;
  bool ascii_file;
  std::string filename;

  PHASE phase;

  int x_pos, y_pos;
  bool pen;

  const int x_limit;

  unsigned short mask;
  bool not_busy;

  PLT_DIRN current_direction;
  int current_count;
};

#endif // _PLT_HPP_
