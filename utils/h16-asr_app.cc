#include <unistd.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <iostream>

#include "dummy_proc.hh"
#include "asr.hh"
#include "stdtty.hh"
#include "serial.hh"

#define SOH  001 // Start of heading
#define ENQ  005 // Enquiry
#define LF   012 // Line feed
#define DC1  021 // Device control 1
#define DC2  022 // Device control 2
#define DC3  023 // Device control 3
#define DEL 0177 // Delete

#define WRU  ENQ // Who Are You?
#define XON  DC1 // Start tape reader
#define TAPE DC2 // Start tape punch
#define XOFF DC3 // Stop reader (and punch)

#define RUBOUT 0377

//#define DEFAULT_DEVICE "/dev/ttyUSB0"
#define DEFAULT_DEVICE "/dev/ttyS0"
//#define DEFAULT_BAUD 1200
//#define DEFAULT_BAUD 600
#define DEFAULT_BAUD 110

static bool call_special_chars(Proc *p, int k)
{
  return p->special(k);
}

class Pal_monitor {
public:
  Pal_monitor();

  unsigned int zero_words(char c);
  
private:
  enum state_e {
    ST_IDLE,
    ST_CHAR1,
    ST_CHAR2,
    ST_CHAR3
  };
  state_e state;
  unsigned int word;
  bool zeros;
  unsigned int prev_zeros;
  
  bool translate(unsigned int &c8);
};

Pal_monitor::Pal_monitor()
  : state(ST_IDLE)
  , word(0)
  , zeros(false)
  , prev_zeros(0)
{
}

bool Pal_monitor::translate(unsigned int &c8)
{
  unsigned int c7 = c8 & 0177;
  unsigned int n7;
  bool good = true;

  switch (c7) {
  case 0177: n7 = XOFF; break;
  case 0176: n7 = XON;  break;
  case 0175: n7 = LF;   break;
  case 0174: n7 = WRU;  break;
  default:   n7 = c7;
  }

  if (n7 & 0140) {
    // Channels 6 or 7 punched
    good = false;
  }

  c8 = ((c8 & 0200) >> 2) | (n7 & 037);

  return good;
}

unsigned int Pal_monitor::zero_words(char c)
{
  int zero_words = 0;
  unsigned int c8 = c & 0377;
  unsigned int c7 = c & 0177;
  state_e next_state = state;

  if (c7 == XOFF) {
    next_state = ST_IDLE;
  } else {
    switch (state) {
      
    case ST_IDLE:
      if (c7 == SOH) {
        next_state = ST_CHAR1;
      }
      break;
    case ST_CHAR1:
      if (translate(c8)) {
        if (c8 & 020) {
          next_state = ST_IDLE;
        } else {
          zeros = ((c8 & 040) != 0);
          word  = (c8 & 017) << 12;
          next_state = ST_CHAR2;
        }
      } else {
        next_state = ST_IDLE;
      }
      break;
    case ST_CHAR2:
      if (translate(c8)) {
        word = word | (c8 << 6);
        next_state = ST_CHAR3;
      } else {
        next_state = ST_IDLE;
      }
      break;
    case ST_CHAR3:
      if (translate(c8)) {
        word = word | c8;
        if (zeros) {
          int n = 0200000 - word;
          if ((n>0) && (n < 50)) { // Maximum PAL block size
            zero_words = n;
          }
        }
        next_state = ST_CHAR1;
      } else {
        next_state = ST_IDLE;
      }
      break;
    }
  }
  state = next_state;

  unsigned int r = prev_zeros;
  prev_zeros = zero_words;
  
  return r;
}

int main(int argc, char **argv)
{
  bool help = false;
  bool usage = false;
  unsigned baud = DEFAULT_BAUD;
  const char *device = DEFAULT_DEVICE;
  
  int a = 1;
  while (a < argc) {
    if (argv[a][0] == '-') {
      if ((strcmp(argv[a], "-h") == 0) || (strcmp(argv[a], "--help") == 0)) {
        help = true;
        break;
      } else if ((strncmp(argv[a], "-b", 2) == 0) || (strncmp(argv[a], "--baud",6) == 0)) {
        unsigned int n = (argv[a][1] == '-') ? 6 : 2;

        if ((n==6) && (strlen(argv[a]) > (n+1)) && (argv[a][n] == '=')) {
          ++n; // Allow "--baud=<num>", but not "--baud=" (with no number)
        }

        unsigned long b = 0;
        char *nptr = &argv[a][n];
        char *endptr;

        if (strlen(argv[a]) <= n) {
          // No text remains following flag - baud in next argument
          ++a;
          if (a >= argc) {
            usage = 1;
            break;
          }
          nptr = argv[a];
        }
        
        // Try to convert the baud rate argument
        b = strtoul(nptr, &endptr, 10);
        if ((*endptr) != '\0') {
          fprintf(stderr, "Failed to convert \"%s\" to baud rate\n", nptr);
          exit(1);
        }
        baud = b;
      } else {
        // Flag not recognized
        usage = 1;
      }
    } else {
      break;
    }
    ++a;
  }

  if (a < argc) {
    device = argv[a];
    ++a;
  }
  
  if (a < argc) {
    usage = 1;
  }
  
  if (help || usage) {
    fprintf(((usage) ? stderr : stdout),
            "usage: %s [-h] [-b|--baud= <baud-rate> (default=%d)] [<device> (default=\"%s\")]\n",
            argv[0], DEFAULT_BAUD, DEFAULT_DEVICE);
    exit((usage) ? 1 : 0);
  }

  fd_set readfs;    /* file descriptor set */
  fd_set writefs;   /* file descriptor set */
  int    maxfd=0;   /* maximum file desciptor used */

  STDTTY stdtty;
  ASR asr(&stdtty);
  Proc p(&asr);
  Serial serial(device, baud);
  char c;
  bool ok;
  Pal_monitor pal_monitor;
  
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
       * (unlike a keyboard) so monitor the
       * serial line to see if it is possible to
       * write to the serial line. 
       */

      if (asr.file_input()) {
        FD_SET(serial.get_fd(), &writefs);
      }
      
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
              if (ok) {
                serial.transmit(c, ok);
                unsigned int zero_words = pal_monitor.zero_words(c);
                unsigned int useconds = 0;
                if (zero_words > 8) {
                  // 100ms second to allow buffers to empty
                  // plus 1ms per zero_word
                  useconds = 100000 + (zero_words * 1000);
                }
                if (useconds > 1000000) {
                  useconds = 1000000;
                }
                (void) usleep(useconds);
              }
            }
          
          if (FD_ISSET(serial.get_fd(), &readfs))
            {
              /* Input from serial */
              c = serial.receive(ok);
              if (ok) {
                fflush(stdout);
                asr.put_asrch(c);
              }
            }
        }
    }
  

  exit(0);
}
