
COMPILER=g++
CFLAGS=-Lcwebsocket -Iinclude -std=c++0x
STATICLIBS=-pthread -lcwebsocket -lssl
OBJS=web_socket_server.o client.o log-server.o

all: lib log-server

log-server: $(OBJS)
	$(COMPILER) $(CFLAGS) -o $@ $(OBJS) $(STATICLIBS)
lib:
	cd cwebsocket && make

web_socket_server.o: web_socket_server.h web_socket_server.cpp
	$(COMPILER) -c $(CFLAGS) -o $@ web_socket_server.cpp
client.o: client.h client.cpp
	$(COMPILER) -c $(CFLAGS) -o $@ client.cpp
log-server.o: log-server.cpp
	$(COMPILER) -c $(CFLAGS) -o $@ log-server.cpp

debug: CFLAGS+=-DDEBUGLOGGING -g
debug: all

clean: cleanlib
	rm log-server web_socket_server.o client.o log-server.o
cleanlib:
	cd cwebsocket && make clean
	
