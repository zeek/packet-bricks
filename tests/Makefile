CC=gcc
CFLAGS=-O3 -pipe -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
DEBUG_CFLAGS=-g -DDEBUG -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
INCLUDE=-I./include
NETMAP_INCLUDE=-I./include/netmap
LDFLAGS=-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc \
	-fno-builtin-free -fno-builtin-posix_memalign -ljemalloc
BINDIR=bin
BIN=$(BINDIR)/vale-test
MKDIR=mkdir
#---------------------------------------------------------------------#
SRCS = vale-test.c

all: default

default: tags
	$(CC) $(CFLAGS) $(INCLUDE) $(NETMAP_INCLUDE) -c $(SRCS)
	$(MKDIR) -p $(BINDIR)
	$(CC) *.o $(LDFLAGS) -o $(BIN)
	$(RM) -rf *.o
#	$(STRIP) $(BIN)

debug: tags
	$(CC) $(DEBUG_CFLAGS) $(INCLUDE) $(NETMAP_INCLUDE) -c $(SRCS)
	$(MKDIR) -p $(BINDIR)
	$(CC) *.o $(LDFLAGS) -o $(BIN)
	$(RM) -rf *.o 
#---------------------------------------------------------------------#
tags:
	find -name '*.c' -or -name '*.h' | xargs ctags
#---------------------------------------------------------------------#
clean:
	rm -rf *~ bin/*  *.h~ *.c~ *.o tags
#---------------------------------------------------------------------#		
