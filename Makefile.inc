IPV6?=1
PINGCPP_DIR?=pingcpp
INCLUDES += -I./$(PINGCPP_DIR)/include

PINGCPP_OBJS:= \
	objs/ping_utils.o \
	objs/ping.o objs/ping4.o

ifneq ($(IPV6),0)
	CXXFLAGS+= -D__PINGCPP_IPV6__
	PINGCPP_OBJS+= objs/ping6.o
endif

objs/ping_utils.o: $(PINGCPP_DIR)/src/ping_utils.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/ping.o: $(PINGCPP_DIR)/src/ping.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/ping4.o: $(PINGCPP_DIR)/src/ping4.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/ping6.o: $(PINGCPP_DIR)/src/ping6.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;
