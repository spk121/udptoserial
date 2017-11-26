#pragma once
// INPUT_QUEUE - A two-state FIFO queue.  Raw bytes are received and placed
// at the end of a raw byte FIFO queue. Periodically raw bytes are popped off
// and converted to UDP packets.

#include <vector>
#include "udp_packet.h"
class input_queue
{
public:
	input_queue();
	~input_queue();

	void push_bytes(std::vector<uint8_t>& input);
	int move_bytes_to_packets();
	bool is_packet_available();
	udp_packet pop_packet();

private:
	std::vector<uint8_t> raw;
	std::vector<udp_packet> packets;
};

