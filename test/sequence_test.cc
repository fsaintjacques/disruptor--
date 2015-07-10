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
#define BOOST_TEST_MODULE SequenceTest

#include <atomic>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include <disruptor/sequence.h>

namespace disruptor {
namespace test {

struct SequenceFixture {
  Sequence seq;
};

BOOST_FIXTURE_TEST_SUITE(SequenceBasic, SequenceFixture)

BOOST_AUTO_TEST_CASE(ShouldStartWithValueInitialized) {
  BOOST_CHECK_EQUAL(seq.sequence(), kInitialCursorValue);

  seq.set_sequence(2L);
  BOOST_CHECK_EQUAL(seq.sequence(), 2L);

  BOOST_CHECK_EQUAL(seq.IncrementAndGet(1L), 3L);
  BOOST_CHECK_EQUAL(seq.IncrementAndGet(2L), 5L);
}

BOOST_AUTO_TEST_CASE(AtLeastOneCacheLine) {
  BOOST_CHECK(sizeof(Sequence) >= CACHE_LINE_SIZE_IN_BYTES);
}

BOOST_AUTO_TEST_CASE(IsCacheLineAligned) {
  BOOST_CHECK_EQUAL(alignof(Sequence), CACHE_LINE_SIZE_IN_BYTES / 8);
}

BOOST_AUTO_TEST_SUITE_END()  // BlockingStrategy suite

};  // namepspace test
};  // namepspace disruptor
