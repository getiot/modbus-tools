CC=gcc
CFLAGS=-I/usr/local/include/modbus/ -lmodbus -g

all:
	$(CC) -o modpoll modpoll.c common.c $(CFLAGS)