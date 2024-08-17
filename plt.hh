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
#ifndef _PLT_HH_
#define _PLT_HH_

class Proc;
class STDTTY;

class PLT : public IODEV
{
 public:
  PLT(Proc * const p, STDTTY * const stdtty);
  STATUS ina(unsigned short instr, signed short &data);
  STATUS ocp(unsigned short instr);
  STATUS sks(unsigned short instr);
  STATUS ota(unsigned short instr, signed short data);
  STATUS smk(unsigned short new_mask);

  void event(int reason);
  void set_filename(char *filename);

 private:
  static const int SPEED = 300; // steps per second
  static const int PEN_TIME = 20; // ms
  static const int SMK_MASK = (1 << (16-13));
  static const int INITIAL_X_POS = 42;
  static const int X_LIMIT = 3400;

  enum PLT_REASON {
    PLT_REASON_NOT_BUSY,
    
    PLT_REASON_NUM
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

  static const char *plt_reason[PLT_REASON_NUM];
  static const char *pd_names[16];

  void master_clear(void);
  void turn_power_on(void);

  bool is_legal(PLT_DIRN dirn);
  bool is_pen(PLT_DIRN dirn);
  bool is_limit();
  void ensure_file_open();
  void plot_data();

  Proc * const p;
  STDTTY * const stdtty;

  FILE *fp;
  bool ascii_file;
  bool pending_filename;
  char *filename;

  PHASE phase;

  int x_pos, y_pos;
  bool pen;

  const int x_limit;

  unsigned short mask;
  bool not_busy;

  PLT_DIRN current_direction;
  int current_count;
};
#endif
