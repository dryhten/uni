CC = gcc
CFLAGS = -Wall -DLOG

all: build

build: server client

client: client.o
	$(CC) $^ -o $@

client.o: client.c general.h
	$(CC) $(CFLAGS) -g -c $< -o $@ -I.

server: server.o
	$(CC) $^ -o $@

server.o: server.c general.h
	$(CC) $(CFLAGS) -c $< -o $@ -I.

.PHONY: clean cleanserver cleanclient

clean: cleanserver cleanclient

cleanserver:
	rm server.o server

cleanclient:
	rm client.o client
