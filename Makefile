#---------------------------------------------------------------------#
BINDIR := $(shell pwd)/bin
BIN := $(BINDIR)/pacf
OSARCH := $(shell uname)
OSARCH := $(findstring $(OSARCH),FreeBSD Linux Darwin)
DEBUG_CFLAGS := -g -DDEBUG -Wall -Werror -Wunused-function -Wextra -D_GNU_SOURCE -D__USE_GNU
DEBUG_CFLAGS += -DDLUA -DDPKTENG -DDNMP -DDUTIL -DDIFACE -DDBKEND -DDPKTHASH -DDRULE
#---------------------------------------------------------------------#
ifeq ($V,) # no echo
    export MSG=@echo
    export HIDE=@
else
    export MSG=@\#
    export HIDE=
endif

export OSARCH
export OBJDIR := $(shell pwd)/.objs
export CFLAGS := -O3 -pipe -Wall -Wunused-function -Wextra -Werror -D_GNU_SOURCE -D__USE_GNU 
export NETMAP_INCLUDE := -I$(shell pwd)/include/netmap

ifeq ($(OSARCH),FreeBSD)
	export INCLUDE := -I$(shell pwd)/include -I/usr/local/include/lua51/ -Isys/sys/
	export LIBS := -L/usr/local/lib/ -llua-5.1 -lpthread
	export LDFLAGS := $(LIBS)
else
	export INCLUDE := -I$(shell pwd)/include -I/usr/include/lua5.1/
	export LIBS := -llua5.1 -lpthread
	export LDFLAGS := $(LIBS) -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc \
	-fno-builtin-free -fno-builtin-posix_memalign -ljemalloc
endif
#---------------------------------------------------------------------#
all: pacf

objs:
	mkdir -p $(OBJDIR)
	cd src && $(MAKE)

objs-dbg:
	$(eval export CFLAGS := $(DEBUG_CFLAGS))
	mkdir -p $(OBJDIR)
	cd src && $(MAKE)

pacf: objs
	mkdir -p $(BINDIR)
	$(MSG) "   LD $@"
	$(HIDE) $(CC) $(OBJDIR)/*.o $(LDFLAGS) -o $(BIN)
	strip $(BIN)

run: pacf
	$(BIN) -f scripts/startup.lua
#---------------------------------------------------------------------#
debug: pacf-debug

pacf-debug: objs-dbg
	mkdir -p $(BINDIR)
	$(MSG) "   LD $@-debug"
	$(HIDE) $(CC) $(OBJDIR)/*.o $(LDFLAGS) -o $(BIN)
#---------------------------------------------------------------------#
clean:
	cd src && $(MAKE) clean
	$(RM) -rf $(BINDIR) include/*~ tags scripts/*~

.PHONY: clean

tags:
	find -name '*.c' -or -name '*.h' | xargs ctags
#---------------------------------------------------------------------#
