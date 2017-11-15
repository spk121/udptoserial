/* This is a queue of packets to be sent down the serial pipe. */

typedef struct _pkt_queue pkt_queue;

struct _pkt_queue
{
  time_t timestamp;
  uint16_t source_port;
  uint16_t dest_port;
  char *data;
  size_t len;
  PQueue *next;
  PQueue *prev;
};

void pkt_queue_free(pkt_queue *queue);
pkt_queue *pkt_queue_append (pkt_queue *queue, uint16_t source, uint16_t dest, char *payload, size_t len);
pkt_queue *pkt_queue_prepend (pkt_queue *queue, uint16_t source, uint16_t dest, char *payload, size_t len);
size_t pkt_queue_length (pkt_queue *lst);
pkt_queue *pkt_queue_remove_first(pkt_queue *queue);

/* Append new packet information to the end of the packet queue. */
pkt_queue *pkt_queue_append(pkt_queue *queue,
			    uint16_t source, uint16_t dest, char *payload, size_t len)
{
  pkt_queue *new_queue;
  pkt_queue *last;
  new_queue = malloc(sizeof(pkt_queue));
  new_queue->source = source;
  new_queue->dest = dest;
  new_queue->data = memcpy(payload, len);
  new_queue->len = len;
  new_queue->next = NULL;

  if (queue)
    {
      last = pkg_queue_last(list);
      last->next = new_queue;
      new_queue->prev = last;

      return queue;
    }
  else
    {
      new_queue->prev = NULL;
      return new_queue;
    }
}

/* Append new packet information to the beginning of the packet queue. */
pkt_queue *pkt_queue_prepend(pkt_queue *queue,
			    uint16_t source, uint16_t dest, char *payload, size_t len)
{
  pkt_queue *new_queue;
  pkt_queue *last;
  
  new_queue = malloc(sizeof(pkt_queue));
  new_queue->source = source;
  new_queue->dest = dest;
  new_queue->data = memcpy(payload, len);
  new_queue->len = len;
  new_queue->next = queue;

  if (queue)
    {
      new_queue->prev = queue->prev;
      if (queue->prev)
	queue->prev->next = new_queue;
      queue->prev = new_queue;
    }
    else
      new_queue->prev = NULL;

  return new_queue;
}

pkt_queue *pkt_queue_remove_first(pkt_queue *queue)
{
  pkt_queue *next;

  if (queue == NULL)
    return NULL;
  
  free (queue->data);

  if (queue->next == NULL)
    return NULL;

  next = queue->next;
  next->prev = NULL;
  free(queue);
  return next;
}



  
  
