#ifndef HORIZR_SLIP
#define HORIZR_SLIP

#include <vector>
//-------1---------2---------3---------4---------5---------6---------7---------8

// This contains procedures that decode and encode bytevectors
// using the RFC 1055 SLIP encoding rules

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

// Given SOURCE, a bytevector that may contain a SLIP-encoded message,
// this function searches source for a complete SLIP-encoded message.
// If one is found, it is decoded SOURCE, appending the result onto DEST.
// If the STRICT flag is true, it will throw an error if SOURCE
// contains an invalid SLIP escape sequence.
// The return value is the number of bytes from SOURCE that were processed.
// If no complete SLIP-packet was found,  the return value is zero.
size_t slip_decode(std::vector<uint8_t>& dest, const std::vector<uint8_t>& source, bool strict);


// Given SOURCE, a bytevector, this procedure encodes it as
// a complete SLIP-encoded message, appending the result onto DEST.
// If INTRODUCE is true, begin the SLIP encoding with END character,
// so that it is delimited on both ends.
// The return value is the length of SOURCE.
size_t slip_encode(std::vector<uint8_t>& dest, const std::vector<uint8_t>& source, bool introduce);

#endif