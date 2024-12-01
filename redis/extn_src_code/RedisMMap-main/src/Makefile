
# find the OS
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

# Compile flags for linux / osx
ifeq ($(uname_S),Linux)
	SHOBJ_CFLAGS ?= -W -Wall -fno-common -g -ggdb -std=c99 -O2
	SHOBJ_LDFLAGS ?= -shared
else
	SHOBJ_CFLAGS ?= -W -Wall -dynamic -fno-common -g -ggdb -std=c99 -O2 -Wtypedef-redefinition
	SHOBJ_LDFLAGS ?= -bundle -undefined dynamic_lookup
endif

LIBS := -L /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib

.SUFFIXES: .c .so .xo .o

all: fmmap.so

.c.xo:
	$(CC) -I. $(CFLAGS) $(SHOBJ_CFLAGS) -fPIC -c $< -o $@

fmmap.xo: fmmap.c

fmmap.so: fmmap.xo
	$(LD) -o $@ $< $(SHOBJ_LDFLAGS) $(LIBS) -lc

clean:
	rm -rf *.xo *.so
