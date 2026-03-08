CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -I../include
LDLIBS = -L../lib -ltzu -lcunit

SOURCES = $(wildcard *.c) $(wildcard */*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = test_runner

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDLIBS)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)
