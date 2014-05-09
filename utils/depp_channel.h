#ifndef _DEPP_CHANNEL_H_
#define _DEPP_CHANNEL_H_

#include <stdbool.h>

struct depp_channel_s;

#ifdef __cplusplus
 extern "C" {
#endif

struct depp_channel_s *depp_channel_init(void);
void depp_channel_done(struct depp_channel_s *ppc);

bool depp_channel_can_send(struct depp_channel_s *ppc);
void depp_channel_send(struct depp_channel_s *ppc, const char *buf, int n);

int depp_channel_num_chars(struct depp_channel_s *ppc);
int depp_channel_read(struct depp_channel_s *ppc, char *buf, int len, int sep);

#ifdef __cplusplus
 }
#endif

#endif
