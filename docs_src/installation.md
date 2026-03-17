# Installation

## Building from Source

### Requirements

- C11 compatible compiler (gcc or clang)
- Make
- Doxygen (optional, for API documentation)

### Build Steps

Clone the repository and build:

```bash
git clone <repository-url>
cd wu
make
```

This builds:
- `lib/libwu.so` - Shared library
- `lib/libwu.a` - Static library
- Example programs in `examples/`
- Test runner in `tests/`

### Running Tests

```bash
cd tests
./test_runner
```

The test suite uses CUnit. Some tests may have known issues—this is an
experimental project under development.

### System Installation

Install to `/usr/local`:

```bash
sudo make install
sudo ldconfig
```

Install to custom location:

```bash
make install PREFIX=/custom/path
```

Uninstall:

```bash
sudo make uninstall
```

### Generating Documentation

Build Doxygen API documentation:

```bash
make docs
```

This creates `docs/html/index.html` with complete API reference.

Build MkDocs user documentation:

```bash
# Install mkdocs if needed
pip install mkdocs

# Build documentation
mkdocs build

# Or serve locally
mkdocs serve
```

This creates `docs/` with tutorial, design notes, and examples.

## Python Bindings

### Using pip

```bash
pip install wu-trading
```

### Building from Source

Requirements:
- Python 3.7+
- SWIG 4.0+ (for development only)

Build and install:

```bash
cd python
pip install -e .
```

## Usage in Your Project

### C Projects

After installation, link against Wu:

```bash
gcc -o myprogram myprogram.c -lwu
```

Or using the shared library from build directory:

```bash
gcc -I./include -o myprogram myprogram.c -L./lib -lwu -Wl,-rpath,./lib
```

### Python Projects

```python
import wu

portfolio = wu.BasicPortfolio(...)
strategy = wu.CrossOverStrat(10, 30, 0.0)
runner = wu.Runner(portfolio, strategy, [reader])
runner.exec()
```

## Development Setup

For contributing or modifying the library:

```bash
# Clone repository
git clone <repository-url>
cd wu

# Build in debug mode (default)
make

# Build with optimization
make CFLAGS="-O3 -DNDEBUG"

# Clean build artifacts
make clean

# Run tests
cd tests && ./test_runner
```

The Makefile supports standard targets and respects common environment
variables like `CC`, `CFLAGS`, and `PREFIX`.
