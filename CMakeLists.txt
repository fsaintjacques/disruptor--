# Copyright (c) 2011-2015, François Saint-Jacques
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the disruptor-- nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL FRANÇOIS SAINT-JACQUES BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 3.1.0)

execute_process(COMMAND ${PROJECT_SOURCE_DIR}../tools/semver
  OUTPUT_VARIABLE SEMVER_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
project(disruptor VERSION ${SEMVER_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

add_library(disruptor INTERFACE)
include_directories("${PROJECT_SOURCE_DIR}")

# options
option(COVERALLS "Generate coverage data" OFF)
option(COVERALLS_UPLOAD "Upload the generated coveralls json" OFF)

# dependencies
find_package(Boost 1.46.0 COMPONENTS unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

# code coverage
set(COVERAGE_MODUDLE_PATH ${PROJECT_SOURCE_DIR}/tools/coveralls-cmake/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/tools/coveralls-cmake/cmake)
if (COVERALLS)
  set(COVERAGE_SRCS ${PROJECT_SOURCE_DIR}/disruptor/sequence.h
                    ${PROJECT_SOURCE_DIR}/disruptor/ring_buffer.h
                    ${PROJECT_SOURCE_DIR}/disruptor/wait_strategy.h
                    ${PROJECT_SOURCE_DIR}/disruptor/claim_strategy.h
                    ${PROJECT_SOURCE_DIR}/disruptor/sequence_barrier.h
                    ${PROJECT_SOURCE_DIR}/disruptor/sequencer.h)
  include(Coveralls)
  coveralls_turn_on_coverage()
  coveralls_setup(
    "${COVERAGE_SRCS}"
    ${COVERALLS_UPLOAD}
    "${PROJECT_SOURCE_DIR}/tools/coveralls-cmake/cmake")
endif()

# tests
enable_testing()

add_executable(sequence_test_bin test/sequence_test.cc)
target_link_libraries(sequence_test_bin ${Boost_LIBRARIES})
add_test(sequence_test sequence_test_bin)

add_executable(ring_buffer_test_bin test/ring_buffer_test.cc)
target_link_libraries(ring_buffer_test_bin ${Boost_LIBRARIES})
add_test(ring_buffer_test ring_buffer_test_bin)

add_executable(wait_strategy_test_bin test/wait_strategy_test.cc)
target_link_libraries(wait_strategy_test_bin ${Boost_LIBRARIES})
add_test(wait_strategy_test wait_strategy_test_bin)

add_executable(sequence_barrier_test_bin test/sequence_barrier_test.cc)
target_link_libraries(sequence_barrier_test_bin ${Boost_LIBRARIES})
add_test(sequence_barrier_test sequence_barrier_test_bin)

add_executable(claim_strategy_test_bin test/claim_strategy_test.cc)
target_link_libraries(claim_strategy_test_bin ${Boost_LIBRARIES})
add_test(claim_strategy_test claim_strategy_test_bin)

add_executable(sequencer_test_bin test/sequencer_test.cc)
target_link_libraries(sequencer_test_bin ${Boost_LIBRARIES})
add_test(sequencer_test sequencer_test_bin)
