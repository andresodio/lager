INCLUDE_DIR := ../common
LDFLAGS=-L/usr/local/lib/
LIBS=-llager_convert -lboost_system -lboost_thread -lvrpn -lpthread
BUILD_DIR=./build

all: lager_gesture_manager

lager_gesture_manager: lager_gesture_manager.cc
	g++ -std=c++11 -I${INCLUDE_DIR} $(LDFLAGS) lager_gesture_manager.cc -o ${BUILD_DIR}/lager_gesture_manager $(LIBS)

clean:
	rm -f lager_gesture_manager
