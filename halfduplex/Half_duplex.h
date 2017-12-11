#ifndef HALF_DUPLEX_H
#define HALF_DUPLEX_H

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <chrono>
#include <functional>
#include <mutex>
#include <deque>

using namespace boost;
using namespace std::placeholders;

namespace Serial {
  const static size_t RAW_READ_BUFFER_SIZE = 8*1024;

  enum class State {
    NEUTRAL,
    MASTER_SELECT,
    MASTER_RECEIVE,
    MASTER_TRANSMIT,
    SLAVE_RECEIVE,
    SLAVE_TRANSMIT,
  };

  std::string to_string(State s);
    enum class Msg_type {
    NONE,
    ACK,
    NAK,
    EOT,
    DLE_EOT,
    ENQ,
    INFO,
    ERROR
  };

  struct Msg {
    Msg_type type;
    std::string prefix;
    std::string header;
    std::string body;
  };

  
  class Half_duplex {

  public:
    Half_duplex(asio::io_service& ios, const std::string & device, unsigned int _baud_rate);

    void enqueue_message(std::string header, std::string body);
    bool get_next_message(std::string& header, std::string& body);

  private:
    
    void read_handler(const system::error_code& error,
		      size_t bytes_transferred);

    // Scan the unprocessed buffer for complete messages.
    Msg parse_input();

    uint8_t compute_cksum(size_t begin, size_t end);

    void handle_message_neutral_state(Msg& m);
    void handle_message_master_select_state(Msg& m);
    void handle_message_master_transmit_state(Msg& m);
    void handle_message_master_receive_state(Msg& m);
    void handle_message_slave_receive_state(Msg& m);
    void handle_clear_request();

    // We should be in NEUTRAL, so let's try to become the master station.
    void maybe_select();

    // We should be in MASTER_TRANSMIT, so let's send out a message.
    void transmit();
    void retransmit();
    void on_master_select_no_response_timeout(const system::error_code& ec);
    void change_state(State s);

    asio::serial_port port_;
    State state_;
    bool accepting_input_;
    size_t enq_nak_count_;
    size_t info_nak_count_;
    asio::deadline_timer no_response_timer_;
    asio::deadline_timer no_receive_timer_;
    asio::deadline_timer no_activity_timer_;
    unsigned char input_raw_[RAW_READ_BUFFER_SIZE];
    std::mutex io_mutex_;
    std::string input_unprocessed_;
    std::deque<Msg> input_queue_;
    Msg last_message_;
    std::deque<Msg> output_queue_;
  };
}

#endif
