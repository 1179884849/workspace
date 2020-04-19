TARGET = libserial
LIBPATH = ../lib
PATH_SHARE_TARGET = $(LIBPATH)/$(TARGET).so
PATH_STATIC_TARGET = $(LIBPATH)/$(TARGET).a
BUILD_PATH = ../build

CC = gcc
CFLAGS = -Wall -O -g -fPIC
INCLUDE = -I ../include

SOURCE = $(wildcard *.c)
OBJS = $(patsubst %.c, $(BUILD_PATH)/%.o, $(wildcard *.c))

all:$(PATH_SHARE_TARGET) $(PATH_STATIC_TARGET)

$(PATH_SHARE_TARGET):$(OBJS)
	$(CC) -shared -fPIC -o $@ $^

$(PATH_STATIC_TARGET):$(OBJS)
	ar rc $@ $^
	ranlib $@

$(OBJS):$(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDE) -c $^ -o $@

clean:
	rm -f $(BUILD_PATH)/*.o
	rm -f $(LIBPATH)/*

