#include "stdafx.h"
#include "CppUnitTest.h"
#include "../libhorizr/libhorizr.h"

#include <vector>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace libhorizr_test
{		
	TEST_CLASS(slip)
	{
	public:
		
		TEST_METHOD(EmptyInput)
		{
			// TODO: Your test code here
			std::vector<uint8_t> source;
			std::vector<uint8_t> dest;
			std::vector<uint8_t> expected = { SLIP_END };
			slip_encode(dest, source, true);
			Assert::IsTrue(bytevector_compare(dest, expected) == 0);
		}

		TEST_METHOD(NoEscapesInput)
		{
			// TODO: Your test code here
			std::vector<uint8_t> source = { 'a', 'b', 'c' };
			std::vector<uint8_t> dest;
			std::vector<uint8_t> expected = { SLIP_END, 'a', 'b', 'c', SLIP_END };
			slip_encode(dest, source, true);
			Assert::IsTrue(bytevector_compare(dest, expected) == 0);
		}

		TEST_METHOD(EscapeEscapeInput)
		{
			// TODO: Your test code here
			std::vector<uint8_t> source = { SLIP_ESC };
			std::vector<uint8_t> dest;
			std::vector<uint8_t> expected = { SLIP_END, SLIP_ESC, SLIP_ESC_ESC, SLIP_END };
			slip_encode(dest, source, true);
			Assert::IsTrue(bytevector_compare(dest, expected) == 0);
		}

		TEST_METHOD(EscapeEndInput)
		{
			// TODO: Your test code here
			std::vector<uint8_t> source = { SLIP_END };
			std::vector<uint8_t> dest;
			std::vector<uint8_t> expected = { SLIP_END, SLIP_ESC, SLIP_ESC_END, SLIP_END };
			slip_encode(dest, source, true);
			Assert::IsTrue(bytevector_compare(dest, expected) == 0);
		}
	};
}