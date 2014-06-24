## XXX short-term Makefile. This will be improved substantially..... ##
#---------------------------------------------------------------------#
CC=gcc
CFLAGS=-O3 -Wall -Werror -pipe -Wunused-function -Wextra
DEBUG_CFLAGS=-g -Wall -Werror -Wunused-function -Wextra
INCLUDE=-I./include -I/usr/include/lua5.1/
LIBS=-llua5.1
LDFLAGS=$(LIBS)
SRC=./src
BINDIR=bin
BIN=$(BINDIR)/pacf
MKDIR=mkdir
RM=rm
#---------------------------------------------------------------------#
SRCS = src/lua_interpreter.c src/main.c src/lua_interface.c \
	src/pkt_engine.c

all: default

default: tags
	$(CC) $(CFLAGS) $(INCLUDE) -c $(SRCS)
	$(MKDIR) -p $(BINDIR)
	$(CC) *.o $(LDFLAGS) -o $(BIN)
	$(RM) -rf *.o

debug: tags
	$(CC) $(DEBUG_CFLAGS) -DDEBUG $(INCLUDE) -c $(SRCS)
	$(MKDIR) -p $(BINDIR)
	$(CC) *.o $(LDFLAGS) -o $(BIN)
	$(RM) -rf *.o 
#---------------------------------------------------------------------#
run: all
	$(BIN) < scripts/startup.lua
#---------------------------------------------------------------------#
tags:
	find -name '*.c' -or -name '*.h' | xargs ctags
#---------------------------------------------------------------------#
clean:
	rm -rf *~ bin/* lib/*.o include/*.h~ src/*.c~ *.o tags \
	scripts/.lua~
#---------------------------------------------------------------------#		
