#ifndef _chain_buffer_h
#define _chain_buffer_h
#include <stdint.h>

typedef struct buf_chain_s buf_chain_t;
typedef struct buffer_s buffer_t;


buffer_t * buffer_new(uint32_t sz);

uint32_t buffer_len(buffer_t *buf);

int buffer_add(buffer_t *buf, const void *data, uint32_t datlen);

int buffer_remove(buffer_t *buf, void *data, uint32_t datlen);

int buffer_drain(buffer_t *buf, uint32_t len);

void buffer_free(buffer_t *buf);

int buffer_search(buffer_t *buf, const char* sep, const int seplen);

uint8_t * buffer_write_atmost(buffer_t *p);

#endif
