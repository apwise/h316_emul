#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
//#include <ioctls.h>
#include <termios.h>

#include <sys/signal.h>
#include <sys/types.h>

#include "stdtty.hh"
#include "serial.hh"

typedef struct
{
  speed_t speed;
  unsigned int baud_rate;
} baud_pair_t;

const baud_pair_t baud_pairs[] =
  {
    {B50,     50    },
    {B75,     75    },
    {B110,    110   },
    {B134,    134   },
    {B150,    150   },
    {B200,    200   },
    {B300,    300   },
    {B600,    600   },
    {B1200,   1200  },
    {B1800,   1800  },
    {B2400,   2400  },
    {B4800,   4800  },
    {B9600,   9600  },
    {B19200,  19200 },
    {B38400,  38400 },
    {B57600,  57600 },
    {B115200, 115200},
    {B230400, 230400},
    {B0,      0     },
  };

Serial::Serial(const char *device, unsigned int baud)
{
  oldtio = new struct termios;
  newtio = new struct termios;

  /* open the device to be non-blocking (read will return immediatly) */
  //printf("About to open %s\n", DEVICE);
  fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd <0) {perror(device); exit(1); }

  /* allow the process to receive SIGIO */
  //fcntl(fd, F_SETOWN, getpid());
  /* Make the file descriptor asynchronous (the manual page says only
     O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
  //fcntl(fd, F_SETFL, FASYNC);

  tcgetattr(fd,oldtio); /* save current port settings */

  tcgetattr(fd,newtio); /* save current port settings */
  /* set new port settings for canonical input processing */

  newtio->c_cflag |= CLOCAL;
  newtio->c_cflag &= ~HUPCL;
  newtio->c_cflag |= CREAD;
  newtio->c_cflag |= CSTOPB;
  newtio->c_cflag &= ~PARENB;
  newtio->c_cflag = (newtio->c_cflag & ~CSIZE) | CS8;
  //  newtio->c_cflag &= ~CCTS_OFLOW;
  //  newtio->c_cflag &= ~CRTS_IFLOW;
  //  newtio->c_cflag &= ~MDMBUF;
  //  newtio->c_cflag &= ~CIGNORE;

  // Lookup the numerical baud rate
  speed_t speed = B0;
  unsigned i = 0;
  while (baud_pairs[i].baud_rate) {
    if (baud_pairs[i].baud_rate == baud) {
      speed = baud_pairs[i].speed;
      break;
    }
    ++i;
  }
  if (speed == B0) {
    fprintf(stderr, "Baud rate not recognised\n");
    exit(1);
  }
  
  if (cfsetispeed(newtio, speed))
    {
      fprintf(stderr, "Could not set input baud rate\n");
      exit(1);
    }

  if (cfsetospeed (newtio, speed))
    {
      fprintf(stderr, "Could not set output baud rate\n");
      exit(1);
    }
        
  newtio->c_iflag &= ~INPCK;
  newtio->c_iflag |= IGNPAR;
  newtio->c_iflag &= ~PARMRK;
  newtio->c_iflag &= ~ISTRIP;
  newtio->c_iflag |= IGNBRK;
  newtio->c_iflag &= ~IGNCR;
  newtio->c_iflag &= ~ICRNL;
  newtio->c_iflag &= ~INLCR;
  newtio->c_iflag &= ~IXOFF;
  newtio->c_iflag &= ~IXON;

  newtio->c_oflag &= ~OPOST;
  newtio->c_oflag &= ~ONLCR;
  //newtio->c_oflag &= ~OXTABS;
  //newtio->c_oflag &= ~ONOEOT;

  //newtio->c_lflag |= ICANON;
  newtio->c_lflag &= ~ICANON;
  newtio->c_lflag &= ~ECHO;
  newtio->c_lflag &= ~ECHONL;
  newtio->c_lflag &= ~ISIG;
  //newtio->c_lflag &= ~LNEXT;
  newtio->c_lflag &= ~TOSTOP;

  newtio->c_cc[VMIN]=1;
  newtio->c_cc[VTIME]=0;
  tcflush(fd, TCIFLUSH);

  if (tcsetattr(fd,TCSANOW,newtio)) {
      fprintf(stderr, "Unable to set terminal attributes\n");
      exit(1);
  }

  struct termios check;
  tcgetattr(fd, &check);

  if (cfgetispeed(&check) != speed) {
    fprintf(stderr, "Input baud rate not correct\n");
    exit(1);
  }
  
  if (cfgetospeed(&check) != speed) {
    fprintf(stderr, "Output baud rate not correct\n");
    exit(1);
  }
  
  // 11 bits per character
  // in microseconds...
  character_time = (1000000 * 11) / baud;
}

Serial::~Serial()
{
  tcsetattr(fd,TCSANOW,oldtio);

  delete oldtio;
  delete newtio;
}

char Serial::receive(bool &ok)
{
  char c;

  ok = (read (fd, &c, 1) == 1);

  //if (ok) {
  //  printf("Got 0x%02x\n", ((int)c) & 0xff);
  //}
  
  return c;
}

void Serial::transmit(char c, bool &ok)
{
  ok = (write (fd, &c, 1) == 1);

  // Don't get ahead of the line
  usleep(character_time);
}
