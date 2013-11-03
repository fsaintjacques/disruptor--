project "perf_1p_1ep_unicast"
  kind "ConsoleApp"
  language "C++"
  files { "one_publisher_to_one_unicast_throughput_test.cc" }

project "perf_1p_3ep_pipeline"
  kind "ConsoleApp"
  language "C++"
  files { "one_publisher_to_three_pipeline_throughput_test.cc" }

project "perf-1p3c"
  kind "ConsoleApp"
  language "C++"
  files { "1p3c.cpp" }

