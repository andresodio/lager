LDFLAGS=-L/usr/local/lib/
LIBS=-lboost_serialization -lrt -lpthread -lX11 -lXtst -lXext

all: liblager_connect

liblager_connect: liblager_connect.cc
	g++ -fPIC -I${INCLUDE_DIR} $(LDFLAGS) $(LIBS) -c liblager_connect.cc
	g++ -shared -o liblager_connect.so liblager_connect.o

clean:
	rm -f liblager_connect.o liblager_connect.so

install:
	cp liblager_connect.so /usr/local/lib/
	cp liblager_connect.h /usr/local/include/

remove:
	rm -f /usr/local/lib/liblager_connect.so
	rm -f /usr/local/include/liblager_connect.h
