#pragma once
// UDP_PACKET - a class that holds and serializes the information contained in a UDP packet.

#include <cstdint>
#include <vector>

#if 0
// This byte indicates the end of a packet.
extern const uint8_t SLIP_END = 192;
// The byte used to indicate the beginning of a two-byte escape sequence.
extern const uint8_t SLIP_ESC = 219;
// The two-byte sequence 219 SLIP_ESC + 220 SLIP_ESC_END unpacks as 192 SLIP_END,
// without indicating the end of a packet.
extern const uint8_t SLIP_ESC_END = 220;
// The two-byte sequence 219 SLIP_ESC + 221 SLIP_ESC_ESC unpacks as 219 SLIP_ESC,
// without indicating a new escape sequence.
extern const uint8_t SLIP_ESC_ESC = 221;
#endif

const size_t UDP_PACKET_HEADER_SIZE = 8;

class udp_packet
{
public:
	udp_packet();
	udp_packet(uint16_t _source_port, uint16_t _dest_port, std::vector<uint8_t>& _data);
	~udp_packet();

	void update_length();
	void update_checksum();
	bool is_empty();

	// Update the UDP packet if bytevector VEC contains a SLIP-encoded UDP packet.
	size_t from_bytes(std::vector<uint8_t> &vec);

	// Return the packet as a SLIP-encoded bytevector.
	std::vector<uint8_t> to_bytes();

public:
	uint16_t source_port;
	uint16_t dest_port;
	uint16_t checksum;

	// This length is a UDP packet length, which includes the
	// size of the 8 byte header and may include one padding
	// byte if the length of data is odd.
	uint16_t length;
	std::vector<uint8_t> data;
};

