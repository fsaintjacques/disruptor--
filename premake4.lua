-- meta proj configurations
solution "disruptor"
  configurations { "debug", "release", "profile" }
  location "build"
  buildoptions "-std=c++11"
  includedirs "."
  links "pthread"

  configuration "debug"
    targetdir "build/bin/debug"

  configuration "release"
    targetdir "build/bin/release"

  configuration "profile"
    targetdir "build/bin/profile"
    buildoptions "--coverage"
    linkoptions "--coverage"

  include "test"
  include "perf"

