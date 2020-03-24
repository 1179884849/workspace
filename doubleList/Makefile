CC = gcc
CFLAGS = -Wall -o -g -fPIC

INCLUDE_PATH = -I ./include

LIB_PATH = -L ./lib

LIBVAR = -static -lDoubleList

TARGET = ./double_list_test
SOURCE = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(wildcard *.c))

all:$(TARGET)
$(TARGET):$(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDE_PATH) -o $@ $^ $(LIBVAR) $(LIB_PATH)

.PTONY:clean
clean:
	rm $(TARGET) 
