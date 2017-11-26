#include "Serial_port.h"
#include <functional>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <strsafe.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <termios.h>
#include <unistd.h>		/* close */
#endif

// The filename of the serial port.  "COM3" for example.
static std::string serial_port_name;

// The hardware baud rate desired for the serial port
static uint32_t baud_rate;

// The desired throughput. Less than or equal to baud rate.
static uint32_t desired_throughput;

std::vector<uint8_t> input_byte_queue;
static const size_t READ_BUFFER_SIZE = 128;
static uint8_t read_buffer[READ_BUFFER_SIZE];

std::vector<uint8_t> output_byte_queue;
static const size_t WRITE_BUFFER_SIZE = 128;
static uint8_t write_buffer[WRITE_BUFFER_SIZE];

#ifdef WIN32
static HANDLE handle;
static bool read_initialized;
static OVERLAPPED oRead = { 0 };
static OVERLAPPED oWrite = { 0 };
static BOOL waiting_on_read = FALSE;
static BOOL waiting_on_write = FALSE;
#endif

static DWORD read_data_bytes_read = 0;


static void throw_win32_error(const char *func_name)
{
	// Retrieve the system error message for the last-error code
	// and throw an exception.

	const size_t message_len_max = 8 * 1024;
	char system_message[message_len_max];
	char output_message[message_len_max];
	DWORD dw = GetLastError();

	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		system_message,
		message_len_max, NULL);

	// Display the error message and exit the process
	snprintf(output_message, message_len_max, "%s failed with error %d: %s", func_name, dw, system_message);
	throw std::exception(output_message);
}

static HANDLE open_win32_port(std::string& serial_port_name)
{
	HANDLE h_serial;
	h_serial = CreateFileA(serial_port_name.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, // Overlapped means asynchronous read/write
		0);
	if (h_serial == INVALID_HANDLE_VALUE)
	{
		std::string err = "Opening serial port " + serial_port_name;
		throw_win32_error(err.c_str());
	}
	return h_serial;
}

void serial_port_init(std::string _name, uint32_t _baud_rate, uint32_t _throttle)
{
	serial_port_name = _name;
	baud_rate = _baud_rate;
	desired_throughput = _throttle;

	HANDLE hReadEvent = CreateEvent(
		NULL,    // default security attribute
		TRUE,    // manual reset event 
		TRUE,    // initial state = unsignaled 
		NULL);   // unnamed event object 
	if (hReadEvent == NULL)
		throw_win32_error("Create event to handle serial port reading");
	oRead.hEvent = hReadEvent;

	HANDLE hWriteEvent = CreateEvent(
		NULL,    // default security attribute
		TRUE,    // manual reset event 
		TRUE,    // initial state = unsignaled 
		NULL);   // unnamed event object 
	if (hWriteEvent == NULL)
		throw_win32_error("Create event to handle serial port reading");
	oWrite.hEvent = hWriteEvent;

	// ResetEvent(async_data.hEvent);
	read_initialized = false;
}

void serial_port_open()
{
	HANDLE h_serial;
	BOOL Status;

	h_serial = open_win32_port(serial_port_name);

	DCB dcbSerialParams = { 0 };                        // Initializing DCB structure
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	Status = GetCommState(h_serial, &dcbSerialParams);     //retreives  the current settings
	if (Status == FALSE)
		throw_win32_error("Getting serial port settings");

#if 0
	printf("\n   Current serial port settings\n");
	printf("\n       Baudrate = %d", dcbSerialParams.BaudRate);
	printf("\n       ByteSize = %d", dcbSerialParams.ByteSize);
	if (dcbSerialParams.StopBits == 0)
		printf("\n       StopBits = 1\n");
	if (dcbSerialParams.StopBits == 1)
		printf("\n       StopBits = 1.5\n");
	if (dcbSerialParams.StopBits == 2)
		printf("\n       StopBits = 2\n");
	if (dcbSerialParams.Parity == 0)
		printf("\n       Parity   = NONE\n");
	if (dcbSerialParams.Parity == 1)
		printf("\n       Parity   = ODD\n");
	if (dcbSerialParams.Parity == 2)
		printf("\n       Parity   = EVEN\n");
#endif

	dcbSerialParams.BaudRate = baud_rate;     
	dcbSerialParams.ByteSize = 8;             // Setting ByteSize = 8
	dcbSerialParams.StopBits = ONESTOPBIT;    // Setting StopBits = 1
	dcbSerialParams.Parity = NOPARITY;      // Setting Parity = None 

	Status = SetCommState(h_serial, &dcbSerialParams);
	if (Status == FALSE)
		throw_win32_error("Adjust serial port settings");

#if 0
	/*------------------------------------ Setting Timeouts --------------------------------------------------*/
	COMMTIMEOUTS timeouts = { 0 };

	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (SetCommTimeouts(h_serial, &timeouts) == FALSE)
		printf("\n   Error! in Setting Time Outs");
	else
		printf("\n\n   Setting Serial Port Timeouts Successfull");
#endif
	handle = h_serial;
}

void serial_port_read(std::vector<uint8_t>& output)
{
	if (output_byte_queue.size() > 0)
	{
		// printf("Moving %d bytes out of serial port output queue\n", output_byte_queue.size());
		for (size_t i = 0; i < output_byte_queue.size(); i++)
		{
			printf(" %02x", output_byte_queue[i]);
		}
		output.insert(output.end(), output_byte_queue.begin(), output_byte_queue.end());
		output_byte_queue.clear();
	}
}

void serial_port_write(std::vector<uint8_t>& input)
{
	input_byte_queue.insert(input_byte_queue.end(), input.begin(), input.end());
}

void serial_port_tick()
{
	// Check to see if anything has completed.
	if (!waiting_on_read)
	{
		// BOOL done = ReadFile(handle, read_buffer, 128, NULL, &oRead);
		BOOL done = ReadFile(handle, read_buffer, 8, NULL, &oRead);
		if (done)
		{
			// Ooh, an instant result.
			if (oRead.InternalHigh > 0)
			{
				for (size_t i = 0; i < oRead.InternalHigh; i++)
				{
					output_byte_queue.push_back(read_buffer[i]);
				}
				// Queue up another read
				oRead.Offset = 0;
				oRead.OffsetHigh = 0;
				oRead.Internal = 0;
				oRead.InternalHigh = 0;
				ResetEvent(oRead.hEvent);
				waiting_on_read = false;
			}
		}
		else
			waiting_on_read = true;
	}

	if (!waiting_on_write && input_byte_queue.size() > 0)
	{
		size_t bytes_enqueued = min(128, input_byte_queue.size());
		memcpy(write_buffer, input_byte_queue.data(), bytes_enqueued);
		input_byte_queue.erase(input_byte_queue.begin(), input_byte_queue.begin() + bytes_enqueued);
		BOOL done = WriteFile(handle, write_buffer, bytes_enqueued, NULL, &oWrite);
		if (done)
		{
			// An instantly completed write.
			if (oWrite.InternalHigh > 0)
			{
				oWrite.Offset = 0;
				oWrite.OffsetHigh = 0;
				oWrite.Internal = 0;
				oWrite.InternalHigh = 0;
				ResetEvent(oWrite.hEvent);
				waiting_on_write = false;
			}
		}
		else
			waiting_on_write = true;
	}

	if (waiting_on_read && waiting_on_write)
	{
		// Check to see if older read/write operations have completed.
		HANDLE h[2] = { oRead.hEvent, oWrite.hEvent };
#define TICK_TIME 100 // milliseconds
		DWORD dw = WaitForMultipleObjects(2, h, FALSE, TICK_TIME);
		if (dw == WAIT_OBJECT_0)
		{
			// Read completed
			if (oRead.InternalHigh > 0)
			{
				for (size_t i = 0; i < oRead.InternalHigh; i++)
				{
					output_byte_queue.push_back(read_buffer[i]);
				}
				oRead.Offset = 0;
				oRead.OffsetHigh = 0;
				oRead.Internal = 0;
				oRead.InternalHigh = 0;
				ResetEvent(oRead.hEvent);
				waiting_on_read = false;
			}
			else
			{
				SetLastError(oRead.Internal);
				throw_win32_error("Serial port read tick");
			}
		}
		else if (dw == WAIT_OBJECT_0 + 1)
		{
			// Write completed
			if (oWrite.InternalHigh > 0)
			{
				oWrite.Offset = 0;
				oWrite.OffsetHigh = 0;
				oWrite.Internal = 0;
				oWrite.InternalHigh = 0;
				ResetEvent(oWrite.hEvent);
				waiting_on_write = false;
			}
			else
			{
				SetLastError(oWrite.Internal);
				throw_win32_error("Serial port write tick");
			}
		}
	}
	else if (waiting_on_read)
	{
		// Check to see if older read/write operations have completed.
		HANDLE h[1] = { oRead.hEvent };
#define TICK_TIME 100 // milliseconds
		DWORD dw = WaitForMultipleObjects(1, h, FALSE, TICK_TIME);
		if (dw == WAIT_OBJECT_0)
		{
			// Read completed
			if (oRead.InternalHigh > 0)
			{
				for (size_t i = 0; i < oRead.InternalHigh; i++)
				{
					output_byte_queue.push_back(read_buffer[i]);
				}
				oRead.Offset = 0;
				oRead.OffsetHigh = 0;
				oRead.Internal = 0;
				oRead.InternalHigh = 0;
				ResetEvent(oRead.hEvent);
				waiting_on_read = false;
			}
			else
			{
				SetLastError(oRead.Internal);
				throw_win32_error("Serial port read tick");
			}
		}
	}
	else if (waiting_on_write)
	{
		// Check to see if older read/write operations have completed.
		HANDLE h[1] = { oWrite.hEvent };
#define TICK_TIME 100 // milliseconds
		DWORD dw = WaitForMultipleObjects(1, h, FALSE, TICK_TIME);
		if (dw == WAIT_OBJECT_0 + 1)
		{
			// Write completed
			if (oWrite.InternalHigh > 0)
			{
				oWrite.Offset = 0;
				oWrite.OffsetHigh = 0;
				oWrite.Internal = 0;
				oWrite.InternalHigh = 0;
				ResetEvent(oWrite.hEvent);
				waiting_on_write = false;
			}
			else
			{
				SetLastError(oWrite.Internal);
				throw_win32_error("Serial port write tick");
			}
		}
	}
}

#if 0
void serial_port_read(std::vector<uint8_t>& output)
{
	// Because of the blocking nature of reading from serial ports,
	// we use the asynchronous API.  This method doesn't
	// 'read' so much as check to see if an asynchronous read
	// has completed.
	if (!waiting_on_read) {
		// Issue read operation.
		if (!ReadFile(handle, read_buffer, READ_BUFFER_SIZE, &read_data_bytes_read, &async_data))
		{
			if (GetLastError() != ERROR_IO_PENDING)     // read not delayed?
				throw_win32_error("Serial port read");
			// Error in communications; report it.
			else
				waiting_on_read = TRUE;
		}
		else
		{
			// The read from the serial port completed immediately.
			// There should now be data in read_buffer.
			for (int i = 0; i < read_data_bytes_read; i++)
			{
				putchar(read_buffer[i]);
				output.push_back(read_buffer[i]);
			}
			waiting_on_read = FALSE;
		}
	}
	else
	{
		// Let's check to see if that read operation is finished yet.
#define WAIT_TIMEOUT 1
		DWORD result = WaitForSingleObject(async_data.hEvent, WAIT_TIMEOUT);
		switch (result)
		{
			// Read completed.
		case WAIT_OBJECT_0:
			if (!GetOverlappedResult(handle, &async_data, &read_data_bytes_read, FALSE))
				// Error in communications; report it.
				// throw_win32_error("Completing serial port read");
				;
			else
			{
				printf("Completed serial port async read of %d bytes\n", read_data_bytes_read);
				// Read completed successfully.
				// There should now be data in read_buffer.
				for (int i = 0; i < read_data_bytes_read; i++)
				{
					putchar(read_buffer[i]);
					output.push_back(read_buffer[i]);
				}
			}
			//  Reset flag so that another opertion can be issued.
			if (ResetEvent(async_data.hEvent) == FALSE)
				throw_win32_error("Resetting serial port read");
			waiting_on_read = FALSE;
			break;

		case WAIT_TIMEOUT:
			// Operation isn't complete yet. waiting_on_read flag isn't
			// changed since I'll loop back around, and I don't want
			// to issue another read until the first one finishes.
			break;

		default:
			// Error in the WaitForSingleObject; abort.
			// This indicates a problem with the OVERLAPPED structure's
			// event handle.
			break;
		}
	}
}

void serial_port_write(std::vector<uint8_t>& input)
{
	unsigned long bytes_written = 0;
	BOOL ret;

#define WRITE_WAIT_TIMEOUT 1000

	if (!waiting_on_write)
	{
		// Issue write operation.
		if (!WriteFile(handle, input.data(), input.size(), &bytes_written, &async_data))
		{
			waiting_on_write = TRUE;

			if (GetLastError() != ERROR_IO_PENDING)     // write not delayed?
				throw_win32_error("Serial port write");
			else
			{
				DWORD result = WaitForSingleObject(async_data.hEvent, WRITE_WAIT_TIMEOUT);
				switch (result)
				{
					// Write completed.
				case WAIT_OBJECT_0:
					if (!GetOverlappedResult(handle, &async_data, &bytes_written, FALSE))
						// Error in communications; report it.
						throw_win32_error("Completing serial port write");
					waiting_on_write = FALSE;
					break;

				case WAIT_TIMEOUT:
					// The operation didn't complete in time, which is odd.
						break;

				default:
					// Error in the WaitForSingleObject; abort.
					break;
				}
			}
		}

		// else
			// The write returned immediately.
			// waiting_on_write is still false, so we're ready for the next round.
	}
	else
	{
		// The port is still busy, so we won't bother writing this data.
		printf("Serial port congestion.");
	}
}


static void read_complete_callback(DWORD error_code, DWORD bytes_read, LPOVERLAPPED async_data)
{
	printf("read complete %d %d\n", error_code, bytes_read);
	// This gets called when the serial port has finished reading a block of data.

	// There are many strange things required to error check this result
	read_data_bytes_read = bytes_read;
}

#endif 

