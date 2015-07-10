disruptor--
===========
[![Build Status](https://travis-ci.org/fsaintjacques/disruptor--.svg?branch=develop)](https://travis-ci.org/fsaintjacques/disruptor--) [![Coverage Status](https://coveralls.io/repos/fsaintjacques/disruptor--/badge.svg?branch=develop)](https://coveralls.io/r/fsaintjacques/disruptor--?branch=develop)

C++ implementation of LMAX's disruptor pattern.

Supported compilers:
  - clang-3.5
  - clang-3.6
  - gcc-4.8
  - gcc-4.9
  - gcc-5

Build instructions
------------------

This library is a header-only library and doesn't require compile step,
move/copy the `disruptor/` folder in one of the include folder.

If you want to develop and/or submit patches to `disruptor--` you need:
  - CMake >= 3.0.1
  - libboost-test

Once dependencies are met

```
# mkdir -p build && cd build
# cmake .. && make all test
```
