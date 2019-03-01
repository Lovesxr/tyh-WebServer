TARGET := libB.a

SOURCE := $(wildcard *.c) $(wildcard *.cc)
OBJS := $(patsubst %.c, %.o, $(patsubst %.cc, %.o, $(SOURCE)))

CC := gcc
CXX := g++

INCLUDE := -I .

CFLAGS := -g -o3 $(INCLUDE)
CXXFLAGS := $(CFLAGS) -std=c++11 -pthread -DNDEBUG

.PHONY : all clean rebuild

all: $(TARGET)
clean: 
	rm -f *.o
	rm $(TARGET)
rebuild: clean all

$(TARGET) : $(OBJS)
	ar rcs $@ $^
