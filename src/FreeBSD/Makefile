#---------------------------------------------------------------------#
SRCS := backend.c pkt_hash.c util.c

_OBJ := backend.o pkt_hash.o util.o
OBJ := $(patsubst %,$(OBJDIR)/%,$(_OBJ))

$(OBJDIR)/%.o: %.c
	$(MSG) "   CC $<"
	$(HIDE) $(CC) -c -o $@ $< $(CFLAGS) $(INCLUDE) $(NETMAP_INCLUDE)

all: $(OBJ)

.PHONY: clean

clean:
	$(RM) -rf *~
#---------------------------------------------------------------------#
