CC := gcc
CFLAGS := -Wall -o -g -fPIC


INCLUDE_PATH := -I ../include .
LIB_PATH := ../lib

TARGET := libThreadPool
SHARE_TARGET = $(LIB_PATH)/$(TARGET).so
STATIC_TARGET = $(LIB_PATH)/$(TARGET).a


#LIBVAR += -static
#LIBVAR += -lpthread

SOURCE = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(wildcard *.c))



all:$(SHARE_TARGET) $(STATIC_TARGET)
$(SHARE_TARGET):$(OBJ)
	$(CC) -shared -fPIC -o $@ $^

$(STATIC_TARGET):$(OBJ)
	ar rc $@ $^
	ranlib $@

$(OBJ):$(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDE) -c $^ -o $@ 

.PTHONY:clean
clean:
	rm ./*.o ../lib/*



