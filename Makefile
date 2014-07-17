## XXX short-term Makefile. This will be improved substantially..... ##
#---------------------------------------------------------------------#
# Borrowed from Luigi's netmap-ipfw package
OSARCH := $(shell uname)
OSARCH := $(findstring $(OSARCH),FreeBSD Linux Darwin)

CC=cc
CFLAGS=-O3 -pipe -Wall -Wunused-function -Wextra -Werror -D_GNU_SOURCE -D__USE_GNU 
DEBUG_CFLAGS=-g -DDEBUG -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
DEBUG_CFLAGS+=-DDLUA -DDPKTENG -DDNMP -DDUTIL -DDIFACE -DDBKEND -DDPKTHASH -DDRULE
ifeq ($(OSARCH),FreeBSD)
	INCLUDE=-I./include -I/usr/local/include/lua51/ -Isys/sys/
	LIBS=-L/usr/local/lib/ -llua-5.1 -lpthread
	LDFLAGS=$(LIBS)
else
	INCLUDE=-I./include -I/usr/include/lua5.1/
	LIBS=-llua5.1 -lpthread
	LDFLAGS=$(LIBS) -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc \
	    -fno-builtin-free -fno-builtin-posix_memalign -ljemalloc
endif
NETMAP_INCLUDE=-I./include/netmap
BINDIR=bin
BIN=$(BINDIR)/pacf
MKDIR=mkdir
RM=rm
STRIP=strip
SRCS = src/lua_interpreter.c src/main.c src/lua_interface.c \
	src/pkt_engine.c src/netmap_module.c \
	src/network_interface.c src/rule.c
ifeq ($(OSARCH),FreeBSD)
	SRCS += src/FreeBSD/pkt_hash.c src/FreeBSD/util.c src/FreeBSD/backend.c
else
	SRCS += src/Linux/pkt_hash.c src/Linux/util.c src/Linux/backend.c
endif
#---------------------------------------------------------------------#
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
	scripts/*.lua~ src/Linux/*~ src/FreeBSD/*~
#---------------------------------------------------------------------#		
