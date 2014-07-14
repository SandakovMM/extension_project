COMPILER=g++
CFLAGS=-Lcwebsocket -Iinclude
STATICLIBS=-pthread -lcwebsocket

log-server: lib log-server.cpp
	$(COMPILER) $(CFLAGS) -o $@ log-server.cpp $(STATICLIBS)
lib:
	cd cwebsocket && make
#	cd ../

clean: cleanlib
	rm log-server
cleanlib:
	cd cwebsocket && make clean
#	cd ../
	
