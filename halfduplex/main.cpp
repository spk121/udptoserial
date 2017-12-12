#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "Half_duplex.h"
#include <iostream>
#include <stdexcept>
using namespace std;

std::shared_ptr<Serial::Half_duplex> com;

static void timer_handler(const boost::system::error_code ec)
{
  if (!ec)
    {
      std::string header, body;
      bool ret = com->get_next_message(header, body);
      com->enqueue_message("poop", "fart");
      com->enqueue_message("poop", "fart");
      com->enqueue_message("poop", "fart");
      com->enqueue_message("poop", "fart");
    }
}

int main()
{
  asio::io_service service;
  try
    {
      //com = std::make_shared<Serial::Half_duplex>(service, "/dev/ttyUSB0", 115200, true);
	  com = std::make_shared<Serial::Half_duplex>(service, "COM4", 115200, true);
	  asio::deadline_timer timer(service);
      timer.expires_from_now(boost::posix_time::seconds(1));
      timer.async_wait(timer_handler);
      service.run();
    }
  catch(std::exception const& e)
    {
      cout << "There was an error: " << e.what() << endl;
    }

  return 0;
}
