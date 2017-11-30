#include "udp_packet.h"
#include "../libhorizr/slip.h"
#include "IPv4.h"

udp_packet::udp_packet()
	: source_port{ 0 },
	dest_port{ 0 },
	length{ 0 },
	checksum{ 0 },
	data()
{
	update_length();
	update_checksum();
}

udp_packet::udp_packet(uint16_t _source_port, uint16_t _dest_port, std::vector<uint8_t>& _data)
	: source_port{ _source_port },
	dest_port{ _dest_port },
	length{ 0 },
	checksum{ 0 },
	data(_data)
{
	if (data.size() % 2)
		data.push_back(0);
	update_length();
	update_checksum();
}

udp_packet::~udp_packet()
{
#ifdef DEBUG
	length = 0;
	checksum = 0;
	data.empty();
	update_checksum();
#endif
}

void udp_packet::update_length()
{
	// Add 8 bytes for the header.
	length = data.size() + 8;

	// A UDP packet always has an even number
	// of bytes
	if ((data.size() % 2) == 1)
		length++;
}

// Update the checksum for this packet.
void udp_packet::update_checksum()
{
	// Note that we can't actually compute the real UDP checksum, because
	// for that we'd need the IPv4 header.  But here is a simple checksum
	// we'll need to check for serial line noise.  This algorithm is similar
	// to the UDP checksum algorithm.

	uint32_t sum = source_port + dest_port + length;

	for (size_t i = 0; i < data.size(); i += 2)
	{
		sum += data[i];
		sum += data[i+1] << 8;
	}

	checksum = ~(((sum & 0xFFFF0000) >> 16) | (sum & 0xFFFF));
}

bool udp_packet::is_empty()
{
	if (length <= 8)
		return true;
	return false;
}

// Converts a udp_packet into a byte vector in RFC 1055 SLIP format.
std::vector<uint8_t> udp_packet::to_bytes()
{
	// FIXME: This is very, very wasteful.  One could do this
	// with a single memory allocation.

	std::vector<uint8_t> raw;

	std::vector<uint16_t> header = { source_port, dest_port, length, checksum };
	for (uint16_t x : header)
	{
		raw.push_back(x & 0x00FF);
		raw.push_back((x & 0xFF00) >> 8);
	}

	raw.insert(raw.end(), data.begin(), data.end());

	std::vector<uint8_t> output;
	//slip_encode(raw, output, true);
	return output;
}

// Tries to convert a byte vector that could contain a packet in RFC 1055 SLIP format
// into a udp packet.  It returns the number of bytes processed.  On failure,
// it throws an exception.

size_t udp_packet::from_bytes(std::vector<uint8_t>& raw)
{
	// FIXME: This whole procedure allocates and reallocates vectors.
	// One could do this with a single memory allocation.

	std::vector<uint8_t> input;
	size_t i = 0;
	// i = slip_decode(raw, input, true);

	if (i == 0)
		return 0;
#if 0
	// Skip over any length-1 messages
	while (i == 1)
		i = slip_decode(raw, input, true);

	// Unpack the IPv4 header.
	IPv4 ip_header;
	bool ip_header_valid = ip_header.from_bytes(input);

	// If the IPv4 header is invalid, quit.
	if (!ip_header_valid)
		return i;

	// If the payload is not UDP, quit.
	if (ip_header.protocol != 0x11)
		return i;

	// Strip off the ip_header
	input.erase(input.begin(), input.begin() + ip_header.header_length_words * 4);

	// If we actually got an END byte after at least a header's worth of data.
	if (((input.size() % 2) == 0) && input.size() >= UDP_PACKET_HEADER_SIZE)
	{
		uint16_t _source_port = input[0] | input[1] << 8;
		uint16_t _dest_port = input[2] | input[3] << 8;
		uint16_t _length_expected = input[4] | input[5] << 8;
		uint16_t _checksum_expected = input[6] | input[7] << 8;

		if (_length_expected == input.size())
		{
			// Here we compute the checksum.
			uint32_t sum = _source_port + _dest_port + _length_expected;

			for (size_t j = UDP_PACKET_HEADER_SIZE; j < input.size(); j += 2)
			{
				sum += input[j];
				sum += input[j + 1] << 8;
			}

			uint16_t _checksum_measured = ~(((sum & 0xFFFF0000) >> 16) | (sum & 0xFFFF));
			if (_checksum_measured == _checksum_expected)
			{
				// Everything checks out
				source_port = _source_port;
				dest_port = _dest_port;
				length = _length_expected;
				checksum = _checksum_expected;
				data.clear();
				data.insert(data.begin(), input.begin() + UDP_PACKET_HEADER_SIZE, input.end());
			}
		}
	}
	return i;
#endif
	return 0;
}