CC=g++ -std=c++0x
CFLAGS = -Wall -O -g

TARGET = ./test
SOURCE = $(wildcard *.cpp)


all:$(TARGET)
$(TARGET):$(SOURCE)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY:clean
clean:
	rm -f $(TARGET)
