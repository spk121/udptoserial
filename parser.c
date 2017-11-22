#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "base64.h"

static int hexdigit_to_val(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else
		abort();
}


static unsigned short crc16(const unsigned char* data_p, int length)
{
	unsigned char x;
	unsigned short crc = 0xFFFF;

	while (length--) {
		x = crc >> 8 ^ *data_p++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x << 5)) ^ ((unsigned short)x);
	}
	return crc;
}

static int extract_hexword(const char *buf)
{
	if (isxdigit(buf[0]) && isxdigit(buf[1]) && isxdigit(buf[2]) && isxdigit(buf[3]))
	{
		int val = 0;
		val = hexdigit_to_val(buf[0]) * 4096
			+ hexdigit_to_val(buf[1]) * 256
			+ hexdigit_to_val(buf[2]) * 16
			+ hexdigit_to_val(buf[3]);
		return val;
	}
	else
		return -1;
}

/* Given a buffer BUF of max length LEN, search for a complete, valid message. 
   The return value is the number of bytes processed.  The unpacked message will,
   be placed into the MSG structure.  If the packed message is invalid, the VALID
   flag of the MSG structure will be FALSE. */
size_t unpack_message(unpacked_msg_t *msg, const char *buf, size_t len)
{
	size_t soh, stx, etx;
	assert(msg != NULL);
	assert(buf != NULL);
	assert(len > 0);

	/* Pull of a message from the buffer, with paranoid error checking. */
	msg->valid = false;

	/* First, drop everything before START_OF_HEADER. */
	soh = 0;
	while (soh < len)
	{
		if (buf[soh] == START_OF_HEADER)
			break;
		soh++;
	}
	if (soh == len)
	{
		printf("No START_OF_HEADER found in %d bytes.\n", soh);
		return soh;
	}
	if (soh != 0)
		printf("%d bytes of junk before START_OF_HEADER\n", soh - 1);
	etx = soh + 1;
	while (etx < len)
	{
		if (buf[etx] == END_OF_TEXT)
			break;
		etx++;
	}
	if (etx == len)
	{
		printf("No END_OF_TEXT found in %d bytes\n", etx - soh);
		if (etx - soh + 1 > PACKED_MSG_MAX_LEN)
		{
			printf("Max possible message length exceeded.\n");
			return etx + 1;
		}
		else
		{
			return soh;
		}
	}
	if (etx - soh + 1 < PACKED_MSG_MIN_LEN)
	{
		printf("Message minimum length not reached: %d < %d\n", etx - soh + 1, PACKED_MSG_MIN_LEN);
		return etx + 1;
	}
	stx = soh;
	while (stx < etx)
	{
		if (buf[stx] == START_OF_TEXT)
			break;
		stx++;
	}
	if (stx == etx)
	{
		printf("No START_OF_TEXT found in message\n");
		return etx + 1;
	}
	if (stx - soh != PACKED_MSG_HEADER_LEN)
	{
		printf("Message header has incorrect length: %d != %d\n", stx - soh, PACKED_MSG_HEADER_LEN);
		return etx + 1;
	}

	/* PROTOCOL */
	unpacked_msg_protocol_t protocol = buf[soh + 1];
	if ((protocol != MSG_PROTOCOL_UDP) && (protocol != MSG_PROTOCOL_TCP))
	{
		printf("Unknown message protocol: '%c'\n", protocol);
		return etx + 1;
	}
	msg->protocol = protocol;

	/* INPUT PORT */
	int iport = extract_hexword(buf + soh + 2);
	if (iport >= 0)
		msg->input_port = iport;
	else
	{
		printf("Invalid input port string '%c%c%c%c'\n", buf[soh + 2], buf[soh + 3], buf[soh + 4], buf[soh + 5]);
		return etx + 1;
	}

	/* OUTPUT PORT */
	int oport = extract_hexword(buf + soh + 6);
	if (oport >= 0)
		msg->output_port = oport;
	else
	{
		printf("Invalid output port string '%c%c%c%c'\n", buf[soh + 6], buf[soh + 7], buf[soh + 8], buf[soh + 9]);
		return etx + 1;
	}

	/* DATA */
	int dlen = extract_hexword(buf + stx + 1);
	if (dlen < 0)
	{
		printf("Invalid data length string '%c%c%c%c'\n", buf[stx + 1], buf[stx + 2], buf[stx + 3], buf[stx + 4]);
		return etx + 1;
	}
	if (etx - stx - 9 != dlen)
	{
		printf("Invalid payload length: %d != %d\n", etx - stx - 9, dlen);
		return etx + 1;
	}
	int crc_expected = extract_hexword(buf + etx - 4);
	if (crc_expected < 0)
	{
		printf("Invalid CRC string '%c%c%c%c'\n", buf[etx - 4], buf[etx - 3], buf[etx - 2], buf[etx - 1]);
		return etx + 1;
	}
	unsigned short crc_measured = crc16(buf + soh, etx - soh - 4);
	if(crc_expected != (int) crc_measured)
	{
		printf("CRC mismatch: %04x != %04x\n", crc_measured, crc_expected);
		return etx + 1;
	}
	size_t out_len;
	msg->crc = crc_expected;
	msg->len = dlen;
	msg->data = base64_decode(buf + stx + 5, etx - stx - 9, &out_len);
	msg->valid = true;
	return etx + 1;
}

/* Given a packed MSG, this creates a packed representation of the MSG,
   and places it into BUF, a buffer that has at least LEN bytes.
   The return value is the number of bytes of the packed representation
   of the message, which may be zero if the MSG is invalid. */
size_t pack_message(unpacked_msg_t *msg, char *buf, size_t len)
{
	char *payload_str = NULL;
	size_t payload_str_len = 0;

	assert(msg != NULL);
	assert(buf != NULL);
	assert(len > 0);

	if (msg->valid == false || msg->len == 0)
		return 0;

	payload_str = base64_encode(msg->data, msg->len, &payload_str_len);

	size_t packed_msg_max_len = payload_str_len
		+ 2 /* brackets */
		+ 5 /* separators */
		+ 1 /* protocol code */
		+ 3 * 5 /* 16-bit numbers as ASCII text */
		+ 4 /* 16-bit number as hex ASCII text */
		+ 2 /* CRLF */
		;
	unsigned char *packed_msg = (unsigned char *)malloc(packed_msg_max_len + 1);
	snprintf(packed_msg, packed_msg_max_len,
		"%c%c%04x%04x%c%04x%s",
		START_OF_HEADER,
		(char)msg->protocol,
		msg->input_port,
		msg->output_port,
		START_OF_TEXT,
		strlen(payload_str),
		payload_str);
	free(payload_str);

	/* Compute a CRC on the string so far. */
	unsigned short crc = crc16(packed_msg, strlen(packed_msg));
	char crcbuf[8];
	sprintf(crcbuf, "%04x%c\r\n", crc, END_OF_TEXT);
	strncat(packed_msg, crcbuf, strlen(crcbuf));

	/* OK, if we're here, there is a valid packed message in PACKED_MSG. */
	size_t packed_len = strlen(packed_msg);
	assert(packed_len < len);
	strncpy(buf, packed_msg, packed_len);
	free(packed_msg);

	unpack_message(msg, buf, strlen(buf));
	return packed_len;
}
