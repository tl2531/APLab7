# Makefile for Lab 7, Part 2

CC  = gcc
CXX = g++

INCLUDES = 
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = 
LDLIBS =


.PHONY: default
default: http-server

.PHONY: clean
clean:
	rm -f *.o *~ a.out core http-server

.PHONY: all
all: clean default