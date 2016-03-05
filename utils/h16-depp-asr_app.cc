#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "dummy_proc.hh"
#include "asr.hh"
#include "stdtty.hh"
#include "depp_channel.h"

static bool call_special_chars(Proc *p, int k)
{
  return p->special(k);
}

int main(int argc, char **argv)
{ 
  STDTTY stdtty;
  ASR asr(&stdtty);
  Proc p(&asr);
  char c;
  bool ok;
  struct depp_channel_s *ppc;

  static const unsigned int BUF_LEN = 128;
  
  char buf[BUF_LEN];
  unsigned int i;
  bool file_input;
  
  if (! (ppc = depp_channel_init()))
    exit(1);
  
  stdtty.set_proc(&p, &call_special_chars);

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
