#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ppdev.h>
#include <linux/parport.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>
#include <stdbool.h>

#include "pp_channel.h"

static const int initial_fifo_size = 0x0400;
static const long initial_wait = 10000; // 10us - 100kHz
static const long max_wait = 10000000;  // 10ms - 100Hz

static inline long next_wait(long current_wait)
{
  long r = current_wait << 1;
  if (r > max_wait)
    r = max_wait;
  return r;
}

struct pp_channel_s {
  int fd;
  pthread_t input_thread;
  pthread_mutex_t mutex_port;
  bool running;
  char *fifo;
  int fifo_size;
  int fifo_wp;
  int fifo_rp;
  pthread_mutex_t mutex_fifo;
};

static void error(const char *msg, struct pp_channel_s *ppc)
{
  perror(msg);

  /* Try to release the port */
  if (ppc->fd) {
    (void) ioctl(ppc->fd, PPRELEASE, NULL);
    
    close(ppc->fd);
  }

  exit(1);
}

static void nanowait(long tv_nsec, struct pp_channel_s *ppc)
{
  struct timespec delay;
  int r;

  delay.tv_sec = 0;
  delay.tv_nsec = tv_nsec;

  r = -1;

  while (r != 0) {
    r = nanosleep(&delay, &delay);
    if (errno != EINTR) break;
  }

  if (r)
    error("nanosleep", ppc);
}

#if 0
static void longwait(long tv_sec, struct pp_channel_s *ppc)
{
  struct timespec delay;
  int r;

  delay.tv_sec = tv_sec;
  delay.tv_nsec = 0;

  r = -1;

  while (r != 0) {
    r = nanosleep(&delay, &delay);
    if (errno != EINTR) break;
  }

  if (r)
    error("nanosleep", ppc);
}
#endif

static inline int fifo_fullness(struct pp_channel_s *ppc)
{
  int n = ppc->fifo_wp - ppc->fifo_rp;
  if (n<0)
    n+=ppc->fifo_size;
  return n;
}

static inline int fifo_space(struct pp_channel_s *ppc)
{
  return (ppc->fifo_size-1) - fifo_fullness(ppc);
}

static inline char fifo_get(struct pp_channel_s *ppc)
{
  char c = 0;

  if (fifo_fullness(ppc))
    c = ppc->fifo[ppc->fifo_rp++];
  if (ppc->fifo_rp >= ppc->fifo_size)
    ppc->fifo_rp = 0;
  return c;
}

static inline void fifo_add(struct pp_channel_s *ppc, char c)
{
  if (fifo_space(ppc))
    ppc->fifo[ppc->fifo_wp++] = c;
  else {
    /*
     * Make the FIFO bigger
     */
    int new_fifo_size = ppc->fifo_size * 2;
    char *new_fifo = (char *) malloc(new_fifo_size);
    int new_wp = 0;

    while (fifo_fullness(ppc))
      new_fifo[new_wp++] = fifo_get(ppc);
    new_fifo[new_wp++] = c;

    free(ppc->fifo);
    ppc->fifo = new_fifo;

    ppc->fifo_size = new_fifo_size;
    ppc->fifo_wp = new_wp;
    ppc->fifo_rp = 0;
  }

  if (ppc->fifo_wp >= ppc->fifo_size)
    ppc->fifo_wp = 0;
}


static void *pp_input_thread(void *p)
{
  struct pp_channel_s *ppc = ((struct pp_channel_s *) p);
  int r;
  int modes;
  char buf[129];
  int n;
  long current_wait = initial_wait;
  unsigned char c;

  /*
   * Interrupts are not working so poll the port to look for
   * ack_N being driven low...
   */

  while(ppc->running) {

    nanowait(current_wait, ppc);
    
    pthread_mutex_lock(&ppc->mutex_port);

    if (ioctl(ppc->fd, PPCLAIM, NULL))
      error("PPCLAIM", ppc);
    c = 0;
    if (ioctl(ppc->fd, PPRSTATUS, &c))
      error("PPRSTATUS", ppc);
    
    if (c & PARPORT_STATUS_ACK) {
      /* No no interrupt pending double the wait...*/

      current_wait=next_wait(current_wait);

    } else {

      /* There is an "interrupt" - read some data */

      modes = IEEE1284_MODE_NIBBLE;
      if (ioctl(ppc->fd, PPNEGOT, &modes))
        error("PPNEGOT IEEE1284_MODE_NIBBLE", ppc);
      
      n = 128;
      if ((r = read(ppc->fd, buf, n)) < 0) {
        if ((errno != EAGAIN)/* && (errno != EWOULBLOCK)*/)
          error("read", ppc);
      }

      pthread_mutex_lock(&ppc->mutex_fifo);
      for (n=0; n<r; n++)
        fifo_add(ppc, buf[n]);
      pthread_mutex_unlock(&ppc->mutex_fifo);

      modes = IEEE1284_MODE_COMPAT;
      if (ioctl(ppc->fd, PPNEGOT, &modes))
        error("PPNEGOT IEEE1284_MODE_COMPAT", ppc);
      
      current_wait = initial_wait;
    }
    
    if (ioctl(ppc->fd, PPRELEASE, NULL))
      error("PPRELEASE 2", ppc);
    pthread_mutex_unlock(&ppc->mutex_port);
  }

  pthread_exit(NULL);
}

struct pp_channel_s *pp_channel_init(void)
{
  pthread_attr_t attr;
  struct ppdev_frob_struct frob;
  struct timespec us50;
  struct timeval tv_us100;
  int modes;

  us50.tv_sec = 0;
  us50.tv_nsec = 50000L;

  tv_us100.tv_sec = 0;
  tv_us100.tv_usec = 100;
  
  struct pp_channel_s *ppc = (struct pp_channel_s *)
    malloc(sizeof(struct pp_channel_s));

  ppc->fifo_size = initial_fifo_size;
  ppc->fifo = (char *) malloc(ppc->fifo_size);
  ppc->fifo_wp = 0;
  ppc->fifo_rp = 0;

  ppc->fd = 0;

  if ((ppc->fd = open("/dev/parport0", O_RDWR | O_NONBLOCK)) < 0)
    error("open", ppc);

  if (ioctl(ppc->fd, PPCLAIM, NULL))
    error("PPCLAIM", ppc);

  if (ioctl(ppc->fd, PPGETPHASE, &modes))
    error("PPGETPHASE", ppc);

  if (ioctl(ppc->fd, PPSETTIME, &tv_us100))
    error("PPSETTIME", ppc);

  /* Initialise the device */
  frob.mask=PARPORT_CONTROL_INIT;
  frob.val = 0;
  if (ioctl(ppc->fd, PPFCONTROL, &frob))
    error("PPFCONTROL", ppc);

  nanowait(50000L, ppc);

  frob.val = PARPORT_CONTROL_INIT;
  if (ioctl(ppc->fd, PPFCONTROL, &frob))
    error("PPFCONTROL", ppc);

  modes = IEEE1284_MODE_COMPAT;
  if (ioctl(ppc->fd, PPNEGOT, &modes))
    error("PPNEGOT IEEE1284_MODE_COMPAT", ppc);

  if (ioctl(ppc->fd, PPRELEASE, NULL))
    error("PPRELEASE", ppc);

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  pthread_mutex_init(&ppc->mutex_port, NULL);
  pthread_mutex_init(&ppc->mutex_fifo, NULL);
  
  /* Start the thread to handle input */
  ppc->running = true;

  if (pthread_create(&ppc->input_thread, &attr, pp_input_thread, ((void*)ppc)))
    error("pthread_create", ppc);
 
  pthread_attr_destroy(&attr);

  return ppc;
}

void pp_channel_done(struct pp_channel_s *ppc)
{
  void *status;

  pthread_mutex_lock(&ppc->mutex_port);
  ppc->running = false; /* Stop at next timeout */
  pthread_mutex_unlock(&ppc->mutex_port);

  if (pthread_join(ppc->input_thread, &status))
    error("pthread_join", ppc);

  if (close(ppc->fd)) {
    ppc->fd = 0;
    error("close", ppc);
  }

  free(ppc->fifo);
  free(ppc);
}

bool pp_channel_can_send(struct pp_channel_s *ppc)
{
  bool r = false;
  unsigned char c;

  pthread_mutex_lock(&ppc->mutex_port);

  if (ioctl(ppc->fd, PPCLAIM, NULL))
    error("PPCLAIM", ppc);

  /* See if the peripheral is busy */
  c = 0;
  if (ioctl(ppc->fd, PPRSTATUS, &c))
    error("PPRSTATUS", ppc);
  
  if (c & PARPORT_STATUS_BUSY) {
    /* Inverted - so here when it's not busy */
    r = true;
  }

  if (ioctl(ppc->fd, PPRELEASE, NULL))
    error("PPRELEASE 1", ppc);
  
  pthread_mutex_unlock(&ppc->mutex_port);
  
  return r;
}

void pp_channel_send(struct pp_channel_s *ppc, const char *buf, int n)
{
  int r;
  int t = n;
  const char *p = buf;
  unsigned char c;
  long current_wait = initial_wait;
 
  while (t>0) {
    pthread_mutex_lock(&ppc->mutex_port);

    if (ioctl(ppc->fd, PPCLAIM, NULL))
      error("PPCLAIM", ppc);

    /* See if the peripheral is busy */
    c = 0;
    if (ioctl(ppc->fd, PPRSTATUS, &c))
      error("PPRSTATUS", ppc);
    
    if (c & PARPORT_STATUS_BUSY) {
      /* Inverted - so here when it's not busy */
      if ((r = write(ppc->fd, p, t)) < 0) {
        if ((errno != EAGAIN)/* && (errno != EWOULBLOCK)*/)
          error("read", ppc);
      }    
      current_wait = initial_wait;
      p += r;
      t -= r;
    } else {
      current_wait=next_wait(current_wait);
    }

    if (ioctl(ppc->fd, PPRELEASE, NULL))
      error("PPRELEASE 1", ppc);
    
    pthread_mutex_unlock(&ppc->mutex_port);

    if (t > 0) {
      nanowait(current_wait, ppc);
    }
  }
}

int pp_channel_num_chars(struct pp_channel_s *ppc)
{
  int r;

  pthread_mutex_lock(&ppc->mutex_fifo);

  r = fifo_fullness(ppc);

  pthread_mutex_unlock(&ppc->mutex_fifo);

  return r;
}

int pp_channel_read(struct pp_channel_s *ppc, char *buf, int len, int sep)
{
  int r = 0;
  int c;

  pthread_mutex_lock(&ppc->mutex_fifo);

  c = -1;

  while (fifo_fullness(ppc) &&
         (r<len) &&
         ((sep<0) || (sep != c))) {
    c = fifo_get(ppc);
    buf[r++] = c;
  }

  if (r < len)
    buf[r] = '\0';

  pthread_mutex_unlock(&ppc->mutex_fifo);
  
  return r;
}
