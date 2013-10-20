project "sequencer_test"
  kind "ConsoleApp"
  language "C++"
  files { "disruptor/*.h", "sequencer_test.cc" }
  links "boost_unit_test_framework"

project "ring_buffer_test"
  kind "ConsoleApp"
  language "C++"
  files { "disruptor/*.h", "ring_buffer_test.cc" }
  links "boost_unit_test_framework"

