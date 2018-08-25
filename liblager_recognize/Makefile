INCLUDE_DIRS=-I../common -I/usr/include/python3.6m
LDFLAGS=-L/usr/local/lib/ -L/usr/lib/python3.6/config-3.6m-x86_64-linux-gnu
ARCH_LIBS := -lpython3.6m -lpthread -lquat -lm -lrt
#BOOST_LIBS := -lboost_system -lboost_serialization
BOOST_LIBS :=
LAGER_LIBS := -llager_connect

all: liblager_recognize

liblager_recognize: liblager_recognize.cc
	g++ -fPIC -std=c++11 -c liblager_recognize.cc $(INCLUDE_DIRS) $(LDFLAGS) $(LAGER_LIBS) $(BOOST_LIBS) $(ARCH_LIBS)
	g++ -shared -o liblager_recognize.so liblager_recognize.o

clean:
	rm -f liblager_recognize.o liblager_recognize.so

install:
	cp liblager_recognize.so /usr/local/lib/
	cp liblager_recognize.h /usr/local/include/

remove:
	rm -f /usr/local/lib/liblager_recognize.so
	rm -f /usr/local/include/liblager_recognize.h
