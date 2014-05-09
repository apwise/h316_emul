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

  if (! (ppc = depp_channel_init()))
    exit(1);
  
  stdtty.set_proc(&p, &call_special_chars);

  /* loop for input */
  while (1) {

    if (depp_channel_can_send(ppc)) {
      ok = asr.get_asrch(c, true);
      if (ok) {
        depp_channel_send(ppc, &c, 1);
        /*printf("Sent 0x%02x ('%03o)\n", ((int)c &255), ((int)c &255));
          sleep(5);*/
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
