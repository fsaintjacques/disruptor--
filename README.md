disruptor--
===========
[![Build Status](https://travis-ci.org/karopawil/disruptor--.svg?branch=develop)](https://travis-ci.org/karopawil/disruptor--) [![Coverage Status](https://coveralls.io/repos/karopawil/disruptor--/badge.svg?branch=develop)](https://coveralls.io/r/karopawil/disruptor--?branch=develop)

C++ implementation of LMAX's disruptor pattern.

Ought to compile and run with gcc 5 and 6, clang 3.6 and 3.8, Visual Studio 2015.

Examples can be run like (./example_* -h to see available options):

./example_bin --np 1 --nc 1 --mt 0 --bs 1 -l 1000 --rb 65536
./example_bin --np 1 --nc 1 --mt 0 --bs 5 -l 1000 --rb 65536
./example_bin --nc 3 --np 1 --bs 1 --mt 0 --rb 8192 -l 5000
./example_bin --nc 1 --np 3 --bs 1 --mt 2 --rb 8192 -l 1000

./example_pipeline_bin --bs 1 --rb 8192 -l 10000

./example_diamond_bin --bs 1 --rb 8192 -l 10000

etc.

