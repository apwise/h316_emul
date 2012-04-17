/* Honeywell Series 16 emulator
 * Copyright (C) 1999, 2001  Adrian Wise
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
#include <unistd.h>

#include "fp.h"

#define COUNT_MAX 1000
static void run(struct FP_INTF *intf)
{
  static int count=0;
  static int pfi_count = 0;

  /*printf("run %d\n", count);*/

  if (intf->start_button_interrupt_pending)
    {
      printf("start_button_interrupt\n");
      intf->start_button_interrupt_pending = 0;
    }

  if (intf->power_fail_interrupt_pending)
    {
      printf("power_fail_interrupt\n");
      intf->power_fail_interrupt_pending = 0;
      pfi_count = 100;
    }

  if (pfi_count)
    {
      if (--pfi_count == 0)
        {
          intf->running = 0;
          intf->power_fail_interrupt_acknowledge = 1;
        }
    }

  usleep(250);

  if (count >= COUNT_MAX)
    count = 0;

  count ++;

  *(intf->reg_value[1]) = count;

  if (count >=  COUNT_MAX)
    intf->running = 0;
}

static void master_clear(struct FP_INTF *intf)
{
  printf("master_clear\n");
}

int main (int argc, char *argv[])
{
  int i;

  short reg[RB_NUM];
  int ssw[4];

  struct FP_INTF intf;

  for (i=0; i<RB_NUM; i++)
    intf.reg_value[i] = &reg[i];
  intf.reg_value[RB_NUM] = NULL;

  for (i=0; i<4; i++)
    intf.ssw[i] = &ssw[i];

  intf.pfh_not_pfi = 1;
  intf.mode = FPM_MA;
  intf.store = 0;
  intf.p_not_pp1 = 0;

  intf.running = 0;

  intf.run = run;

  intf.start_button_interrupt_pending = 0;
  intf.power_fail_interrupt_pending = 0;
  intf.power_fail_interrupt_acknowledge = 0;

  intf.master_clear = master_clear;
  
  intf.cpu = CPU_H316;

  for (i=0; i<RB_NUM; i++)
    reg[i] = (7-i)<<3;
  
  for (i=0; i<4; i++)
    ssw[i] = 0;
  ssw[1] = 1;

  process_args(&argc, &argv);
  setup_fp(&intf);

  exit(0);
}

