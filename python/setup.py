#!/usr/bin/env python

from distutils.core import setup, Extension
import glob

# Find all C source files
wu_sources = ['wu_wrap.c'] + glob.glob('../src/**/*.c', recursive=True)

wu_module = Extension(
    '_wu',
    sources=wu_sources,
    include_dirs=['../include'],
    libraries=['m'],
    extra_compile_args=['-std=c11']
)

setup(
    name='wu',
    version='0.1',
    author='DataInquiry',
    description='Python bindings for TZU backtesting library',
    ext_modules=[wu_module],
    py_modules=['wu'],
)
