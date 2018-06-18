INCLUDE_DIR=../common
LIBDIR=/usr/local/lib/
LDFLAGS=-L$(LIBDIR)
ARCH_LIBS := -lpthread -lquat -lm -lrt
LIBS=-lboost_thread -losvrClientKit

all: liblager_convert

liblager_convert: liblager_convert.cc
	g++ -fPIC -std=c++11 -I${INCLUDE_DIR} $(LDFLAGS) $(LIBS) $(ARCH_LIBS) -c liblager_convert.cc
	g++ -shared -o liblager_convert.so liblager_convert.o

clean:
	rm -f liblager_convert.o liblager_convert.so

install:
	cp liblager_convert.so /usr/local/lib/
	cp liblager_convert.h /usr/local/include/

remove:
	rm -f /usr/local/lib/liblager_convert.so
	rm -f /usr/local/include/liblager_convert.h
