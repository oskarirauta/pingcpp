all: world

IPV6?=1
PINGCPP_DIR=.

CXX?=g++
CXXFLAGS:=--std=c++17
INCLUDES:=-I./include -I.
LIBS:=

PING_OBJS:= \
	objs/pingmain.o

MINIPING_OBJS:= \
	objs/miniping.o

include common/Makefile.inc
include Makefile.inc

world: ping miniping

objs/pingmain.o: example/ping.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/miniping.o: example/miniping.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

ping: $(COMMON_OBJS) $(PINGCPP_OBJS) $(PING_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@;

miniping: $(COMMON_OBJS) $(PINGCPP_OBJS) $(MINIPING_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@;

clean:
	rm -f objs/*.o ping miniping
