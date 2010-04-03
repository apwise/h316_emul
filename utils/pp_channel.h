#ifndef _PP_CHANNEL_H_
#define _PP_CHANNEL_H_

#include <stdbool.h>

struct pp_channel_s;

#ifdef __cplusplus
 extern "C" {
#endif

struct pp_channel_s *pp_channel_init(void);
void pp_channel_done(struct pp_channel_s *ppc);

bool pp_channel_can_send(struct pp_channel_s *ppc);
void pp_channel_send(struct pp_channel_s *ppc, const char *buf, int n);

int pp_channel_num_chars(struct pp_channel_s *ppc);
int pp_channel_read(struct pp_channel_s *ppc, char *buf, int len, int sep);

#ifdef __cplusplus
 }
#endif

#endif
