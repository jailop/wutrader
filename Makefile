CFLAGS = -I./include
SOURCES = $(wildcard src/*/*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = lib/libwu.a

## Also build examples

all: $(TARGET) tests examples

tests: $(TARGET)
	$(MAKE) -C tests

run_tests: tests
	./tests/test_runner

examples: $(TARGET)
	$(MAKE) -C examples

$(TARGET): $(OBJECTS)
	@mkdir -p lib
	$(AR) rcs $@ $^

clean:
	rm -f $(OBJECTS) $(TARGET)
	$(MAKE) -C examples clean
