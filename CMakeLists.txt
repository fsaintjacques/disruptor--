cmake_minimum_required(VERSION 2.8.10)
project(disruptor)

find_package(Boost 1.49.0 COMPONENTS unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

list(APPEND CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -g -pthread")
include_directories("${PROJECT_SOURCE_DIR}")

add_executable(sequencer_test test/sequencer_test.cc)
target_link_libraries(sequencer_test ${Boost_LIBRARIES})

add_executable(ring_buffer_test test/ring_buffer_test.cc)
target_link_libraries(ring_buffer_test ${Boost_LIBRARIES})

add_executable(perf-1P-1EP-UNICAST perf/one_publisher_to_one_unicast_throughput_test.cc)
target_link_libraries(ring_buffer_test ${Boost_LIBRARIES})

add_executable(perf-1P-3EP-PIPELINE perf/one_publisher_to_three_pipeline_throughput_test.cc)
target_link_libraries(ring_buffer_test ${Boost_LIBRARIES})

