/*
 * Copyright (C) 2010, 2014, 2026  Adrian Wise
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

#include "config.h"

#include <cstdlib>
#include <cstdio>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <iostream>

#include "asr.hpp"
#include "stdtty.hpp"
#include "pp_channel.h"

static bool special_chars(void *callback_arg, int k)
{
  ASR *p = static_cast<ASR *>(callback_arg);
  return p->special(k);
}

int main(int argc, char **argv)
{ 
  StdTty &stdtty {StdTty::getInstance()};
  ASR asr;
  stdtty.register_callback(static_cast<void *>(&asr), special_chars);
  
  char c;
  bool ok;
  struct pp_channel_s *ppc;

  ppc = pp_channel_init();

  /* loop for input */
  while (1) {

    if (pp_channel_can_send(ppc)) {
      ok = asr.get_asrch(c, true);
      if (ok) {
        pp_channel_send(ppc, &c, 1);
        /*printf("Sent 0x%02x ('%03o)\n", ((int)c &255), ((int)c &255));
          sleep(5);*/
      }
    }

    if (pp_channel_num_chars(ppc)) {
      if (pp_channel_read(ppc, &c, 1, -1) == 1) {
        fflush(stdout);
        asr.put_asrch(c);
      }
    }
  }

  pp_channel_done(ppc);
  exit(0);
}
