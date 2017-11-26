#pragma once
#include <vector>
// This contains procedures that decode and encode bytevectors
// using the RFC 1055 SLIP encoding rules


// Decode SOURCE, appending the result onto DEST.  The bytes processed
// in SOURCE are removed.  It returns the number of bytes processed.
// If there is no SLIP END character in SOURCE, SOURCE will not be
// modified and the return value is zero.
size_t slip_decode(std::vector<uint8_t>& source, std::vector<uint8_t>& dest);


// Encode SOURCE, appending the result onto DEST.  The bytes processed
// in SOURCE are removed.  It is assumed that SOURCE contains one
// complete message to be encoded as SLIP. Returns the number of bytes
// in source.
size_t slip_encode(std::vector<uint8_t>& source, std::vector<uint8_t>& dest);
