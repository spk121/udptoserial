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

#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/serial_port.hpp>
#include "../libhorizr/ip.h"

using namespace boost;

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

#include <deque>

std::shared_ptr<asio::io_service> service_;
std::shared_ptr<asio::io_service::strand> write_strand_;
asio::streambuf in_packet_;
std::deque<std::string> send_packet_queue_;
std::shared_ptr<asio::serial_port> serial_port_;

static void serial_port_init(std::shared_ptr<asio::io_service> io_service, std::string serial_port_name, int baud_rate)
{
	serial_port_ = std::make_shared<asio::serial_port>(io_service);
	serial_port_->open(serial_port_name);
	serial_port_->set_option(asio::serial_port_base::baud_rate(baud_rate));
	service_ = io_service;
	write_strand_ = std::make_shared<asio::io_service::strand>(io_service);


	serial_port_->async_read_some
	(asio::mutable_buffers_1(serial_read_buffer_raw_, SERIAL_READ_BUFFER_SIZE),
		serial_read_handler);



}

////////////////////////////////////////////////
// The following four serial_port_send functions
// form an async queueing and sending chain.

static void serial_port_send_tcp(asio::ip::tcp::endpoint& source,
	asio::ip::tcp::endpoint& dest,
	std::string payload)
{
	struct ip_hdr iphdr;
	struct tcp_hdr tcphdr;
	ip_tcp_headers_default_set(&iphdr, &tcphdr,
		htonl(source.address().to_v4().to_ulong()),
		htons(source.port()),
		htonl(dest.address().to_v4().to_ulong()),
		htons(source.port()),
		(uint8_t *)payload.c_str(), payload.size());
	uint8_t *binary_msg = (uint8_t *)malloc(sizeof(iphdr) + sizeof(tcphdr) + payload.size());
	memcpy(binary_msg, &iphdr, sizeof(iphdr));
	memcpy(binary_msg + sizeof(iphdr), &tcphdr, sizeof(tcphdr));
	memcpy(binary_msg + sizeof(iphdr) + sizeof(tcphdr), payload.c_str(), payload.size());
	std::string binary_str((char *)binary_msg, sizeof(iphdr) + sizeof(tcphdr) + payload.size());
	free(binary_msg);
	// slip
	service_->post(write_strand_->wrap([binary_str]()
	{
		serial_port_queue_message(binary_str);
	}));
}

void serial_port_queue_message(std::string message)
{
	bool write_in_progress = !send_packet_queue_.empty();
	send_packet_queue_.push_back(std::move(message));
	if (!write_in_progress)
	{
		serial_port_start_packet_send();
	}
}

void serial_port_start_packet_send()
{
	send_packet_queue_.front() += "\0";
	async_write(*serial_port_
		, asio::buffer(send_packet_queue_.front())
		, write_strand_->wrap([](system::error_code const & ec, std::size_t)
		{
			serial_port_packet_send_done(ec);
		}
	));
}

void serial_port_packet_send_done(system::error_code const & error)
{
	if (!error)
	{
		send_packet_queue_.pop_front();
		if (!send_packet_queue_.empty())
		{ 
			serial_port_start_packet_send(); 
		}
	}
}

///////////////////////////
// The following functions form the async serial port read handler



void serial_read_handler(
	const boost::system::error_code& error, // Result of operation.
	std::size_t bytes_transferred           // Number of bytes read.
)
{
	serial_read_buffer_unprocessed_.insert(serial_read_buffer_unprocessed_.end(), (unsigned char *)&serial_read_buffer_raw_[0], serial_read_buffer_raw_ + bytes_transferred);
	// BOOST_LOG_TRIVIAL(debug) << "serial port has " << serial_read_buffer_unprocessed_.size() << " bytes";

	// See if this is a complete SLIP message
	std::vector<uint8_t> slip_msg;
	size_t bytes_decoded;
	bytes_decoded = slip_decode(slip_msg, serial_read_buffer_unprocessed_, false);
	if (bytes_decoded > 0)
	{
		serial_read_buffer_unprocessed_.erase(serial_read_buffer_unprocessed_.begin(), serial_read_buffer_unprocessed_.begin() + bytes_decoded);

		bool ret = ip_bytevector_validate(slip_msg);
		if (ret)
		{
			if (ip_bytevector_is_udp(slip_msg))
				BOOST_LOG_TRIVIAL(debug) << "Valid slip-decoded UDP message of " << bytes_decoded << " bytes";
			else if (ip_bytevector_is_tcp(slip_msg))
			{
				struct ip_tcp_hdr *phdr = (struct ip_tcp_hdr*)slip_msg.data();

				// Make sure that the data in the sin_port and sin_addr are
				// in network byte order.
				asio::ip::tcp::endpoint saddr(asio::ip::address_v4(phdr->_ip_hdr.saddr), phdr->_tcp_hdr.source_port);
				asio::ip::tcp::endpoint daddr(asio::ip::address_v4(phdr->_ip_hdr.daddr), phdr->_tcp_hdr.destination_port);
				slip_msg.erase(slip_msg.begin(), slip_msg.begin() + ip_bytevector_data_start(slip_msg));
				tcp_send(saddr, daddr, slip_msg);

				// ADD read handler here.

				serial_port_->async_read_some(MutableBuffer, Read Handler);
					(asio::mutable_buffers_1(serial_read_buffer_raw_, SERIAL_READ_BUFFER_SIZE),
							serial_read_handler);
			}
			else
				BOOST_LOG_TRIVIAL(debug) << "Valid slip-decoded message of " << bytes_decoded << " bytes";
		}
		else
			BOOST_LOG_TRIVIAL(debug) << "Invalid slip decoded message of " << bytes_decoded << " bytes";
	}
	// And queue up the next async read

	serial_port_->async_read_some
	(asio::mutable_buffers_1(serial_read_buffer_raw_, SERIAL_READ_BUFFER_SIZE),
		serial_read_handler);
}

#if 0

// Now we prep this for transmission, using the following mapping.
// From our point of view, this socket is local_address:server_port.
// It is a forwarder for a server on destination_address:server_port.
// It received a packet from remote_address:client_port.
// The packet's SOURCE is remote_address:client_port.
// The packet's DEST is local_address:server_port.

// When we construct a forwarding packet
// That packet's SOURCE is remote_address:client_port
// That packet's DEST is destination_address:server_port

// destination_address is from the .ini file 
struct ip4_hdr iphdr;
struct tcp_hdr tcphdr;
ip4_tcp_headers_default_set(&iphdr, &tcphdr,
	htonl(socket_.remote_endpoint().address().to_v4().to_ulong()),
	htons(socket_.remote_endpoint().port()),
	htonl(remote_addr_BE_),
	htons(socket_.local_endpoint().port()),
	(uint8_t *)packet_string.c_str(), packet_string.size());
uint8_t *binary_msg = (uint8_t *)malloc(sizeof(iphdr) + sizeof(tcphdr) + packet_string.size());
memcpy(binary_msg, &iphdr, sizeof(iphdr));
memcpy(binary_msg + sizeof(iphdr), &tcphdr, sizeof(tcphdr));
memcpy(binary_msg + sizeof(iphdr) + sizeof(tcphdr), packet_string.c_str(), packet_string.size());
std::string binary_str((char *)binary_msg, sizeof(iphdr) + sizeof(tcphdr) + packet_string.size());
free(binary_msg);
printf("----\n");
for (auto c : binary_str)
{
	printf("%02x ", (unsigned)c);
}
printf("\n");
for (auto c : binary_str)
{
	if (c < 32)
		printf("^%c", c + 64);
	else
		printf("%c", c);
}
printf("\n");
std::vector<uint8_t> source;
for (auto x : binary_str)
source.push_back((uint8_t)x);
std::vector<uint8_t> dest;
slip_encode(dest, source, true);
std::string dest_str;
for (auto x : dest)
dest_str.push_back(x);
send(dest_str);
read_packet();
}

void Tcp_client_handler::send(std::string msg)
{
	service_.post(write_strand_.wrap([me = shared_from_this(), msg]()
	{
		me->queue_message(msg);
	}));
}


void Tcp_client_handler::packet_send_done(system::error_code const & error)
{
	if (!error)
	{
		send_packet_queue_.pop_front();
		if (!send_packet_queue_.empty()) { start_packet_send(); }
	}
}

void Tcp_client_handler::start_packet_send()
{
	send_packet_queue_.front() += "\0";
	async_write(*serial_port_
		, asio::buffer(send_packet_queue_.front())
		, write_strand_.wrap([me = shared_from_this()]
		(system::error_code const & ec
			, std::size_t)
	{
		me->packet_send_done(ec);
	}
	));
}

void Tcp_client_handler::queue_message(std::string message)
{
	bool write_in_progress = !send_packet_queue_.empty();
	send_packet_queue_.push_back(std::move(message));
	if (!write_in_progress)
	{
		start_packet_send();
	}
}

#endif