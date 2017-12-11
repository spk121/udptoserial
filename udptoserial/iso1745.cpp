#include "iso1745.h"
#include <ctype.h>

namespace Iso1745
{
	Peer::Peer()
		: state_{ State::Neutral }
		, outbox_ {}
	{

	}

	Peer::tick()
	{

	}

	// Search the received bytes for a complete message.
	// The formats are
	// prefix + EOT
	// prefix + ENQ
	// prefix + ACK
	// prefix + NAK
	// prefix + DLE + EOT
	// STX + text + ETX + checksum
	// STX + text + ETB + checksum
	// SOH + heading + STX + text + ETX + checksum
	// SOH + heading + STX + text + ETB + checksum
	// SOH + heading + ETB + checksum

	// prefix is zero to 15 ASCII graphical characters
	// The transmission control characters are SOH, STX, ETX, EOT, ENQ, ACK, DLE, NAK, SYN or ETB
	// heading is ASCII text not including any of transmission control characters
	// text is ASCII text not includeing any of the transmissio control characters
	// checksum is the ISO1155 block check character, which is the XOR of all the bytes
	//  in a message except the starting STX or SOH.

#define PREFIX_LEN_MAX 15
	void Peer::get_next_message(std::vector<uint8_t> msg)
	{
		char prefix[PREFIX_LEN_MAX + 1];
		// extract a prefix, if present
		int i = 0;
		uint8_t c;
		while (i <= PREFIX_LEN_MAX && isgraph((c = msg.at(i))))
			prefix[i++] = c;
		if (i > PREFIX_LEN_MAX)
		{
			// Prefix too long
			return false;
		}
		switch (msg.at[i])
		{
		case SOH:
		case STX:
			// Remember prefixes not allowed for these
			parse_information_msg();
			break;
		case EOT:
		case ENQ:
		case ACK:
		case NAK:
		case DLE:
			parse_supervisory_msg();
			break;
		case SYN:
			parse_syn();
			break;
		default:
			// Invalid message.
			break;
		}

	}

	// Remember that the initial STX or SOH character in an information
	// message should not be computed in the checksum calculation.
	uint8_t Peer::compute_iso1155_checksum(std::vector<uint8_t> msg)
	{
		uint32_t sum = 0;
		for (uint8_t x : msg)
		{
			sum += x;
		}
		return (uint8_t)(((sum & 0xFFu) + 1u) & 0xFFu);
	}
}

