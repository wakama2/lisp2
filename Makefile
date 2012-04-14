CC = g++
CPP = g++
TARGET=lisp
CFLAGS = -O2 -g3 -Wall
INCDIR = -Iinc
LIB = -lreadline -lpthread
SRCS = \
	src/vm.cpp \
	src/scheduler.cpp \
	src/codegen.cpp \
	src/builder.cpp \
	src/context.cpp \
	src/lisp.cpp \
	src/parse.cpp
HEADERS = \
	inc/lisp.h
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(HEADERS) $(OBJS)
	$(CC) $(CFLAGS) $(INCDIR) -o $@ $(OBJS) $(LIB)

.SUFFIXES: .cpp.o
.cpp.o: $(HEADERS)
	$(CPP) $(CFLAGS) $(INCDIR) -o $@ -c $< $(LIB)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)

