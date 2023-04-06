IPV6?=1
PINGCPP_DIR?=pingcpp
INCLUDES += -I./$(PINGCPP_DIR)/include
PINGCPP_OBJS:= \
	objs/network.o \
	objs/ping.o objs/ping4.o

ifneq ($(IPV6),0)
	CXXFLAGS+= -D__IPV6__
	PINGCPP_OBJS+= objs/ping6.o
endif

objs/network.o: network/network.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/ping.o: network/ping.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/ping4.o: network/ping4.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/ping6.o: network/ping6.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;
