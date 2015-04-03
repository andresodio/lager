INCLUDE_DIR=../common
LDFLAGS=-L/usr/local/lib/
ARCH_LIBS := -lpthread -lquat -lm -lrt
#BOOST_LIBS := -lboost_system -lboost_serialization
BOOST_LIBS :=
LAGER_LIBS := -llager_connect

all: liblager_recognize

liblager_recognize: liblager_recognize.cc
	g++ -fPIC -std=c++11 -I${INCLUDE_DIR} $(LDFLAGS) $(LAGER_LIBS) $(BOOST_LIBS) $(ARCH_LIBS) -c liblager_recognize.cc
	g++ -shared -o liblager_recognize.so liblager_recognize.o

clean:
	rm -f liblager_recognize.o liblager_recognize.so

install:
	cp liblager_recognize.so /usr/local/lib/
	cp liblager_recognize.h /usr/local/include/

remove:
	rm -f /usr/local/lib/liblager_recognize.so
	rm -f /usr/local/include/liblager_recognize.h
