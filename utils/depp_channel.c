#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

#include <dpcdecl.h>
#include <depp.h>
#include <dmgr.h>

#include "depp_channel.h"

static const int initial_fifo_size = 0x0400;
static const long initial_wait = 10000; // 10us - 100kHz
static const long max_wait = 10000000;  // 10ms - 100Hz

#define ADDR_CTRLSTAT 0
#define ADDR_ASRKBD   1
#define ADDR_ASRTYP   2

static inline long next_wait(long current_wait)
{
  long r = current_wait << 1;
  if (r > max_wait)
    r = max_wait;
  return r;
}

struct depp_channel_s {
  HIF hif;
  pthread_t input_thread;
  pthread_mutex_t mutex_port;
  bool running;
  char *fifo;
  int fifo_size;
  int fifo_wp;
  int fifo_rp;
  pthread_mutex_t mutex_fifo;
};

static void error(const char *msg, struct depp_channel_s *ppc)
{
  fprintf(stderr, "%s\n", msg);

  if (ppc->hif != hifInvalid) {
    DeppDisable(ppc->hif);
    DmgrClose(ppc->hif);
  }

  exit(1);
}

static void nanowait(long tv_nsec, struct depp_channel_s *ppc)
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

static inline int fifo_fullness(struct depp_channel_s *ppc)
{
  int n = ppc->fifo_wp - ppc->fifo_rp;
  if (n<0)
    n+=ppc->fifo_size;
  return n;
}

static inline int fifo_space(struct depp_channel_s *ppc)
{
  return (ppc->fifo_size-1) - fifo_fullness(ppc);
}

static inline char fifo_get(struct depp_channel_s *ppc)
{
  char c = 0;

  if (fifo_fullness(ppc))
    c = ppc->fifo[ppc->fifo_rp++];
  if (ppc->fifo_rp >= ppc->fifo_size)
    ppc->fifo_rp = 0;
  return c;
}

static inline void fifo_add(struct depp_channel_s *ppc, char c)
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


static void *depp_input_thread(void *p)
{
  struct depp_channel_s *ppc = ((struct depp_channel_s *) p);
  int i, n;
  char buf[129];
  long current_wait = initial_wait;
  BYTE d;

  /*
   * Interrupts are not working so poll the port to look for
   * ack_N being driven low...
   */

  while(ppc->running) {
    
    nanowait(current_wait, ppc);
    
    pthread_mutex_lock(&ppc->mutex_port);
    
    if(!DeppGetReg(ppc->hif, ADDR_CTRLSTAT, &d, fFalse))
      error("depp_input_thread: DeppGetReg: Failed to read status register.", ppc);
    //printf("depp_input_thread: Status = 0x%02x\n", ((int) d));

    if (!(d & 1)) {
      /* No, no character waiting double the wait...*/
      
      current_wait = next_wait(current_wait);
      //printf("current_wait=%ld\n", current_wait);

    } else {

      /* There is a character - read some data */
      i = 0;
      do {
	//printf("i=%d\n", i);
	//printf("(cs) d = 0x%02x\n", d);
	d = 0;
	if(!DeppGetReg(ppc->hif, ADDR_ASRTYP, &d, fFalse))
	  error("DeppGetReg: Failed to read ASR typewriter register.", ppc);
	
	//printf("d = 0x%02x\n", d);

	buf[i++] = d;
	
	if(!DeppGetReg(ppc->hif, ADDR_CTRLSTAT, &d, fFalse))
	  error("depp_input_thread: DeppGetReg (2): Failed to read status register.", ppc);
	
      } while ((i<128) && (d & 1));
      
      pthread_mutex_lock(&ppc->mutex_fifo);
      for (n=0; n<i; n++)
        fifo_add(ppc, buf[n]);
      pthread_mutex_unlock(&ppc->mutex_fifo);
      
      current_wait = initial_wait;
    }
    
    pthread_mutex_unlock(&ppc->mutex_port);
  }

  pthread_exit(NULL);
}

struct depp_channel_s *depp_channel_init(void)
{
  pthread_attr_t attr;

  struct depp_channel_s *ppc = (struct depp_channel_s *)
    malloc(sizeof(struct depp_channel_s));

  ppc->hif = hifInvalid;
  ppc->fifo_size = initial_fifo_size;
  ppc->fifo = (char *) malloc(ppc->fifo_size);
  ppc->fifo_wp = 0;
  ppc->fifo_rp = 0;

  if(!DmgrOpen(&(ppc->hif), "CmodS6")) {
    fprintf(stderr, "DmgrOpen failed (check the device name you provided)\n");
    return NULL;
  }

  if(!DeppEnable(ppc->hif)) {
    fprintf(stderr, "DeppEnable failed\n");
    return NULL;
  }

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  pthread_mutex_init(&ppc->mutex_port, NULL);
  pthread_mutex_init(&ppc->mutex_fifo, NULL);
  
  /* Start the thread to handle input */
  ppc->running = true;

  if (pthread_create(&ppc->input_thread, &attr, depp_input_thread, ((void*)ppc)))
    error("pthread_create", ppc);
 
  pthread_attr_destroy(&attr);

  return ppc;
}

void depp_channel_done(struct depp_channel_s *ppc)
{
  void *status;

  pthread_mutex_lock(&ppc->mutex_port);
  ppc->running = false; /* Stop at next timeout */
  pthread_mutex_unlock(&ppc->mutex_port);

  if (pthread_join(ppc->input_thread, &status))
    error("pthread_join", ppc);

  if (ppc->hif != hifInvalid) {
    DeppDisable(ppc->hif);
    DmgrClose(ppc->hif);
  }

  free(ppc->fifo);
  free(ppc);
}

bool depp_channel_can_send(struct depp_channel_s *ppc)
{
  bool r = false;
  BYTE d;
  
  pthread_mutex_lock(&ppc->mutex_port);
  
  //if(!DeppGetReg(ppc->hif, ADDR_CTRLSTAT, &d, fFalse))
  //  error("depp_channel_can_send: DeppGetReg: Failed to read status register.", ppc);
  d = 0;

  if (d & 2)
    r = true;

  pthread_mutex_unlock(&ppc->mutex_port);
  
  return r;
}

void depp_channel_send(struct depp_channel_s *ppc, const char *buf, int n)
{
  int i;
  int t = n;
  const char *p = buf;
  BYTE d;
  long current_wait = initial_wait;
 
  while (t>0) {
    pthread_mutex_lock(&ppc->mutex_port);

    if(!DeppGetReg(ppc->hif, ADDR_CTRLSTAT, &d, fFalse))
      error("DeppGetReg: Failed to read status register.", ppc);

    i = 0;
    if (d & 2) {
      while ((d & 2) && (i < t)) {
	
	d = p[i++];
	
	if(!DeppPutReg(ppc->hif, ADDR_ASRKBD, d, fFalse))
	  error("DeppPutReg: Failed to write ASR keyboard register.", ppc);
	
	if(!DeppGetReg(ppc->hif, ADDR_CTRLSTAT, &d, fFalse))
	  error("DeppGetReg: Failed to read status register.", ppc);
      }
      
      current_wait = initial_wait;
      p += i;
      t -= i;
    } else {
      current_wait=next_wait(current_wait);
    }

    pthread_mutex_unlock(&ppc->mutex_port);

    if (t > 0) {
      nanowait(current_wait, ppc);
    }
  }
}

int depp_channel_num_chars(struct depp_channel_s *ppc)
{
  int r;

  pthread_mutex_lock(&ppc->mutex_fifo);

  r = fifo_fullness(ppc);

  pthread_mutex_unlock(&ppc->mutex_fifo);

  return r;
}

int depp_channel_read(struct depp_channel_s *ppc, char *buf, int len, int sep)
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
