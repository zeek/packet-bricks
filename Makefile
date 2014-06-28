## XXX short-term Makefile. This will be improved substantially..... ##
#---------------------------------------------------------------------#
CC=gcc
CFLAGS=-O3 -pipe -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
DEBUG_CFLAGS=-g -DDEBUG -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
INCLUDE=-I./include -I/usr/include/lua5.1/
NETMAP_INCLUDE=-I./include/netmap
LIBS=-llua5.1 -lpthread
LDFLAGS=$(LIBS) -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -fno-builtin-posix_memalign -ljemalloc
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
	$(CC) $(DEBUG_CFLAGS) $(INCLUDE) $(NETMAP_INCLUDE) -c $(SRCS)
	$(MKDIR) -p $(BINDIR)
	$(CC) *.o $(LDFLAGS) -o $(BIN)
	$(RM) -rf *.o 
#---------------------------------------------------------------------#
run: all
	$(BIN) -f scripts/startup.lua
#---------------------------------------------------------------------#
tags:
	find -name '*.c' -or -name '*.h' | xargs ctags
#---------------------------------------------------------------------#
clean:
	rm -rf *~ bin/* lib/*.o include/*.h~ src/*.c~ *.o tags \
	scripts/*.lua~
#---------------------------------------------------------------------#		
