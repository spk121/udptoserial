#ifdef WIN32
// When I build Boost in the standard way on Win32, I get DLLs.
#else
// #define BOOST_LOG_DYN_LINK
#endif
// #define BOOST_ALL_DYN_LINK


#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;



void init()
{
  logging::add_file_log
    (
     keywords::file_name = "sample.log"
     );

  logging::core::get()->set_filter
    (
     logging::trivial::severity >= logging::trivial::info
     );
}

int main (int, char*[])
{
  init();
  logging::add_common_attributes();

  using namespace logging::trivial;
  src::severity_logger<severity_level> lg;

  BOOST_LOG_SEV(lg, warning) << "A WARNING!!!!";
}

