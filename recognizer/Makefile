INCLUDE_DIR=../common
LDFLAGS=-L/usr/local/lib/
ARCH_LIBS := -lpthread -lquat
BOOST_LIBS := -lboost_system -lboost_thread -lboost_serialization
LAGER_LIBS := -llager_connect -llager_convert -llager_recognize
VRPN_LIBS := -lvrpn
BUILD_DIR := ./build

all: lager_recognizer

lager_recognizer: lager_recognizer.cc
	g++ -std=c++11 -I$(INCLUDE_DIR) $(LDFLAGS) lager_recognizer.cc -o $(BUILD_DIR)/lager_recognizer $(LAGER_LIBS) $(BOOST_LIBS) $(VRPN_LIBS) $(ARCH_LIBS)

clean:
	rm -f $(BUILD_DIR)/*

install:
	cp $(BUILD_DIR)/lager_recognizer /usr/local/bin/

remove:
	rm -f /usr/local/bin/lager_recognizer
