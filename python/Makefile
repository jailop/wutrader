# Makefile for building Python bindings with SWIG

SWIG = swig
PYTHON = python3
PYTHON_CONFIG = python3-config

# SWIG options
SWIG_OPTS = -python -Wall

# Python include flags
PYTHON_INCLUDE = $(shell $(PYTHON_CONFIG) --includes)

# Get Python extension suffix
PYTHON_EXT_SUFFIX = $(shell $(PYTHON_CONFIG) --extension-suffix)

# Source files
SRC_DIR = ../src
INCLUDE_DIR = ../include
LIB_DIR = ../lib
# Get version from parent Makefile
WU_VERSION = $(shell grep "^VERSION = " ../Makefile | cut -d' ' -f3)
WU_LIB = $(LIB_DIR)/libwu.so.$(WU_VERSION)
SWIG_INTERFACE = wu.i
SWIG_WRAPPER = wu_wrap.c
MODULE_NAME = _wu

# All C source files
WU_SOURCES = $(shell find $(SRC_DIR) -name "*.c")
WU_OBJS = $(WU_SOURCES:.c=.o)

# Output files
WRAPPER_OBJ = wu_wrap.o
SHARED_LIB = $(MODULE_NAME)$(PYTHON_EXT_SUFFIX)
PYTHON_MODULE = wu.py

# Compiler flags
CFLAGS = -fPIC -I$(INCLUDE_DIR) $(PYTHON_INCLUDE) -std=c11 -O2
LDFLAGS = -shared -lm -L../lib -lwu -Wl,-rpath,$(shell pwd)/../lib

.PHONY: all clean distclean swig build install test docs version

all: swig build

# Generate SWIG wrapper
swig: $(SWIG_WRAPPER)

$(SWIG_WRAPPER): $(SWIG_INTERFACE)
	$(SWIG) $(SWIG_OPTS) -o $(SWIG_WRAPPER) $(SWIG_INTERFACE)

# Build object files
$(WRAPPER_OBJ): $(SWIG_WRAPPER)
	$(CC) $(CFLAGS) -c $(SWIG_WRAPPER) -o $(WRAPPER_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build shared library
build: $(SHARED_LIB)

# Ensure C library is built first
$(WU_LIB):
	$(MAKE) -C ..

$(SHARED_LIB): $(WRAPPER_OBJ) $(WU_LIB)
	$(CC) -shared $(WRAPPER_OBJ) -L../lib -lwu -lm -Wl,-rpath,$(shell pwd)/../lib -o $(SHARED_LIB)

# Install using setup.py (alternative method)
install: swig
	$(PYTHON) setup.py install --user

# Test the bindings
test: $(SHARED_LIB)
	$(PYTHON) -c "import wu; print('Successfully imported wu module')"

# Show version
version:
	@echo "WU Python Bindings version $(WU_VERSION)"

# Generate API documentation
docs: $(SHARED_LIB)
	@echo "Generating API documentation..."
	@if [ ! -d .venv ]; then \
		echo "Creating virtual environment..."; \
		$(PYTHON) -m venv .venv; \
		.venv/bin/pip install pdoc --quiet; \
	fi
	@PYTHONPATH=. .venv/bin/pdoc wu -o docs/
	@echo "Documentation generated in docs/wu.html"
	@echo "Open with: xdg-open docs/wu.html or open docs/wu.html"

# Clean build artifacts (keep SWIG generated files for distribution)
clean:
	rm -f $(WRAPPER_OBJ) $(SHARED_LIB)
	rm -rf build/ dist/ *.egg-info/ docs/
	find . -name "*.pyc" -delete
	find . -name "__pycache__" -type d -delete

# Deep clean including SWIG generated files (for development)
distclean: clean
	rm -f $(SWIG_WRAPPER) $(PYTHON_MODULE)
	rm -rf .venv/
