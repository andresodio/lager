INCLUDE_DIR := ../common
LDFLAGS=-L/usr/local/lib/
LIBS=-llager_connect -lboost_system -lboost_serialization -lrt -lpthread -lX11 -lXtst -lXext

all: lager_injector

lager_injector: lager_injector.cc
	g++ -I${INCLUDE_DIR} $(LDFLAGS) lager_injector.cc -o lager_injector $(LIBS)

clean:
	rm -f lager_injector
