#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "dummy_proc.hh"
#include "asr.hh"
#include "stdtty.hh"
#include "serial.hh"

static bool call_special_chars(Proc *p, int k)
{
  return p->special(k);
}

int main(int argc, char **argv)
{ 
  fd_set readfs;    /* file descriptor set */
  fd_set writefs;   /* file descriptor set */
  int    maxfd=0;   /* maximum file desciptor used */

  STDTTY stdtty;
  ASR asr(&stdtty);
  Proc p(&asr);
  Serial serial;
  char c;
  bool ok;

  stdtty.set_proc(&p, &call_special_chars);

  maxfd = serial.get_fd() + 1;
  if (maxfd < STDIN_FILENO)
    maxfd = STDIN_FILENO + 1;

  /* loop for input */
  bool rfs, wfs;
  int r;

  while (1)
    {
      FD_ZERO(&readfs);
      FD_ZERO(&writefs);

      FD_SET(STDIN_FILENO, &readfs);

      /*
       * If input is coming from a file then
       * it will always be possible to get input
       * (unlike a keyoboard) so monitor the
       * serial line to see if it is possible to
       * write to the serial line 
       */

      if (asr.file_input())
        FD_SET(serial.get_fd(), &writefs);
      
      FD_SET(serial.get_fd(), &readfs);

      /*
       * block until input becomes available
       * or output is possible
       */
      r = select(maxfd, &readfs, &writefs, NULL, NULL);
      if (r >= 0)
        {
          rfs = wfs = false;
          
          if ((rfs=FD_ISSET(STDIN_FILENO, &readfs)) ||
              (wfs=FD_ISSET(serial.get_fd(), &writefs)) )
            {
              //if (rfs)
              //  fprintf(stderr, "Keyboard input ready\n");
              //if (wfs)
              //  fprintf(stderr, "Serial output ready\n");
              
              /* Input from keyboard */
              ok = asr.get_asrch(c, false);
              if (ok)
                serial.transmit(c, ok);
            }
          
          if (FD_ISSET(serial.get_fd(), &readfs))
            {
              //fprintf(stderr, "Serial input ready\n");
              
              /* Input from serial */
              c = serial.receive(ok);
              if (ok) {
		//printf("<%03o>", c & 0xff);
		fflush(stdout);
                asr.put_asrch(c);
	      }
            }
        }
    }
  

  exit(0);
}
