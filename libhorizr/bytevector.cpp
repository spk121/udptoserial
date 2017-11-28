#include "bytevector.h"

#include <algorithm>

int bytevector_compare(const std::vector<uint8_t>& A, const std::vector<uint8_t>& B)
{
	size_t len = std::min(A.size(), B.size());
	for (size_t i = 0; i < len; ++i)
	{
		if (A[i] != B[i])
			return (A[i] < B[i] ? -1 : 1);
	}
	if (A.size() < B.size())
		return 1;
	else if (A.size() > B.size())
		return -1;

	return 0;
}
