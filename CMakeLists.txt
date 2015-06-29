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

cmake_minimum_required(VERSION 2.8.0)
project(disruptor CXX)

find_package(Boost 1.49.0 COMPONENTS unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

list(APPEND CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -g -pthread")
include_directories("${PROJECT_SOURCE_DIR}")

# tests

enable_testing()

add_executable(sequencer_test_bin test/sequencer_test.cc)
target_link_libraries(sequencer_test_bin ${Boost_LIBRARIES})
add_test(sequencer_test sequencer_test_bin)

add_executable(ring_buffer_test_bin test/ring_buffer_test.cc)
target_link_libraries(ring_buffer_test_bin ${Boost_LIBRARIES})
add_test(ring_buffer_test ring_buffer_test_bin)

add_executable(perf-1P-1EP-UNICAST
  perf/one_publisher_to_one_unicast_throughput_test.cc)
target_link_libraries(ring_buffer_test_bin ${Boost_LIBRARIES})

add_executable(perf-1P-3EP-PIPELINE
  perf/one_publisher_to_three_pipeline_throughput_test.cc)
target_link_libraries(ring_buffer_test_bin ${Boost_LIBRARIES})
