bin_PROGRAMS = udptoserial

udptoserial_CXXFLAGS = -DBOOST_ALL_DYN_LINK -fdiagnostics-color=auto
udptoserial_SOURCES = main.cpp Server.cpp IPv4.cpp Tcp_server_handler.cpp Configuration.cpp ini.cpp \
    Ip_endpoint_join.cpp
udptoserial_LDFLAGS = -pthread
udptoserial_LDADD = -lboost_system -lboost_log ../libhorizr/libhorizr.a

