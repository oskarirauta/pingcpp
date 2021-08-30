all: ping

IPV6?=1
CXX?=g++

SHARED_OBJS:= \
	objs/common.o

NETWORK_OBJS:= \
	objs/network.o \
	objs/ping.o objs/ping4.o

PING_OBJS:= \
	objs/pingmain.o

LIBS:=
INCLUDES:=-I./include -I.
CXXFLAGS:=--std=c++17

ifneq ($(IPV6),0)
	CXXFLAGS+= -D__IPV6__
	NETWORK_OBJS+= objs/ping6.o
endif

objs/common.o: shared/common.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/network.o: network/network.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/ping.o: network/ping.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/ping4.o: network/ping4.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/ping6.o: network/ping6.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/pingmain.o: example/ping.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

ping: $(SHARED_OBJS) $(NETWORK_OBJS) $(PING_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SHARED_OBJS) $(NETWORK_OBJS) $(PING_OBJS) $(LIBS) -o $@;

clean:
	rm -f objs/*.o ping
