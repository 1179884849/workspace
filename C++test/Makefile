CC=g++ -std=c++0x
CFLAGS = -Wall -O -g

TARGET = ./a.out
SOURCE = template_test.cpp

LIB_PATH = -L ./lib

LIBVAR =  

all:$(TARGET)
$(TARGET):$(SOURCE)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBVAR) $(LIB_PATH)

.PHONY:clean
clean:
	rm -f $(TARGET)
