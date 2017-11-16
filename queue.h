#ifndef U2S_QUEUE_H
#define U2S_QUEUE_H

#include <stdint.h>
#include <time.h>
#include "serial.h"

typedef struct _pkt_queue_t pkt_queue_t;

struct _pkt_queue_t
{
  int64_t timestamp;
  uint16_t source;
  uint16_t dest;
  char *data;
  size_t len;
  pkt_queue_t *next;
  pkt_queue_t *prev;
};

pkt_queue_t *pkt_queue_append(pkt_queue_t *queue,
			      uint16_t source, uint16_t dest, char *payload, size_t len);
pkt_queue_t *pkt_queue_last(pkt_queue_t *queue);
pkt_queue_t *pkt_queue_prepend(pkt_queue_t *queue,
			       uint16_t source, uint16_t dest, char *payload, size_t len);
size_t pkt_queue_length (pkt_queue_t *queue);
size_t pkt_queue_size (pkt_queue_t *queue);
pkt_queue_t *pkt_queue_remove_first(pkt_queue_t *queue);
void pkt_queue_free(pkt_queue_t *queue);
pkt_queue_t *pkt_queue_send (pkt_queue_t *queue, serial_port_t *port);
char *pkt_stringify (pkt_queue_t *queue);

#endif
