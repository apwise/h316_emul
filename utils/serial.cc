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

#define DEVICE "/dev/ttyUSB0"
#define BAUD B1200
//#define BAUD B600
//#define BAUD B110
//#define BAUD B9600

#include "stdtty.hh"
#include "serial.hh"

Serial::Serial()
{
  oldtio = new struct termios;
  newtio = new struct termios;

  /* open the device to be non-blocking (read will return immediatly) */
  //printf("About to open %s\n", DEVICE);
  fd = open(DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd <0) {perror(DEVICE); exit(1); }

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

  if (cfsetispeed(newtio, BAUD))
    {
      fprintf(stderr, "Could not set input baud rate\n");
      exit(1);
    }

  if (cfsetospeed (newtio, BAUD))
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

  if (cfgetispeed(&check) != BAUD) {
    fprintf(stderr, "Input baud rate not correct\n");
    exit(1);
  }
  
  if (cfgetospeed(&check) != BAUD) {
    fprintf(stderr, "Output baud rate not correct\n");
    exit(1);
  }
  
  
  unsigned int baud = 0;
  switch (BAUD) {
  case B50:   baud = 50;   break;
  case B75:   baud = 75;   break;
  case B110:  baud = 110;  break;
  case B300:  baud = 300;  break;
  case B600:  baud = 600;  break;
  case B1200: baud = 1200; break;
  case B2400: baud = 2400; break;
  case B4800: baud = 4800; break;
  case B9600: baud = 9600; break;
  default:
    fprintf(stderr, "Unknown baud rate\n");
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
