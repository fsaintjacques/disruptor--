
#--------------------------------
# find BOOST - see http://www.cmake.org/cmake/help/cmake-2-8-docs.html#module:FindBoost

OPTION ( Boost_DEBUG "Whether to print debug info when cmake is looking for Boost" OFF )
set ( Boost_DETAILED_FAILURE_MSG TRUE )

set ( Boost_USE_MULTITHREADED ${DISRUPTOR_Boost_USES_MULTITHREADED} )
set ( Boost_USE_STATIC_LIBS ${DISRUPTOR_Boost_USES_STATIC_LIBS} )
set ( Boost_USE_STATIC_RUNTIME ${DISRUPTOR_Boost_USES_STATIC_RUNTIME} )
add_definitions ( -DBOOST_ALL_NO_LIB )
add_definitions ( -DBOOST_LIB_DIAGNOSTIC )

find_package ( Boost COMPONENTS unit_test_framework REQUIRED )

#--------------------------------

set ( TEST_SUPPORT_HEADERS 
  ${CMAKE_CURRENT_SOURCE_DIR}/support/long_event.h
  ${CMAKE_CURRENT_SOURCE_DIR}/support/stub_event.h
)

add_executable ( sequencer_test sequencer_test.cc ${TEST_SUPPORT_HEADERS} )
target_link_libraries ( sequencer_test ${Boost_LIBRARIES} )

add_executable ( ring_buffer_test ring_buffer_test.cc ${TEST_SUPPORT_HEADERS} )
target_link_libraries ( ring_buffer_test ${Boost_LIBRARIES} )

