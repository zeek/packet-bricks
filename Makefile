## XXX short-term Makefile. This will be improved substantially..... ##
#---------------------------------------------------------------------#
CC=gcc
CFLAGS=-O3 -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
DEBUG_CFLAGS=-g -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
INCLUDE=-I./include -I/usr/include/lua5.1/
NETMAP_INCLUDE=-I./include/netmap
LIBS=-llua5.1 -lpthread
LDFLAGS=$(LIBS)
SRC=./src
BINDIR=bin
BIN=$(BINDIR)/pacf
MKDIR=mkdir
RM=rm
STRIP=strip
#---------------------------------------------------------------------#
SRCS = src/lua_interpreter.c src/main.c src/lua_interface.c \
	src/pkt_engine.c src/netmap_module.c src/thread.c

all: default

default: tags
	$(CC) $(CFLAGS) $(INCLUDE) $(NETMAP_INCLUDE) -c $(SRCS)
	$(MKDIR) -p $(BINDIR)
	$(CC) *.o $(LDFLAGS) -o $(BIN)
	$(RM) -rf *.o
	$(STRIP) $(BIN)

debug: tags
	$(CC) $(DEBUG_CFLAGS) -DDEBUG $(INCLUDE) $(NETMAP_INCLUDE) -c $(SRCS)
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
