/* Honeywell Series 16 emulator
 * Copyright (C) 1998  Adrian Wise
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

enum RB
  {
    RB_X,
    RB_A,
    RB_B,
    RB_OP,
    RB_PY,
    RB_M,
    RB_NUM
  };

enum CPU
  {
    CPU_H316,
    CPU_DDP516,
    CPU_NUM
  };

enum FP_MODE
  {
    FPM_MA,
    FPM_SI,
    FPM_RUN,
    FPM_NUM
  };

struct FP_INTF
{
  /*
   * Pointers to the registers
   *
   * The spare one (*reg_values[RB_NUM]) is used
   * internally by the front_panel to point at a
   * dummy value if none of the register select
   * buttons is depressed
   */
  short *reg_value[RB_NUM+1];

  /*
   * (Boolean) sense switch values
   */
  int *ssw[4];

  /*
   * (Boolean) Power fail action
   */
  int pfh_not_pfi;

  /*
   * Front-panel MA-SI-RUN switch;
   */
  enum FP_MODE mode;
        
  /*
   * Front-panel store/fetch and p/p+1 switches
   */
  int store;
  int p_not_pp1;

  /*
   * Is the machine running?
   */
  int running;
  int exit_called;

  int start_button_interrupt_pending;
  int power_fail_interrupt_pending;
  int power_fail_interrupt_acknowledge;

  /*
   * Call-back routines from the front-panel
   */
        
  void (*run)(struct FP_INTF *intf);
  void (*master_clear)(struct FP_INTF *intf);

  /*
   * CPU options
   */

  enum CPU cpu;

  /*
   * Carry a pointer (intended as a pointer to the Proc
   * to which this front-panel is connected).
   */
  void *data;
};

/*
 * setup_fp() doesn't return until it is appropriate to
 * quit
 */
#ifdef __cplusplus
#define _EXTERN_
extern "C" {
#else
#define _EXTERN_ extern
#endif

  _EXTERN_ void process_args(int *argc, char ***argv);
  _EXTERN_ void setup_fp(struct FP_INTF *intf);

#ifdef __cplusplus
}
#endif

#undef _EXTERN_


