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
LDFLAGS = -shared -lm

.PHONY: all clean swig build install test

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

$(SHARED_LIB): $(WRAPPER_OBJ) $(WU_OBJS)
	$(CC) $(LDFLAGS) $(WRAPPER_OBJ) $(WU_OBJS) -o $(SHARED_LIB)

# Install using setup.py (alternative method)
install: swig
	$(PYTHON) setup.py install --user

# Test the bindings
test: $(SHARED_LIB)
	$(PYTHON) -c "import wu; print('Successfully imported wu module')"

# Clean build artifacts
clean:
	rm -f $(SWIG_WRAPPER) $(PYTHON_MODULE) $(WRAPPER_OBJ) $(SHARED_LIB)
	find $(SRC_DIR) -name "*.o" -delete
	rm -rf build/ dist/ *.egg-info/
	find . -name "*.pyc" -delete
	find . -name "__pycache__" -type d -delete
