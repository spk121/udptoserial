#pragma once

/* A struct that holds the information for a UDP packet, plus routines
   to convert that struct to/from a serial-port-friendly string. */

#include <stdint.h>
#include <stdbool.h>

/* Greater than the maximum number of bytes in a serialized message. */
#define UNPACKED_INPUT_BUFFER_MAX (64*1024)

/* START_OF_HEADER + 1-byte protocol + 8 hex digits */
#define PACKED_PKTMSG_HEADER_LEN (10)  

/* full header + START_OF_TEXT + 4 hex digits + base64_string + 4 hex digits + END_OF_TEXT */
#define PACKED_PKTMSG_MAX_LEN (PACKED_MSG_HEADER_LEN + 0xFFFF + 10) 
#define PACKED_PKTMSG_MIN_LEN (PACKED_MSG_HEADER_LEN + 4 + 10)

#define START_OF_HEADER '['
#define START_OF_TEXT '|'
#define END_OF_TEXT ']'

typedef struct _pktmsg_t pktmsg_t;

/* This is an unpacket version of UDP (RFC 768) */
struct _pktmsg_t
{
	/* OPTIONAL: port of the sending process and may be assumed to be the
	   reply port.  Can be zero if no reply is necessary. */
	uint16_t source_port;
	uint16_t dest_port;

	/* Length includes the length of this 8-byte header, so the minimum is 8. */
	uint16_t length;

	/* The UDP's default checksum is a 16-bit one's complement of a one's complement
	   sum of the IP header, the UDP header, and the data, zero padded to
	   an even number of bytes. */
	uint16_t checksum;
	bool valid;
	uint16_t input_port;
	uint16_t output_port;
	unsigned char *data;
	size_t len;
};

size_t pktmsg_to_bytes(pktmsg_t *msg, const char *buf, size_t maxOutputLen);
size_t pktmsg_from_bytes(pktmsg_t *msg, char *buf, size_t maxInputLen);


