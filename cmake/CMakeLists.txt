# CMake build system for disruptor-- 
# 2012, Joao Paulo Magalhaes <jpmagalhaes@ist.utl.pt>

cmake_minimum_required ( VERSION 2.8 )

project ( disruptor )
set_property ( GLOBAL PROPERTY USE_FOLDERS ON )

set ( DISRUPTOR_MAJOR_VERSION 0 CACHE STRING "QuantLib major version number." FORCE )
set ( DISRUPTOR_MINOR_VERSION 0 CACHE STRING "QuantLib minor version number." FORCE )
set ( DISRUPTOR_VERSION_STRING "${DISRUPTOR_MAJOR_VERSION}.${DISRUPTOR_MINOR_VERSION}" )

#--------------------------------------------------
# initial settings - cached

OPTION ( DISRUPTOR_BUILD_PERF "Whether to build performance tests." ON )
OPTION ( DISRUPTOR_BUILD_TEST "Whether to build tests." ON )

# tests require boost::unit_test_framework which must be linked
if ( DISRUPTOR_BUILD_TEST )
  OPTION ( DISRUPTOR_Boost_USES_MULTITHREADED "Use the multithreaded Boost libraries" OFF )
  OPTION ( DISRUPTOR_Boost_USES_STATIC_LIBS "Use the static Boost libraries" OFF )
  OPTION ( DISRUPTOR_Boost_USES_STATIC_RUNTIME
  "Searches for Boost libraries linked against a static C++ standard library (ABI tag s)." OFF )
endif ()


#--------------------------------------------------
# initial settings - non-cached

set ( DISRUPTOR_ROOT_DIR  ${PROJECT_SOURCE_DIR}/.. )
# resolve the path without any relative components
get_filename_component ( tmp ${DISRUPTOR_ROOT_DIR} REALPATH )
set ( DISRUPTOR_ROOT_DIR  ${tmp} ) 

set ( DISRUPTOR_SRC_DIR   ${DISRUPTOR_ROOT_DIR}/disruptor )
set ( DISRUPTOR_PERF_DIR  ${DISRUPTOR_ROOT_DIR}/perf )
set ( DISRUPTOR_TEST_DIR  ${DISRUPTOR_ROOT_DIR}/test )
set ( DISRUPTOR_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR} )

#--------------------------------------------------
# C++11 features

set ( CXX11_REQUIRED_FEATURES RANGE_BASED_FOR CONSTEXPR )
include ( CheckCXX11Features.cmake )
add_definitions ( ${CXX11_REQUIRED_FLAGS} )

#--------------------------------------------------
# third-party libraries

set ( DISRUPTOR_LIBRARIES )

#---------------------------------------------------------
# deal with the source files

set ( DISRUPTOR_HEADERS 
  ${DISRUPTOR_SRC_DIR}/batch_descriptor.h
  ${DISRUPTOR_SRC_DIR}/claim_strategy.h
  ${DISRUPTOR_SRC_DIR}/event_processor.h
  ${DISRUPTOR_SRC_DIR}/event_publisher.h
  ${DISRUPTOR_SRC_DIR}/exception_handler.h
  ${DISRUPTOR_SRC_DIR}/exceptions.h
  ${DISRUPTOR_SRC_DIR}/interface.h
  ${DISRUPTOR_SRC_DIR}/ring_buffer.h
  ${DISRUPTOR_SRC_DIR}/sequence_barrier.h
  ${DISRUPTOR_SRC_DIR}/sequence.h
  ${DISRUPTOR_SRC_DIR}/sequencer.h
  ${DISRUPTOR_SRC_DIR}/utils.h
  ${DISRUPTOR_SRC_DIR}/wait_strategy.h
)
set ( DISRUPTOR_SOURCES )

include ( CreateSourceGroup.cmake )
create_source_group ( "Header Files" ${DISRUPTOR_SRC_DIR} "${DISRUPTOR_HEADERS}" )
create_source_group ( "Source Files" ${DISRUPTOR_SRC_DIR} "${DISRUPTOR_SOURCES}" )

#---------------------------------------------------------
# create the library

include_directories ( 
  ${DISRUPTOR_ROOT_DIR}
  ${DISRUPTOR_INCLUDE_DIRS}
)

# not necessary to add_library() since there are no source files

#----------------------------------------------------------------------
# perf

if ( DISRUPTOR_BUILD_PERF )
  add_subdirectory ( ${DISRUPTOR_PERF_DIR} perf )
endif ()

#----------------------------------------------------------------------
# test

if ( DISRUPTOR_BUILD_TEST )
  add_subdirectory ( ${DISRUPTOR_TEST_DIR} test )
endif ()

