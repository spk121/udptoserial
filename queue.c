#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <sys/time.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "queue.h"
#include "base64.h"

static unsigned short crc16(const unsigned char* data_p, int length);

/* This is a queue of packets to be sent down the serial pipe. */

static int64_t get_time_now_usec()
{
#ifdef WIN32
	SYSTEMTIME tval;
	FILETIME ftval;
	ULARGE_INTEGER uftval;
#else
	struct timeval tval;
#endif

#ifdef WIN32
	GetSystemTime(&tval);
	SystemTimeToFileTime(&tval, &ftval);
	uftval.HighPart = ftval.dwHighDateTime;
	uftval.LowPart = ftval.dwLowDateTime;
	/* Convert to microseconds. */
	return (int64_t) uftval.QuadPart / 10LL;
#else
	if (gettimeofday(&tval, NULL) < 0)
	{
		perror("gettimeofday");
		return -1;
	}
	else
		return (int64_t)tval.tv_sec * 1000000LL + (int64_t)tval.tv_usec;
#endif

}

/* Append new packet information to the end of the packet queue. */
pkt_queue_t *pkt_queue_append(pkt_queue_t *queue,
			      uint16_t source, uint16_t dest, char *payload, size_t len)
{
  pkt_queue_t *new_queue;
  pkt_queue_t *last;
  
  new_queue = (pkt_queue_t *) malloc(sizeof(pkt_queue_t));
  new_queue->timestamp = get_time_now_usec();
  new_queue->source = source;
  new_queue->dest = dest;
  new_queue->data = (char *)malloc(len + 1);
  memcpy(new_queue->data, payload, len);
  new_queue->data[len] = '\0';
  new_queue->len = len;
  new_queue->next = NULL;

  if (queue)
    {
      last = pkt_queue_last(queue);
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

pkt_queue_t *pkt_queue_last(pkt_queue_t *queue)
{
  if (queue == NULL)
    return NULL;

  while (queue->next != NULL)
    queue = queue->next;

  return queue;
}

/* Append new packet information to the beginning of the packet queue. */
pkt_queue_t *pkt_queue_prepend(pkt_queue_t *queue,
			    uint16_t source, uint16_t dest, char *payload, size_t len)
{
  pkt_queue_t *new_queue;

  new_queue = (pkt_queue_t *)malloc(sizeof(pkt_queue_t));
  new_queue->timestamp = get_time_now_usec();
  new_queue->source = source;
  new_queue->dest = dest;
  memcpy(new_queue->data, payload, len);
  new_queue->len = len;
  new_queue->next = queue;

  if (queue)
    queue->prev = new_queue;
  else
    new_queue->prev = NULL;

  return new_queue;
}

size_t pkt_queue_length (pkt_queue_t *queue)
{
  if (queue == NULL)
    return 0;
  int i = 1;
  while (queue->next == NULL)
    {
      queue = queue->next;
      i++;
    }
  return i;
}

size_t pkt_queue_size (pkt_queue_t *queue)
{
  if (queue == NULL)
    return 0;
  size_t siz = sizeof(pkt_queue_t) + queue->len;
  while (queue->next == NULL)
    {
      queue = queue->next;
      siz += sizeof(pkt_queue_t) + queue->len;
    }
  return siz;
}


pkt_queue_t *pkt_queue_remove_first(pkt_queue_t *queue)
{
  pkt_queue_t *next;

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

void pkt_queue_free(pkt_queue_t *queue)
{
  while ((queue = pkt_queue_remove_first (queue)) != NULL)
    ;
}

size_t pkt_estimated_string_length (pkt_queue_t *queue)
{
  return (4 			/* send port */
	  + 4			/* recv port */
	  + 4			/* separators */
	  + 2			/* CRC16 */
	  + queue->len * 4 / 3);	/* Base64 packed payload */
}

pkt_queue_t *pkt_queue_send (pkt_queue_t *queue, serial_port_t *port)
{
  if (queue == NULL)
    return NULL;

  int64_t time_now = get_time_now_usec();

  if (port)
  {
	  if (((time_now - queue->timestamp) * 1000000 * port->baud_rate) < (int64_t)pkt_estimated_string_length(queue))
	  {
		  /* Delay sending packet to avoid flooding the port. */
		  return queue;
	  }
  }

  char *str = pkt_stringify (queue);
  serial_port_send (port, str, strlen(str));
  free (str);
  return pkt_queue_remove_first (queue);
}

char *pkt_stringify (pkt_queue_t *queue)
{
  size_t payload_str_len;
  char *payload_str = base64_encode(queue->data, queue->len, &payload_str_len);
  
  size_t str_len = payload_str_len
	  + 2 /* brackets */
	  + 4 /* separators */
	  + 3 * 5 /* 16-bit numbers as ascii */
	  + 4 /* 16-bit number as hex */
	  + 2 /* CRLF */
	  ;
  
  char *str = (char *) malloc (str_len + 1);
  str[0] = '\0';

  /* The packet begins with a SOH character. */
  snprintf (str, str_len,
	    "[%d:%d:%d:%s:",
	    queue->source,
	    queue->dest,
	  strlen(payload_str),
	    payload_str
	    );
  free (payload_str);

  /* Compute a CRC on the string so far. */
  unsigned short crc = crc16 (str, strlen(str));
  char crcbuf[8];
  sprintf(crcbuf, "%04x]\r\n", crc);
  strncat (str, crcbuf, str_len);

  return str;
}

static unsigned short crc16(const unsigned char* data_p, int length)
{
    unsigned char x;
    unsigned short crc = 0xFFFF;

    while (length--){
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
    }
    return crc;
}



  
