CC ?= gcc
PKG_CONFIG ?= pkg-config
CFLAGS += $(shell $(PKG_CONFIG) --cflags libsexy3) $(shell $(PKG_CONFIG) --cflags gtk+-3.0)
LIBS += $(shell $(PKG_CONFIG) --libs libsexy3) $(shell $(PKG_CONFIG) --libs gtk+-3.0)

all:
	${CC} ${CFLAGS} main.c -o example ${LIBS}

clean:
	rm -f ./example
