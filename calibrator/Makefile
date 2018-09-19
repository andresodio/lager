INCLUDE_DIR := ../common
LDFLAGS=-L/usr/local/lib/
LIBS=-losvrClientKit
BUILD_DIR=./build

all: lager_calibrator

lager_calibrator: lager_calibrator.cc
	g++ -std=c++11 -I${INCLUDE_DIR} $(LDFLAGS) lager_calibrator.cc -o ${BUILD_DIR}/lager_calibrator $(LIBS)

clean:
	rm -f build/*

install:
	cp ${BUILD_DIR}/lager_calibrator /usr/local/bin/

remove:
	rm /usr/local/bin/lager_calibrator
