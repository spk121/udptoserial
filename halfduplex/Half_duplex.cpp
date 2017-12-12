#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "Half_duplex.h"
#include <string>

#define LONG_TIMEOUTS
namespace Serial
{
#ifdef LONG_TIMEOUTS
	const static auto NO_RESPONSE_TIMEOUT = posix_time::seconds(30);
	const static auto NO_RECEIVE_TIMEOUT = posix_time::seconds(20);
	const static auto NO_ACTIVITY_TIMEOUT = posix_time::seconds(200);
	const static auto POLL_TIMEOUT = posix_time::seconds(7);
#else
	const static auto NO_RESPONSE_TIMEOUT = posix_time::seconds(3);
	const static auto NO_RECEIVE_TIMEOUT = posix_time::seconds(2);
	const static auto NO_ACTIVITY_TIMEOUT = posix_time::seconds(20);
#endif
	const static size_t MAX_PREFIX_LEN = 15;
	const static size_t MAX_MESSAGE_LEN = 1550;
	const static size_t MAX_ENQ_TRIES = 20;
	const static size_t MAX_INFO_TRIES = 20;
	const static char SOH = 1;
	const static char STX = 2;
	const static char ETX = 3;
	const static char EOT = 4;
	const static char ENQ = 5;
	const static char ACK = 6;
	const static char DLE = 16;
	const static char NAK = 21;
	const static char SYN = 22;
	const static char ETB = 23;

	const std::string CONTROL_CHARACTERS = "\x01\x02\x03\x04\x05\x06\x10\x15\x16\x17";
	const std::string MSG_START_CHARACTERS = "\x01\x02\x04\x05\x06\x10\x15";

	std::string to_string(State s)
	{
		if (s == State::NEUTRAL)
			return "NEUTRAL";
		else if (s == State::POLL_TRANSMIT)
			return "POLL_TRANSMIT";

		else if (s == State::MASTER_SELECT_TRANSMIT)
			return "MASTER_SELECT_TRANSMIT";
		else if (s == State::MASTER_SELECT_ACK_RECEIVE)
			return "MASTER_SELECT_ACK_RECEIVE";
		else if (s == State::MASTER_INFO_TRANSMIT)
			return "MASTER_INFO_TRANSMIT";
		else if (s == State::MASTER_INFO_ACK_RECEIVE)
			return "MASTER_INFO_ACK_RECEIVE";

		else if (s == State::SLAVE_SELECT_RECEIVE)
			return "SLAVE_SELECT_RECEIVE";
		else if (s == State::SLAVE_SELECT_ACK_TRANSMIT)
			return "SLAVE_SELECT_ACK_TRANSMIT";
		else if (s == State::SLAVE_INFO_RECEIVE)
			return "SLAVE_INFO_RECEIVE";
		else if (s == State::SLAVE_INFO_ACK_TRANSMIT)
			return "SLAVE_INFO_ACK_TRANSMIT";

		else
			return "UNKNOWN_STATE";
	}

	std::string to_string(Msg_type s)
	{
		if (s == Msg_type::NONE)
			return "NONE";
		else if (s == Msg_type::ACK)
			return "ACK";
		else if (s == Msg_type::NAK)
			return "NAK";
		else if (s == Msg_type::EOT)
			return "EOT";
		else if (s == Msg_type::DLE_EOT)
			return "DLE_EOT";
		else if (s == Msg_type::ENQ)
			return "ENQ";
		else if (s == Msg_type::INFO)
			return "INFO";
		else if (s == Msg_type::MALFORMED)
			return "MALFORMED";
		else
			return "UNKNOWN_TYPE";
	}

	static std::string to_string(Msg& m)
	{
		std::string out;
		if (!m.prefix.empty())
			out.insert(out.end(), m.prefix.begin(), m.prefix.end());
		if (m.type == Msg_type::ACK)
			out.push_back(ACK);
		else if (m.type == Msg_type::NAK)
			out.push_back(NAK);
		else if (m.type == Msg_type::EOT)
			out.push_back(EOT);
		else if (m.type == Msg_type::ENQ)
			out.push_back(ENQ);
		else if (m.type == Msg_type::DLE_EOT)
		{
			out.push_back(DLE);
			out.push_back(EOT);
		}
		else if (m.type == Msg_type::INFO)
		{
			// The first location in the string from which the block-check character (an 8-bit CRC) is computed.
			size_t bcc_start_pos;

			if (!m.header.empty())
			{
				out.push_back(SOH);
				bcc_start_pos = out.size();
				out.insert(out.end(), m.header.begin(), m.header.end());
				out.push_back(STX);
			}
			else
			{
				out.push_back(STX);
				bcc_start_pos = out.size();
			}
			if (!m.body.empty())
				out.insert(out.cend(), m.body.begin(), m.body.end());
			out.push_back(ETX);

			// The computation of the block-check character (an 8-bit CRC).
			uint32_t sum = 0;
			for (auto x : out.substr(bcc_start_pos))
				sum += (uint8_t)x;
			out.push_back((char)(uint8_t)(((sum & 0xFFu) + 1u) & 0xFFu));
		}
		else
			abort();
		return out;
	}


	Half_duplex::Half_duplex(asio::io_service& ios, const std::string & device, unsigned int _baud_rate, bool ctrl)
		: port_(ios, device)
		, accepting_input_(true)
		, no_response_timer_(ios)
		, no_receive_timer_(ios)
		, no_activity_timer_(ios)
		, poll_timer_(ios)
		, enq_nak_count_(0)
		, info_nak_count_(0)
		, controller_(ctrl)
		, state_(State::NEUTRAL)
	{
		port_.set_option(asio::serial_port_base::baud_rate(_baud_rate));

		// Start up the async read handler.
		auto callback = std::bind(&Half_duplex::read_handler, this, _1, _2);
		port_.async_read_some
		(asio::mutable_buffers_1(input_raw_, RAW_READ_BUFFER_SIZE),
			callback);
		poll_timer_.expires_from_now(POLL_TIMEOUT);
		auto func = std::bind(&Half_duplex::poll, this, _1);
		poll_timer_.async_wait(func);
	}

	// This is the public API that adds a message to the output queue.
	void Half_duplex::enqueue_message(std::string header, std::string body)
	{
		BOOST_LOG_TRIVIAL(debug) << "enqueue_message called";

		Msg m;
		m.type = Msg_type::INFO;
		m.header = header;
		m.body = body;
		io_mutex_.lock();
		output_queue_.push_back(m);
		io_mutex_.unlock();

		BOOST_LOG_TRIVIAL(debug) << "there are " << output_queue_.size() << " messages in the output queue";
	}

	// This is the public API that gets a message from the input queue.
	bool Half_duplex::get_next_message(std::string& header, std::string& body)
	{
		BOOST_LOG_TRIVIAL(debug) << "get_next_message called with " << input_queue_.size() << " message(s) in the input queue";

		Msg g;
		bool ret = false;

		io_mutex_.lock();
		if (input_queue_.size() > 0)
		{
			g = input_queue_.front();
			input_queue_.pop_front();
			header = g.header;
			body = g.body;
			ret = true;
		}
		io_mutex_.unlock();
		return ret;
	}

	
	void Half_duplex::read_handler(const system::error_code& error,
		size_t bytes_transferred)
	{
		input_unprocessed_.insert(input_unprocessed_.cend(),
			(unsigned char *)&input_raw_[0],
			input_raw_ + bytes_transferred);

		// Receipt of any characters resets the No Activity Timer.
		no_activity_timer_.expires_from_now(NO_ACTIVITY_TIMEOUT);

		// Scan the unprocessed buffer for any complete ISO-1745 messages.
		// If any complete messages are found, put them in the message queue.
		while (true)
		{
			Msg m = parse_input();
			if (m.type == Msg_type::NONE)
				// No complete message ready to go, so keep waiting.
				break;

			BOOST_LOG_TRIVIAL(debug) << "read_handler: state " << to_string(state_) << ", message " << to_string(m.type);

			// Otherwise, handle messages based on the current state.
			switch (state_)
			{
			case State::NEUTRAL:
				handle_message_neutral_state(m);
				break;
			case State::SLAVE_SELECT_RECEIVE:
				handle_message_slave_select_receive_state(m);
				break;
			case State::MASTER_SELECT_ACK_RECEIVE:
				handle_message_master_select_ack_receive_state(m);
				break;
			case State::MASTER_INFO_ACK_RECEIVE:
				handle_message_master_info_ack_receive_state(m);
				break;
			case State::SLAVE_INFO_RECEIVE:
				handle_message_slave_info_receive_state(m);
				break;
			default:
				abort();
			}
		}

		// Start up another async read handler. 
		auto callback = std::bind(&Half_duplex::read_handler, this, _1, _2);
		port_.async_read_some
		(asio::mutable_buffers_1(input_raw_, RAW_READ_BUFFER_SIZE),
			callback);
	}


	inline void Half_duplex::write_simple_msg(Msg_type typ)
	{
		write_simple_msg(typ, "");
	}

	inline void Half_duplex::write_simple_msg(Msg_type typ, std::string prefix)
	{
		Msg m;
		m.prefix = prefix;
		m.type = typ;
		std::string m_str = to_string(m);
		asio::write(port_, asio::buffer(m_str));
	}
	
	inline void Half_duplex::write_output_queue_msg()
	{
		last_message_ = output_queue_.front();
		output_queue_.pop_front();
		std::string reply_str = to_string(last_message_);
		asio::write(port_, asio::buffer(reply_str));
	}

	inline void Half_duplex::handle_output_queue_msg_and_state()
	{
		if (output_queue_.empty())
		{
			BOOST_LOG_TRIVIAL(debug) << "transmit empty queue";
			write_simple_msg(Msg_type::EOT);
			change_state(State::NEUTRAL);
		}
		else
		{
			BOOST_LOG_TRIVIAL(debug) << "transmit, " << output_queue_.size() << " messages in queue";
			change_state(State::MASTER_INFO_TRANSMIT);
			write_output_queue_msg();
			change_state(State::MASTER_INFO_ACK_RECEIVE);
		}
	}

	void Half_duplex::handle_message_neutral_state(Msg& m)
	{
		// Normally, the non-supervisor peer is waiting for the supervisor
		// to let me know that I can become the Master station.
		// (The functionality where the supervisor peer becomes the master station
		// is elsewhere in poll().)
		if (m.type == Msg_type::ENQ && m.prefix == "bravo")
		{
			if (accepting_input_)
			{
				change_state(State::MASTER_SELECT_TRANSMIT);
				write_simple_msg(Msg_type::ACK);
				change_state(State::MASTER_SELECT_ACK_RECEIVE);
			}
			else
			{
				change_state(State::MASTER_SELECT_TRANSMIT);
				write_simple_msg(Msg_type::EOT);
				change_state(State::NEUTRAL);
			}
		}
		else
			throw std::runtime_error("invalid message in NEUTRAL");
	}

	void Half_duplex::handle_message_slave_select_receive_state(Msg& m)
	{
		// Normally, I am the supervisor and I've sent out a poll ENQ.
		// I'm expecting to receive a select ENQ directed at alpha (aka me) to which I'll respond with
		// an ACK.
		if (m.type == Msg_type::ENQ && m.prefix == "alpha")
		{
			if (accepting_input_)
			{
				BOOST_LOG_TRIVIAL(debug) << "accepting selection by bravo";
				change_state(State::SLAVE_SELECT_ACK_TRANSMIT);
				write_simple_msg(Msg_type::ACK);
				change_state(State::SLAVE_INFO_RECEIVE);
			}
			else
			{
				change_state(State::SLAVE_SELECT_ACK_TRANSMIT);
				write_simple_msg(Msg_type::NAK);
				change_state(State::NEUTRAL);
			}
		}
		else if (m.type == Msg_type::EOT)
		{
			if (controller_)
				BOOST_LOG_TRIVIAL(debug) << "bravo rejects selection";
			change_state(State::NEUTRAL);
		}
		else
			throw std::runtime_error("invalid message for SLAVE_SELECT_RECEIVE state");
	}

	void Half_duplex::handle_message_master_select_ack_receive_state(Msg& m)
	{
		// Normally, I've sent out an ENQ to request to be the master
		// station, and I'm waiting for an ACK so I can start to send
		// data.
		if (m.type == Msg_type::ACK)
		{
			BOOST_LOG_TRIVIAL(debug) << "peer accepted my selection as master";
			enq_nak_count_ = 0;
			handle_output_queue_msg_and_state();
		}
		else if (m.type == Msg_type::NAK)
		{
			// If the other end rejects me, normally jump back to
			// neutral, unless this has happened too many times.
			if (controller_)
			{
				BOOST_LOG_TRIVIAL(debug) << "peer rejected my selection as master";

				enq_nak_count_++;
				if (enq_nak_count_ < MAX_ENQ_TRIES)
				{
					change_state(State::POLL_TRANSMIT);
				}
				else
				{
					BOOST_LOG_TRIVIAL(warning) << "Exceeded max number of tries to poll " << m.prefix;
					change_state(State::NEUTRAL);
				}
			}
			else
				change_state(State::NEUTRAL);
		}
		else if (m.type == Msg_type::DLE_EOT)
		{
			handle_clear_request();
		}
		else if (m.type == Msg_type::EOT)
		{
			change_state(State::NEUTRAL);
		}
		else
			throw std::runtime_error("Invalid msg type");
	}

	// void Half_duplex::handle_message_master_transmit_state(Msg& m)
	// {
	//   // I'm not expecting any messages right now.  It is supposed to
	//   // be my turn to send INFO messages.
	//     BOOST_LOG_TRIVIAL(debug) << "handle_message_master_transmit_state(" << (int)m.type << ")";
	//   if (m.type == Msg_type::DLE_EOT)
	//     handle_clear_request();
	//   else
	//     {
	//// Invalid.  Try to recover.
	//Msg reply;
	//reply.type = Msg_type::EOT;
	//std::string m_str = to_string(m);
	//asio::write(port_, asio::buffer(m_str));
	//change_state(State::NEUTRAL);
	//maybe_select();
	//     }
	// }

	void Half_duplex::handle_message_master_info_ack_receive_state(Msg& m)
	{
		// I've sent an INFO message, and I'm expecting an ACK.
		if (m.type == Msg_type::ACK)
		{
			BOOST_LOG_TRIVIAL(debug) << "peer accepted my info message";
			no_response_timer_.cancel();
			info_nak_count_ = 0;
			handle_output_queue_msg_and_state();
		}
		else if (m.type == Msg_type::NAK)
		{
			BOOST_LOG_TRIVIAL(debug) << "peer rejected my info message";
			// Slave wants the previous message resent
			no_response_timer_.cancel();
			info_nak_count_++;
			if (info_nak_count_ < MAX_INFO_TRIES)
				retransmit();
			else
			{
				// Try to recover.
				write_simple_msg(Msg_type::EOT);
				change_state(State::NEUTRAL);
			}
		}
		else if (m.type == Msg_type::DLE_EOT)
			handle_clear_request();
		else
		{
			// Invalid.  Try to recover.
			write_simple_msg(Msg_type::EOT);
			change_state(State::NEUTRAL);
		}
	}

	void Half_duplex::handle_message_slave_info_receive_state(Msg& m)
	{
		// I'm expecting for an INFO message from master.
		BOOST_LOG_TRIVIAL(debug) << "handle_message_slave_info_receive_state(" << to_string(m.type) << ")";
		if (m.type == Msg_type::INFO)
		{
			BOOST_LOG_TRIVIAL(debug) << "accepting valid INFO message";
			// FIXME: add some sort of thresholds where I can choose to EOT
			// if I have received too many messages.

			change_state(State::SLAVE_INFO_ACK_TRANSMIT);
			io_mutex_.lock();
			input_queue_.push_back(m);
			io_mutex_.unlock();
			write_simple_msg(Msg_type::ACK);
			change_state(State::SLAVE_INFO_RECEIVE);
		}
		else if (m.type == Msg_type::MALFORMED)
		{
			BOOST_LOG_TRIVIAL(debug) << "rejecting malformed message";
			change_state(State::SLAVE_INFO_ACK_TRANSMIT);
			write_simple_msg(Msg_type::NAK);
			change_state(State::SLAVE_INFO_RECEIVE);
		}
		else if (m.type == Msg_type::EOT)
		{
			// A graceful request to return to neutral
			change_state(State::NEUTRAL);
		}
		else if (m.type == Msg_type::DLE_EOT)
			handle_clear_request();
		else
			throw std::runtime_error("Invalid msg type");
	}

	void Half_duplex::handle_clear_request()
	{
		BOOST_LOG_TRIVIAL(debug) << "clearing the channel";
		// Clear all the queues.
		input_unprocessed_.clear();
		io_mutex_.lock();
		input_queue_.clear();
		output_queue_.clear();
		io_mutex_.unlock();
		change_state(State::NEUTRAL);
	}


	// Scan the unprocessed buffer for complete messages.
	Msg Half_duplex::parse_input()
	{
		Msg msg;
		msg.type = Msg_type::NONE;
		char introducer;

		if (input_unprocessed_.empty())
			return msg;

		// There can be up to 15 prefix characters.
		size_t pos_intro = input_unprocessed_.find_first_of(MSG_START_CHARACTERS);
		if (pos_intro == std::string::npos)
		{
			if (input_unprocessed_.size() > MAX_PREFIX_LEN)
			{
				BOOST_LOG_TRIVIAL(debug) << "Prefix too long";
				input_unprocessed_.clear();
				msg.type = Msg_type::MALFORMED;
			}
			return msg;
		}

		// Prefix contains non-graphic characters
		if (!all_of(input_unprocessed_.cbegin(),
			input_unprocessed_.cbegin() + pos_intro,
			isgraph))
		{
			BOOST_LOG_TRIVIAL(debug) << "Prefix contains non-graphical characters";
			msg.type = Msg_type::MALFORMED;
			input_unprocessed_.clear();
			return msg;
		}

		msg.prefix.insert(msg.prefix.begin(),
			input_unprocessed_.cbegin(),
			input_unprocessed_.cbegin() + pos_intro);
		introducer = input_unprocessed_[pos_intro];

		if (introducer == ENQ)
		{
			msg.type = Msg_type::ENQ;
			input_unprocessed_.erase(0, pos_intro + 1);;
			return msg;
		}
		else if (introducer == EOT)
		{
			msg.type = Msg_type::EOT;
			input_unprocessed_.erase(0, pos_intro + 1);
			return msg;
		}
		else if (introducer == ACK)
		{
			msg.type = Msg_type::ACK;
			input_unprocessed_.erase(0, pos_intro + 1);
			return msg;
		}
		else if (introducer == NAK)
		{
			msg.type = Msg_type::NAK;
			input_unprocessed_.erase(0, pos_intro + 1);
			return msg;
		}

		// The only remaining choice is information messages
		if (introducer != SOH && introducer != STX)
			abort();

		// Information messages don't have prefixes
		if (!msg.prefix.empty())
		{
			BOOST_LOG_TRIVIAL(debug) << "Invalid prefix for information message";
			input_unprocessed_.clear();
			msg.type = Msg_type::MALFORMED;
			return msg;
		}

		size_t pos_stx;
		if (introducer == STX)
			pos_stx = pos_intro;
		else
			pos_stx = input_unprocessed_.find(STX);
		size_t pos_etx = input_unprocessed_.find(ETX);
		if (pos_etx == std::string::npos)
		{
			// No end of message found
			if (input_unprocessed_.size() > MAX_MESSAGE_LEN)
			{
				BOOST_LOG_TRIVIAL(debug) << "Info message with missing ETX";
				msg.type = Msg_type::MALFORMED;
				input_unprocessed_.clear();
				return msg;
			}
			else
				return msg;
		}

		if (pos_stx == std::string::npos || pos_stx > pos_etx)
		{
			BOOST_LOG_TRIVIAL(debug) << "Info message with missing STX";
			input_unprocessed_.clear();
			msg.type = Msg_type::MALFORMED;
			return msg;
		}

		if (input_unprocessed_.size() == pos_etx + 1)
		{
			// Haven't received the checksum yet
			return msg;
		}

		if (pos_etx > MAX_MESSAGE_LEN)
		{
			BOOST_LOG_TRIVIAL(debug) << "Info message too long";
			msg.type = Msg_type::MALFORMED;
			input_unprocessed_.clear();
			return msg;
		}

		if (pos_stx > pos_intro)
			msg.header.insert(msg.header.begin(),
				input_unprocessed_.cbegin() + 1,
				input_unprocessed_.cbegin() + pos_stx);
		msg.body.insert(msg.body.begin(),
			input_unprocessed_.cbegin() + pos_stx + 1,
			input_unprocessed_.cbegin() + pos_etx);
		uint8_t cksum = (uint8_t)input_unprocessed_[pos_etx + 1];

		// The checksum computation skips the first SOH or STX, but
		// includes the ETX or EOB.
		if (cksum != compute_cksum(pos_intro + 1, pos_etx + 1))
		{
			BOOST_LOG_TRIVIAL(debug) << "Info message checksum error";
			input_unprocessed_.clear();
			msg.type = Msg_type::MALFORMED;
			return msg;
		}

		// This is a valid info message
		msg.type = Msg_type::INFO;
		input_unprocessed_.erase(0, pos_etx + 2);
		return msg;
	}

	uint8_t Half_duplex::compute_cksum(size_t begin, size_t end)
	{
		uint32_t sum = 0;
		// FIXME: unnecessary allocation of substring.
		for (auto x : input_unprocessed_.substr(begin, end))
			sum += x;

		return (uint8_t)(((sum & 0xFFu) + 1u) & 0xFFu);
	}


	// If we are in NEUTRAL, and we are the control station, let's see if anyone
	// needs to be a master station.
	void Half_duplex::poll(const system::error_code& ec)
	{
		BOOST_LOG_TRIVIAL(debug) << "in poll()";
		poll_timer_.expires_from_now(POLL_TIMEOUT);
		auto func = std::bind(&Half_duplex::poll, this, _1);
		poll_timer_.async_wait(func);

		if (state_ != State::NEUTRAL)
		{
			BOOST_LOG_TRIVIAL(debug) << "not polling because state is " << to_string(state_);
			return;
		}

		if (!controller_)
		{
			BOOST_LOG_TRIVIAL(debug) << "not polling because not the controlling peer";
			return;
		}

		// If our output queue is not empty, we'll become the master
		// station.
		if (!output_queue_.empty())
		{
			// First, send out an EOT to make sure the peer is in neutral
			BOOST_LOG_TRIVIAL(debug) << "poll(): supervisor sends initial EOT";
			write_simple_msg(Msg_type::EOT);

			// Second, we inform us and the peer that we are becoming master station
			{
				change_state(State::POLL_TRANSMIT);
				BOOST_LOG_TRIVIAL(debug) << "poll(): supervisor sends poll ENQ to alpha";
				write_simple_msg(Msg_type::ENQ, "alpha");
			}
			// Third, we query the peer if it wants to connect with us
			{
				change_state(State::MASTER_SELECT_TRANSMIT);
				BOOST_LOG_TRIVIAL(debug) << "poll(): supervisor sends select ENQ to bravo";
				write_simple_msg(Msg_type::ENQ, "bravo");
			}

			no_response_timer_.expires_from_now(NO_RESPONSE_TIMEOUT);
			auto func = std::bind(&Half_duplex::on_master_select_no_response_timeout, this, _1);
			no_response_timer_.async_wait(func);
			change_state(State::MASTER_SELECT_ACK_RECEIVE);
		}
		else
		{
			// If our output queue is empty, we can ask the peer if it wants to be
			// the master station
			// First, send out an EOT to make sure the peer is in neutral
			BOOST_LOG_TRIVIAL(debug) << "poll(): supervisor sends initial EOT";
			write_simple_msg(Msg_type::EOT);

			// Second, we inform the peer that they are becoming master station
			{
				change_state(State::POLL_TRANSMIT);
				BOOST_LOG_TRIVIAL(debug) << "poll(): supervisor sends poll ENQ to bravo";
				write_simple_msg(Msg_type::ENQ, "bravo");
			}
			// FIXME: need a polling timeout
			// Now we wait for the peer's select request
			change_state(State::SLAVE_SELECT_RECEIVE);
		}
	}

	//// We should be in MASTER_TRANSMIT, so let's send out a message.
	//void Half_duplex::transmit()
	//{
	//	if (output_queue_.empty())
	//	{
	//		BOOST_LOG_TRIVIAL(debug) << "transmit() empty queue";
	//		Msg reply;
	//		reply.type = Msg_type::EOT;
	//		std::string reply_str = to_string(reply);
	//		asio::write(port_, asio::buffer(reply_str));
	//		change_state(State::NEUTRAL);
	//		maybe_select();
	//	}
	//	else
	//	{
	//		BOOST_LOG_TRIVIAL(debug) << "transmit(), " << output_queue_.size() << " messages in queue";
	//		last_message_ = output_queue_.front();
	//		output_queue_.pop_front();
	//		std::string reply_str = to_string(last_message_);
	//		asio::write(port_, asio::buffer(reply_str));
	//	}
	//}

	void Half_duplex::retransmit()
	{
		std::string reply_str = to_string(last_message_);
		asio::write(port_, asio::buffer(reply_str));
	}

	void Half_duplex::on_master_select_no_response_timeout(const system::error_code& ec)
	{
		if (ec != asio::error::operation_aborted)
		{
			BOOST_LOG_TRIVIAL(debug) << "on_master_select_no_response_timeout";
			write_simple_msg(Msg_type::EOT);
			change_state(State::NEUTRAL);
		}
	}
	
	void Half_duplex::change_state(State _new)
	{
		State old_state = state_;
		state_ = _new;
		BOOST_LOG_TRIVIAL(debug) << "state transition from " << to_string(old_state) << " to " << to_string(_new);
	}
}
