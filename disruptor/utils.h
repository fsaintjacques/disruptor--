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

#ifndef DISRUPTOR_UTILS_H_  // NOLINT
#define DISRUPTOR_UTILS_H_  // NOLINT

inline void SpinPause() {
#if defined(__GNUC__) || defined(__clang__)

#if defined(__i386__) || defined(__x86_64__)

#if defined(__SSE__)
  __asm__ __volatile__("pause");
#else
  __asm__ __volatile__("rep; nop");
#endif

#elif defined(__sparc)
  // high latency nop; "move contents of condition code to gr0"
  // see https://blogs.oracle.com/dave/entry/polite_busy_waiting_with_wrpause
  __asm__ __volatile__("rd %ccr,%g0");
  __asm__ __volatile__("rd %ccr,%g0");
// For Sparc T4 and newer we should use WRPAUSE but currently no suitable
// way to get -mcpu switch from commandline
#endif

#elif __SUNPRO_CC >= 0x5100
  __asm__ __volatile__("rd %ccr,%g0");
  __asm__ __volatile__("rd %ccr,%g0");

#elif defined(_MSC_VER)

  YieldProcessor();

#else

#warning "unsupported platform for pause(); consider implementing it"
  ; /* do nothing */
#endif
}

// From Google C++ Standard, modified to use C++11 deleted functions.
// A macro to disallow the copy constructor and operator= functions.
#define DISALLOW_COPY_MOVE_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;           \
  TypeName(const TypeName&&) = delete;          \
  void operator=(const TypeName&) = delete

#endif  // DISRUPTOR_UTILS_H_ NOLINT
