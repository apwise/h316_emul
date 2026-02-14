/* Honeywell Series 16 emulator
 *
 * Copyright (C) 1997, 1998, 1999, 2005, 2010, 2011, 2018, 2026  Adrian Wise
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
 * This file constitutes the main simulation kernel of the processor.
 */

#include "cpu.hpp"
#include "gtk/fp.h"

/*****************************************************************
 * Build the front-panel interface structure
 *****************************************************************/
struct FP_INTF *h16::CPU::fp_intf()
{
  struct FP_INTF *intf;
  int i;

  intf = new struct FP_INTF;

  intf->reg_value[RB_X] = &x;
  intf->reg_value[RB_A] = &a;
  intf->reg_value[RB_B] = &b;
  intf->reg_value[RB_PY] = (int16_t *)&y;
  intf->reg_value[RB_OP] = &op;
  intf->reg_value[RB_M] = &m;
  intf->addr_mask = addr_mask;
  
  for (i=0; i<4; i++)
    intf->ssw[i] = &ss[i];

  intf->pfh_not_pfi = 0;
  intf->mode=FPM_RUN;
  intf->store=0;
  intf->p_not_pp1=0;

  intf->running=run;

  intf->start_button_interrupt_pending=0;
  intf->power_fail_interrupt_pending=0;
  intf->power_fail_interrupt_acknowledge=0;

  /*
   * Call-back routines from the front-panel
   */

  intf->run = NULL;
  intf->master_clear = NULL;

  intf->cpu_model = CPU_H316;

  intf->data = (void *) this;

  return intf;
}
