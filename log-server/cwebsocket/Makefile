COMPILER=gcc
OBJS=sha1.o base64_enc.o websocket.o

libcwebsocket.a: $(OBJS)
	ar rcs $@ $(OBJS)
sha1.o: sha1.h sha1.c
	$(COMPILER) -c -o $@ sha1.c
base64_enc.o: base64_enc.c base64_enc.h
	$(COMPILER) -c -o $@ base64_enc.c
websocket.o: websocket.h websocket.c
	$(COMPILER) -c -o $@ websocket.c

clean:
	rm $(OBJS) libcwebsocket.a
