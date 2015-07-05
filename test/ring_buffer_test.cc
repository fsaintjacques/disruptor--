// Copyright (c) 2011-2015, Francois Saint-Jacques
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the disruptor-- nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL FRANCOIS SAINT-JACQUES BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE RingBufferTest

#define RING_BUFFER_SIZE 8

#include <boost/test/unit_test.hpp>
#include <disruptor/ring_buffer.h>

namespace disruptor {
namespace test {

struct RingBufferFixture {
  RingBufferFixture() : ring_buffer(initArray()) {}

  size_t f(const size_t i) { return i + 1; }

  std::array<int, RING_BUFFER_SIZE> initArray() {
    std::array<int, RING_BUFFER_SIZE> tmp;
    for (size_t i = 0; i < RING_BUFFER_SIZE; i++) tmp[i] = f(i);
    return tmp;
  }

  RingBuffer<int, RING_BUFFER_SIZE> ring_buffer;
};

BOOST_AUTO_TEST_SUITE(RingBufferBasic)

BOOST_FIXTURE_TEST_CASE(VerifyWrapArround, RingBufferFixture) {
  for (size_t i = 0; i < RING_BUFFER_SIZE * 2; i++)
    BOOST_CHECK_EQUAL(ring_buffer[i], f(i % RING_BUFFER_SIZE));

  for (size_t i = 0; i < RING_BUFFER_SIZE * 2; i++)
    const auto& t = ring_buffer[i];
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace test
};  // namespace disruptor
