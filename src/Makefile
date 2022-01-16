

CC = gcc
CFLAGS = -g -std=c99 -Wall -Werror -pedantic -pedantic-errors -pthread -O3 -L. -I.
AR = ar
ARFLAGS = rvs
INCLUDES = -I.
LDFLAGS = -L.



.PHONY: all clean test

all: server client 

# Pattern generico di compilazione di un file oggetto
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

# Eseguibile del server
server: server.c libsocket.a libhash_table.a libop_server.a 
	$(CC) $(CFLAGS) $< -o $@  -lop_server -lhash_table -lsocket

# Eseguibile del client
client: client.c libsocket.a libobjectstore.a 
	$(CC) $(CFLAGS) $< -o $@ -lobjectstore -lsocket


testhash: testhash.c libhash_table.a
	$(CC) $(CFLAGS) $< -o $@ -libhash_table

libhash_table.a: hash_table.o 
	$(AR) $(ARFLAGS) $@ $^

libsocket.a: util.o socket.o
	$(AR) $(ARFLAGS) $@ $^

libobjectstore.a: objectstore.o
	$(AR) $(ARFLAGS) $@ $^

libop_server.a: op_server.o
	$(AR) $(ARFLAGS) $@ $<



# Target che avvia il server e gli script di test
test: all
	./server &
	./test.sh
	./testsum.sh

clean:
	rm -rf ./data ./tmp.sock ./*.o ./*.a ./server ./client ./testout.log
