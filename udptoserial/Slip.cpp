#include "Slip.h"

// This byte indicates the end of a packet.
const uint8_t SLIP_END = 192;
// The byte used to indicate the beginning of a two-byte escape sequence.
const uint8_t SLIP_ESC = 219;
// The two-byte sequence 219 SLIP_ESC + 220 SLIP_ESC_END unpacks as 192 SLIP_END,
// without indicating the end of a packet.
const uint8_t SLIP_ESC_END = 220;
// The two-byte sequence 219 SLIP_ESC + 221 SLIP_ESC_ESC unpacks as 219 SLIP_ESC,
// without indicating a new escape sequence.
const uint8_t SLIP_ESC_ESC = 221;

// Decode SOURCE, appending the result onto DEST.  The bytes processed
// in SOURCE are removed.  It returns the number of bytes processed.
// If there is no SLIP END character in SOURCE, SOURCE will not be
// modified and the return value is zero.
size_t slip_decode(std::vector<uint8_t>& source, std::vector<uint8_t>& dest)
{
	size_t size = source.size();
	size_t i = 0;
	uint8_t c, c2;
	bool is_terminated = false;

	// Search for proper termination
	while (i < size)
	{
		c = source[i];

		if (c != SLIP_END && c != SLIP_ESC)
			i++;
		else if (c == SLIP_ESC)
		{
			// Here we handle some escape sequences that are encoded as two bytes, but,
			// unpack as a single byte.
			if (i + 1 < size)
				i += 2;
			else
				break;
		}
		else if (c == SLIP_END)
		{
			// If it is an END character, then we're done with the packet.
			is_terminated = true;
			break;
		}
	}

	if (!is_terminated)
		return 0;

	// Reset the position to the beginning.
	i = 0;

	while (i < size)
	{
		c = source[i];
		if (c != SLIP_END && c != SLIP_ESC)
		{
			// Usually, the input byte passes through unchanged.
			dest.push_back(c);
			i++;
		}
		else if (c == SLIP_ESC)
		{
			// Here we handle some escape sequences that are encoded as two bytes, but,
			// unpack as a single byte.
			if (i + 1 < size)
			{
				c2 = source[i + 1];
				if (c2 == SLIP_ESC_END)
					dest.push_back(SLIP_END);
				else if (c2 == SLIP_ESC_ESC)
					dest.push_back(SLIP_ESC);
				else
					// This is a protocol violation. RFC 1055 recommends ignoring the
					// violation and continuing.
					dest.push_back(c2);
				i += 2;
			}
			else
				break;
		}
		else if (c == SLIP_END)
		{
			// If it is an END character, then we're done with the packet.
			break;
		}
		else
			abort();
	}
	source.erase(source.begin(), source.begin() + i);
	return i;
}

// Encode SOURCE, appending the result onto DEST.  The bytes processed
// in SOURCE are removed.  It is assumed that SOURCE contains one
// complete message to be encoded as SLIP. Returns the number of bytes
// in source.
size_t slip_encode(std::vector<uint8_t>& source, std::vector<uint8_t>& dest)
{
	int i = 0;
	dest.push_back(SLIP_END);
	for (uint8_t x : source)
	{
		i++;
		if (x == SLIP_END)
		{
			dest.push_back(SLIP_ESC);
			dest.push_back(SLIP_ESC_END);
		}
		else if (x == SLIP_ESC)
		{
			dest.push_back(SLIP_ESC);
			dest.push_back(SLIP_ESC_ESC);
		}
		else
			dest.push_back(x);
	}
	dest.push_back(SLIP_END);
	source.erase(source.begin(), source.begin() + i);
	return i;
}