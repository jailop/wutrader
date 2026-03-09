#!/usr/bin/env python
"""
Setup script for WU Python bindings

This setup.py uses pre-generated SWIG wrapper files included in the repository.
SWIG is only needed for development, not for installation.
"""

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import subprocess
import os

class CustomBuildExt(build_ext):
    """Custom build to compile C library first if needed"""
    
    def run(self):
        # Build the C library if not already built
        if not os.path.exists('../lib/libwu.so.0.1.0'):
            print("Building WU C library...")
            try:
                subprocess.check_call(['make', '-C', '..'])
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to build C library: {e}")
                print("Please ensure you have a C compiler and Make installed.")
                raise
        
        # Continue with the normal build
        build_ext.run(self)

# Extension module that links to the shared library
wu_module = Extension(
    '_wu',
    sources=['wu_wrap.c'],
    include_dirs=['../include'],
    library_dirs=['../lib'],
    libraries=['wu', 'm'],
    runtime_library_dirs=[os.path.abspath('../lib')],
    extra_compile_args=['-std=c11', '-fPIC']
)

setup(
    ext_modules=[wu_module],
    py_modules=['wu'],
    cmdclass={'build_ext': CustomBuildExt},
)



