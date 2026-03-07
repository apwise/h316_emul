/*
 * Copyright (C) 2014, 2016, 2026  Adrian Wise
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

#include <cstdlib>
#include <cstdio>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <iostream>

#include "asr.hpp"
#include "stdtty.hpp"

#include "depp_channel.h"

static bool special_chars(void *callback_arg, int k)
{
  ASR *p = static_cast<ASR *>(callback_arg);
  return p->special(k);
}

int main(int argc, char **argv)
{ 
  StdTty &stdtty(StdTty::getInstance());
  ASR asr;
  stdtty.register_callback(static_cast<void *>(&asr), special_chars);

  char c;
  bool ok;
  struct depp_channel_s *ppc;

  static const unsigned int BUF_LEN = 128;
  
  char buf[BUF_LEN];
  unsigned int i;
  bool file_input;
  
  if (! (ppc = depp_channel_init()))
    exit(1);
  
  i = 0;
  file_input = false;
  
  /* loop for input */
  while (1) {

    if ((file_input && (i<BUF_LEN)) ||
        depp_channel_can_send(ppc)) {
      ok = asr.get_asrch(c, true);
      if (ok) {
        buf[i++] = c;

        //bool prev_file_input = file_input;
        
        file_input = asr.file_input();
        if ((i >= BUF_LEN) || (!file_input)) {
          depp_channel_send(ppc, buf, i);
          i = 0;
        }

        /*
        if (prev_file_input && (!file_input)) {
          // Can we kick input thread out of wait?
        }
        */
      }
    }

    if (depp_channel_num_chars(ppc)) {
      if (depp_channel_read(ppc, &c, 1, -1) == 1) {
        fflush(stdout);
        asr.put_asrch(c);
      }
    }
  }

  depp_channel_done(ppc);
  exit(0);
}
