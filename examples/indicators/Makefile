CFLAGS = -std=c11 -g -Wall -Wextra -Werror -pedantic -I../../include
LDLIBS = -L../../lib -lwu -Wl,-rpath,$(shell pwd)/../../lib

SOURCES = $(wildcard *.c)
TARGETS = $(SOURCES:.c=)

.PHONY: all clean

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(TARGETS)
