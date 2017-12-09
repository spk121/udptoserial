#pragma once
#include <vector>
#include <deque>

/** \file iso1745.h
*
* This is an implementation of ISO 1745, basic mode control procedures for
* data communication systems.  It forms part of the udptoserial program, and
* it is responsible for managing the half-duplex communication channel.
*
* It specifically implements "alternate two-way transfer of information with
* alternate supervision on the same channel."  Since this is a point-to-point
* serial link, there is no 'switching' or 'identification' phase, so we jump
* straight to 'polling'.
*
* Phase 2b Polling.
*
* The channel is initially in 'neutral', in which neither station is the 
* master or the slave.  If a peer has information to send, it needs to 
* inform the other peer that it wishes to be the master station.  To do
* so, it sends a (prefix)+ENQ message, and starts Timer 1.
*
* If the other peer accepts this request, it responds with (prefix)+ACK.
* It is then the slave station and is ready to receive a message, and we
* proceed to Phase 3.
* If the other peer rejects this request, it responds with (prefix)+EOT
* and the channel returns to neutral.
* If the other peer fails to respond within Timer 1, or sends an invalid
* reply, we jump to Recovery Phase 1.
*
* Phase 3 Information Transfer
* 
* The master station sends a message to the slave station and starts Timer 2.
* The slave station will either respond with ACK, NAK, or EOT.  ACK indicates
* that the message was received, and the slave is ready for another message.
* NAK indicates that the previous message was unacceptable, and a retransmission
* is requested.  An EOT indicates that the slave station is unable to receive
* further, aka an 'interruption'.  After EOT, the system returns to Phase 2B.
*
* If the master sends (prefix)+ENQ, the master is likely in Recover Procedure 1
* because the reply from the slave station was garbled or lost.  The slave station
* should resend its previous ACK or NAK.
*
* When the master has no more messages to transmit, it sends EOT and the system
* returns to neutral, to Phase 2B.  When the master sends EOT at a designated time, we say
* the master has 'terminated.'  If it sends EOT at any other
* time, we say it has 'aborted'.  In either case, we 'return to neutral' and jump
* to Phase 2B.
*
* If the slave fails to respond within Timer 2, we jump to Recovery Phase 3.
*
* Recovery Procedure 3
*
* If a peer requests to be the master station, but, receives an invalid reply,
* or no reply within Timer 1, the master station repeats its (prefix)+ENQ request
* to invite the peer to (re)transmit its previous response (ACK or NAK or EOT).
* 
* Recover Procedure 4
*
* If the master station has repeated unsuccessful transmission of messages
* it should alarm and go to the termination phase.
*/

namespace Iso1745
{
	enum class State
	{
		Neutral,
		MASTER_SELECTING,  ///< sending a request to become the master station
		MASTER_SELECTING_AWAITING_RESPONSE,  ///< waiting for a response to my request to be a master station
		SLAVE_SELECT_RESPONDING, ///< sending a response to the peer's request to be a master station
		MASTER_TRANSFER,   ///< sending a message
		MASTER_TRANSFER_AWAITING_RESPONSE,   ///< awaiting a respone to my message transfer
		SLAVE_AWAITING_TRANSFER, ///< waiting for a message from the master station
		SLAVE_RESPONDING, ///< sending a response in receipt of a message from the master station
		TERMINATING,
		DISCONNECTING,
		CLEARING
	};

	class Peer
	{
	public:
		Peer();
		~Peer();

		void async_read();
		void async_read_done();
		void get_next_message();
		void tick();

		/**
		* When true, this endpoint is the control station: it has the
		* responsibility of designating the master station.  Only one endpoint can
		* be a control station.
		*/
		State state_;
		std::deque<std::string> outbox_;
		std::vector<uint8_t> inbox_raw_;
		std::deque<Msg> inbox_;
	};
}