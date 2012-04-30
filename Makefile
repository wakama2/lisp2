CC = g++
CPP = $(CC)
TARGET=lisp
CFLAGS = -O0 -g3 -Wall
INCDIR = -Iinc
LIB = -lreadline -lpthread
SRCS = \
	src/vm.cpp \
	src/scheduler.cpp \
	src/codegen.cpp \
	src/builder.cpp \
	src/context.cpp \
	src/lisp.cpp \
	src/parse.cpp \
	src/llvm.cpp
HEADERS = \
	inc/lisp.h
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(HEADERS) $(OBJS)
	$(CC) $(OBJS) -o $@ $(LIB) $(CFLAGS) $(INCDIR) `llvm-config --libs core jit native --ldflags`

src/llvm.o: $(HEADERS)
	$(CPP) src/llvm.cpp -c -o $@ $(CFLAGS) $(INCDIR) `llvm-config --cxxflags`

.SUFFIXES: .cpp.o
.cpp.o: $(HEADERS)
	$(CPP) $< -c -o $@ $(CFLAGS) $(INCDIR) 

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)

