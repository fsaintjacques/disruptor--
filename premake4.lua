-- meta proj configurations
solution "disruptor"
  configurations { "Debug", "Release" }
  location "build"
  buildoptions "-std=c++11"
  includedirs "."
  links "pthread"
  targetdir "build/bin"

  configuration "Debug"
    targetsuffix "-g"

  include "test"
  include "perf"

