CC=gcc
CFLAGS = -Wall -O -g
PWD = $(shell pwd)

INCLUDE = -I ./include
LIBPATH = -L ./lib
LIBVAR += -static
LIBVAR += -lserial

TARGET = ./bin/serial_test
SOURCE = $(wildcard *.c)


all:$(TARGET)
$(TARGET):$(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBVAR) $(LIBPATH)

.PHONY:clean
clean:
	rm -f $(PWD)/bin/*
