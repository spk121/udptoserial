#!/bin/sh
g++ -Wall -g -o logtest logtest.cpp -std=gnu++11 -lboost_log -lpthread -lboost_thread
