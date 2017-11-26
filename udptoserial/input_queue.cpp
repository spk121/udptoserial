#include "input_queue.h"



input_queue::input_queue()
	: raw{},
	packets{}
{
}

input_queue::~input_queue()
{
}

void input_queue::push_bytes(std::vector<uint8_t>& input)
{
	raw.insert(raw.end(), input.begin(), input.end());
	move_bytes_to_packets();
}

// If the raw data queue contains any complete messages,
// extract them into the packet queue.
int input_queue::move_bytes_to_packets()
{
	size_t bytes_used;
	int count = 0;

	while (raw.size() > 0)
	{
		udp_packet p;
		bytes_used = p.from_bytes(raw);
		raw.erase(raw.begin(), raw.begin() + bytes_used);
		if (!p.is_empty())
		{
			packets.push_back(p);
			count++;
		}
		else
			break;
	}
	return count;
}

// If there a packet ready to go?
bool input_queue::is_packet_available()
{
	return packets.size() > 0;
}

// Return the first ready packet, removing it
// from the queue.
udp_packet input_queue::pop_packet()
{
	if (packets.size() > 0)
	{
		udp_packet p = packets[0];
		packets.erase(packets.begin(), packets.begin() + 1);
		return p;
	}
	return udp_packet();
}