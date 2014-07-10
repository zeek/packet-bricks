CC=gcc
CFLAGS=-O0 -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
DEBUG_CFLAGS=-g -DDEBUG -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
INCLUDE=-I./include
NETMAP_INCLUDE=-I./include/netmap
#LDFLAGS=-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc \
	-fno-builtin-free -fno-builtin-posix_memalign -ljemalloc
LDFLAGS=
BINDIR=bin
MKDIR=mkdir
#---------------------------------------------------------------------#
SRCS = vale-test.c pkt-rx.c

all: default

default: tags
	$(CC) $(CFLAGS) $(INCLUDE) $(NETMAP_INCLUDE) -c $(SRCS)
	$(MKDIR) -p $(BINDIR)
	$(CC) vale-test.o $(LDFLAGS) -o $(BINDIR)/vale-test
	$(CC) pkt-rx.o $(LDFLAGS) -o $(BINDIR)/pkt-rx
	$(RM) -rf *.o
#	$(STRIP) $(BIN)

debug: tags
	$(CC) $(DEBUG_CFLAGS) $(INCLUDE) $(NETMAP_INCLUDE) -c $(SRCS)
	$(MKDIR) -p $(BINDIR)
	$(CC) vale-test.o $(LDFLAGS) -o $(BINDIR)/vale-test
	$(CC) pkt-rx.o $(LDFLAGS) -o $(BINDIR)/pkt-rx	
	$(RM) -rf *.o 
#---------------------------------------------------------------------#
tags:
	find -name '*.c' -or -name '*.h' | xargs ctags
#---------------------------------------------------------------------#
clean:
	rm -rf *~ bin/*  *.h~ *.c~ *.o tags
#---------------------------------------------------------------------#		
