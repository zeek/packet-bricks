CC=cc
CFLAGS=-g -O0 -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
DEBUG_CFLAGS=-g -DDEBUG -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
INCLUDE=-I./include
NETMAP_INCLUDE=-I./include/netmap
#LDFLAGS=-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc \
	-fno-builtin-free -fno-builtin-posix_memalign -ljemalloc
LDFLAGS= -lpcap
BINDIR=bin
MKDIR=mkdir
RM=rm
BRO_SRC_PATH=/usr/home/ajamshed/johanna/bro
BROKER_INC_PATH=${BRO_SRC_PATH}/aux/broker
BROKER_LIB_PATH=/usr/home/ajamshed/johanna/bro-install/lib/
#---------------------------------------------------------------------#
SRCS = vale-test.c pkt-rx.c pcap-test.c nw-unit-test.c

all: default

default: tags
	$(CC) $(CFLAGS) $(INCLUDE) $(NETMAP_INCLUDE) -c $(SRCS)
	$(MKDIR) -p $(BINDIR)
	$(CC) vale-test.o $(LDFLAGS) -o $(BINDIR)/vale-test
	$(CC) pkt-rx.o $(LDFLAGS) -o $(BINDIR)/pkt-rx
	$(CC) pcap-test.o $(LDFLAGS) -o $(BINDIR)/pcap-test
	$(CC) nw-unit-test.o $(LDFLAGS) -o $(BINDIR)/nw-unit-test
	$(CC) -g -O3 -I $(BROKER_INC_PATH) event-send-broker.c -o $(BINDIR)/event-send-c -L $(BROKER_LIB_PATH) -lbroker
	$(CC) -g -O3 -I $(BROKER_INC_PATH) event-recv-broker.c -o $(BINDIR)/event-recv-c -L $(BROKER_LIB_PATH) -lbroker
	$(RM) -rf *.o
#	$(STRIP) $(BIN)

debug: tags
	$(CC) $(DEBUG_CFLAGS) $(INCLUDE) $(NETMAP_INCLUDE) -c $(SRCS)
	$(MKDIR) -p $(BINDIR)
	$(CC) vale-test.o $(LDFLAGS) -o $(BINDIR)/vale-test
	$(CC) pkt-rx.o $(LDFLAGS) -o $(BINDIR)/pkt-rx
	$(CC) pcap-test.o $(LDFLAGS) -o $(BINDIR)/pcap-test
	$(CC) nw-unit-test.o $(LDFLAGS) -o $(BINDIR)/nw-unit-test
	$(CC) -g -I $(BROKER_INC_PATH) event-send-broker.c -o $(BINDIR)/event-send-c -L $(BROKER_LIB_PATH) -lbroker
	$(CC) -g -I $(BROKER_INC_PATH) event-recv-broker.c -o $(BINDIR)/event-recv-c -L $(BROKER_LIB_PATH) -lbroker
	$(RM) -rf *.o 
#---------------------------------------------------------------------#
tags:
	find . -name '*.c' -or -name '*.h' | xargs ctags
#---------------------------------------------------------------------#
clean:
	rm -rf *~ bin/*  *.h~ *.c~ *.o tags
#---------------------------------------------------------------------#		
