CC=g++ -g -Wall -std=c++20

UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
    CC+=-D_XOPEN_SOURCE
    LIBTHREAD=libthread_macos.o
else
    LIBTHREAD=libthread.o
endif 

# List of source files for your disk scheduler
SCHEDULER_SOURCES=main.cpp

# Generate the names of the disk scheduler's object files
SCHEDULER_OBJS=${SCHEDULER_SOURCES:.cpp=.o}

all: disk

# Compile the disk scheduler and tag this compilation
disk: ${SCHEDULER_OBJS} ${LIBTHREAD}
	${CC} -o $@ $^ -ldl -pthread

# Generic rules for compiling a source file to an object file
%.o: %.cpp
	${CC} -c $<
%.o: %.cc
	${CC} -c $<

clean:
	rm -f ${SCHEDULER_OBJS} disk
