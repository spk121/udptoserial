#pragma once
#include <vector>
#include <deque>

/** \file iso1745.h
*
* This is an implementation of ISO 1745, basic mode control procedures for
* data communication systems, and ISO 2628 _Basic Mode Control Procedures
* -- Complements_  It is a manager for a half-duplex communication channel
* Ohh, half-duplex is ancient stuff where you can't send and receive
* at the same time, but, our radio modem is still half-duplex.
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
* so, it sends a (prefix)+ENQ message.
*
* Aafter transmitting the ENQ message, the master station starts Timer A,
* the no-response timer.
*
* If the other peer accepts this request, it responds with (prefix)+ACK.
* It is then the slave station and is ready to receive a message. We
* proceed to Phase 3.
* If the other peer rejects this request, it responds with (prefix)+EOT
* and the channel returns to neutral.
*
* If the Master Station fails to receive a valid ACK EOT
* within Timer A, or if the Slave station sends an incorrect reply
* we jump to Recovery Phase 1.
*
* Phase 3 Information Transfer
* 
* The master station sends a message to the slave station and starts Timer A.
* The slave station will either respond with ACK, NAK, or EOT.  ACK indicates
* that the message was received, and the slave is ready for another message.
* NAK indicates that the previous message was unacceptable, and a retransmission
* is requested.  An EOT indicates that the slave station is unable to receive
* further, aka an 'interruption'.  After EOT, the system returns to its
* neutral state at Phase 2B.
*
* The master station starts Timer A (No-response timer) after sending the
* terminating ETX or ETB.  It stops Timer A when it receives ACK, NAK, or
* EOT. 
*
* The slave station starts Timer B (receive timer) on the receipt of the initial SOH,
* or STX (if not preceded by SOH), and ends with it receives the terminating
* character, such as ETB, ETX, ENQ.  When Timer B times out, it discards the
* incomplete block, and carries on.
*
* When the master has no more messages to transmit, it sends EOT and the system
* returns to neutral, to Phase 2B.  When the master sends EOT at a designated time, we say
* the master has 'terminated.'  If it sends EOT at any other
* time, we say it has 'aborted'.  In either case, we 'return to neutral' and jump
* to Phase 2B.
*
* If the slave fails to respond within Timer A, we jump to Recovery Phase 3.
* If the slave NAK's too many times, we jump to Recover 4.
*
* Recovery Procedure 3
*
* If a peer requests to be the master station, but, receives an invalid reply,
* or no reply within Timer 1, the master station repeats its (prefix)+ENQ request
* to invite the peer to (re)transmit its previous response (ACK or NAK or EOT).
* It will repeat transmission up to MAX_REPEAT times.
* 
* Recover Procedure 4
*
* If the master station has repeated unsuccessful transmission of messages
* it should alarm and go to the termination phase.
*
* No-Activity Timer
*
* Both peers implement a Timer D (No-activity timer for non-switched lines)
* That timer starts or restarts on receipt or transmission of any
* character, and stops
* on receipt or transmission of EOT.  When a time-out occurs, the peer,
* if not in neutral,
* will return to neutral.
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
		Timer no_respose_timer_;
		Timer receive_timer_;
		Timer no_activity_timer;
		std::deque<std::string> outbox_;
		std::vector<uint8_t> inbox_raw_;
		std::deque<Msg> inbox_;
	};
}