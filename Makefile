VERSION = 0.1.0
VERSION_MAJOR = 0
VERSION_MINOR = 1

CFLAGS = -I./include -Wall -Wextra -std=c11 -g -fPIC
SOURCES = $(wildcard src/*/*.c)
OBJECTS = $(SOURCES:.c=.o)
SONAME = libwu.so.$(VERSION_MAJOR)
TARGET = lib/libwu.so.$(VERSION)
LINK = lib/libwu.so
PREFIX ?= /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

all: $(TARGET) tests examples

tests: $(TARGET)
	$(MAKE) -C tests

run_tests: tests
	LD_LIBRARY_PATH=./lib ./tests/test_runner

examples: $(TARGET)
	$(MAKE) -C examples

run_examples: examples
	LD_LIBRARY_PATH=./lib ./examples/backtest/example01 ./tests/data/btcusd.csv -v
	LD_LIBRARY_PATH=./lib ./examples/backtest/pairs_trading ./tests/data/spy.csv ./tests/data/qqq.csv  -v

$(TARGET): $(OBJECTS)
	@mkdir -p lib
	$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^ -lm
	@ln -sf libwu.so.$(VERSION) $(LINK)
	@ln -sf libwu.so.$(VERSION) lib/$(SONAME)

docs:
	@echo "Generating Doxygen documentation..."
	@doxygen Doxyfile
	@echo "Documentation generated in docs/html/index.html"

version:
	@echo "WU Trading Library version $(VERSION)"

install: $(TARGET)
	@echo "Installing WU library $(VERSION)..."
	install -d $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(INCLUDEDIR)/wu
	install -m 755 $(TARGET) $(DESTDIR)$(LIBDIR)
	ln -sf libwu.so.$(VERSION) $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sf libwu.so.$(VERSION) $(DESTDIR)$(LIBDIR)/libwu.so
	install -m 644 include/wu.h $(DESTDIR)$(INCLUDEDIR)
	install -m 644 include/wu/*.h $(DESTDIR)$(INCLUDEDIR)/wu/
	@echo "Running ldconfig..."
	@ldconfig || echo "Note: Run 'sudo ldconfig' to update library cache"
	@echo "Installation complete!"

uninstall:
	@echo "Uninstalling WU library..."
	rm -f $(DESTDIR)$(LIBDIR)/libwu.so*
	rm -f $(DESTDIR)$(INCLUDEDIR)/wu.h
	rm -rf $(DESTDIR)$(INCLUDEDIR)/wu
	@ldconfig || echo "Note: Run 'sudo ldconfig' to update library cache"
	@echo "Uninstall complete!"

clean:
	rm -f $(OBJECTS) lib/libwu.so* lib/libwu.a
	$(MAKE) -C examples clean
	rm -rf docs/

.PHONY: all tests run_tests examples docs version install uninstall clean
