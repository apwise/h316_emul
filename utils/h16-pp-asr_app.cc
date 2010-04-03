#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "dummy_proc.hh"
#include "asr.hh"
#include "stdtty.hh"
#include "pp_channel.h"

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
  struct pp_channel_s *ppc;

  ppc = pp_channel_init();

  stdtty.set_proc(&p, &call_special_chars);

  /* loop for input */
  bool rfs, wfs;
  int r;

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
