INCLUDE_DIR := ../common
LDFLAGS=-L/usr/local/lib/
LIBS=-llager_connect -lboost_system -lboost_serialization -lrt -lpthread -lX11 -lXtst -lXext
BUILD_DIR=./build

all: lager_injector

lager_injector: lager_injector.cc
	g++ -I${INCLUDE_DIR} $(LDFLAGS) lager_injector.cc -o ${BUILD_DIR}/lager_injector $(LIBS)

clean:
	rm -f build/*

install:
	cp ${BUILD_DIR}/lager_injector /usr/local/bin/
	mkdir -p ~/.lager/injector/
	cp gestures.dat ~/.lager/injector
	chown -R "${SUDO_USER}" ~/.lager/injector

remove:
	rm /usr/local/bin/lager_injector
	rm -rf ~/.lager/injector/
