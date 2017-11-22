#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Greater than the maximum number of bytes in a serialized message. */
#define UNPACKED_INPUT_BUFFER_MAX (64*1024)

/* START_OF_HEADER + 1-byte protocol + 8 hex digits */
#define PACKED_MSG_HEADER_LEN (10)  

/* full header + START_OF_TEXT + 4 hex digits + base64_string + 4 hex digits + END_OF_TEXT */
#define PACKED_MSG_MAX_LEN (PACKED_MSG_HEADER_LEN + 0xFFFF + 10) 
#define PACKED_MSG_MIN_LEN (PACKED_MSG_HEADER_LEN + 4 + 10)

#define START_OF_HEADER '['
#define START_OF_TEXT '|'
#define END_OF_TEXT ']'

typedef enum _unpacked_msg_protocol_t unpacked_msg_protocol_t;

enum _unpacked_msg_protocol_t
{
	MSG_PROTOCOL_UNKNOWN = 0,
	MSG_PROTOCOL_UDP = 'U',
	MSG_PROTOCOL_TCP = 'T'
};

typedef struct _unpacked_msg_t unpacked_msg_t;

struct _unpacked_msg_t
{
	bool valid;
	uint16_t input_port;
	uint16_t output_port;
	int protocol;
	size_t len;
	unsigned char *data;
	uint16_t crc;
};

size_t unpack_message(unpacked_msg_t *msg, const char *buf, size_t len);
size_t pack_message(unpacked_msg_t *msg, char *buf, size_t len);


