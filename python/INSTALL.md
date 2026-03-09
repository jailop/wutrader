# Installing WU Python Bindings

## From Codeberg (pip install)

### Direct install from git

The easiest way to install (no SWIG required):

```bash
pip install git+https://codeberg.org/jailop/wu.git#subdirectory=python
```

This will:

1. Clone the repository (includes pre-generated SWIG wrappers)
2. Build the C library
3. Compile and install the Python module

**Requirements:** Only a C compiler and Make (SWIG not needed for installation)

### Install from local clone

```bash
# Clone and navigate to python directory
git clone https://codeberg.org/jailop/wu.git
cd wu/python

# Build and install
make
pip install .
```

### Development install (editable)

For development, use editable mode:

```bash
cd wu/python
make
pip install -e .
```

Changes to the C code will require rebuilding:
```bash
make clean && make
```

## System-wide C Library (Optional)

For better performance and to avoid needing rpath, you can install the C library system-wide:

```bash
cd wu
make
sudo make install
sudo ldconfig

# Then Python bindings will use the system library
cd python
pip install .
```

## Requirements

- **System packages:**
  - C compiler (gcc or clang)
  - Python 3.7+
  - Make
  - SWIG 4.0+ (only for development, not for pip install)

- **Python packages:**
  - setuptools (automatically installed with pip)

**Note:** SWIG is only needed if you're modifying the interface (wu.i). For normal installation, pre-generated wrapper files are included in the repository.

## Verification

After installation:

```python
import wu
print(f"WU version: {wu.__version__}")

# Create a moving average
ma = wu.moving_average_new(20)
ma.update(100.0)
print(f"MA value: {ma.value()}")
```

## Troubleshooting

**"cannot find -lwu" error:**

- The setup.py will try to build the C library automatically
- If it fails, build manually: `cd .. && make`

**"ModuleNotFoundError: No module named '_wu'":**

- The extension module failed to build
- Check that you have a C compiler: `gcc --version` or `clang --version`
- Check that the C library built successfully: `ls ../lib/libwu.so*`

**For developers modifying wu.i:**

- SWIG is required to regenerate wrapper files
- Run `make swig` to regenerate wu_wrap.c and wu.py
- The generated files are committed to the repository for users

**"libwu.so.0: cannot open shared object file":**

- Run `sudo ldconfig` if library is installed system-wide
- Or set `LD_LIBRARY_PATH`: `export LD_LIBRARY_PATH=$PWD/../lib:$LD_LIBRARY_PATH`

**Development setup:**

For development, the Makefile approach is recommended as it uses rpath to find the library without system installation.

