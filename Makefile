CC = cc
CPP = g++
TARGET = lisp
OBJS = src/lisp.o
CFLAGS ?= -O2
#CFLAGS ?= -g3 -O0 -pg

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) 

.SUFFIXES: .c.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $@

.cpp.o:
	$(CPP) $(CFLAGS) -c $*.cpp -o $@

clean:
	rm -f $(TARGET) $(OBJS)

