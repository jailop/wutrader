# Installation

## Building from Source

Requirements:

- C11 compatible compiler (gcc or clang)
- Make
- CUnit (for running tests)
- Doxygen (optional, for API documentation)

Build Steps:

Clone the repository and build.

```bash
git clone https://codeberg.org/jailop/wutrader.git
cd wutrader
make
```

This builds:

- `lib/libwu.so` - Shared library
- `lib/libwu.a` - Static library
- Example programs in `examples/`
- Test runner in `tests/`

Running Tests:

```bash
make run_tests
```

System Installation:

Install to `/usr/local`.

```bash
sudo make install
sudo ldconfig
```

Install to custom location.

```bash
make install PREFIX=/custom/path
```

Uninstall:

```bash
sudo make uninstall
```

## Generating Documentation

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
