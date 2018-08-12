/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "../base/random.h"

#include <math.h>

#include "../base/checks.h"

namespace yuntongxunwebrtc {

Random::Random(uint64_t seed) {
  DCHECK(seed != 0x0ull);
  state_ = seed;
}

uint32_t Random::Rand(uint32_t t) {
  // Casting the output to 32 bits will give an almost uniform number.
  // Pr[x=0] = (2^32-1) / (2^64-1)
  // Pr[x=k] = 2^32 / (2^64-1) for k!=0
  // Uniform would be Pr[x=k] = 2^32 / 2^64 for all 32-bit integers k.
  uint32_t x = NextOutput();
  // If x / 2^32 is uniform on [0,1), then x / 2^32 * (t+1) is uniform on
  // the interval [0,t+1), so the integer part is uniform on [0,t].
  uint64_t result = x * (static_cast<uint64_t>(t) + 1);
  result >>= 32;
  return result;
}

uint32_t Random::Rand(uint32_t low, uint32_t high) {
  DCHECK(low <= high);
  return Rand(high - low) + low;
}

int32_t Random::Rand(int32_t low, int32_t high) {
  DCHECK(low <= high);
  // We rely on subtraction (and addition) to be the same for signed and
  // unsigned numbers in two-complement representation. Thus, although
  // high - low might be negative as an int, it is the correct difference
  // when interpreted as an unsigned.
  return Rand(high - low) + low;
}

template <>
float Random::Rand<float>() {
  double result = NextOutput() - 1;
  result = result / 0xFFFFFFFFFFFFFFFEull;
  return static_cast<float>(result);
}

template <>
double Random::Rand<double>() {
  double result = NextOutput() - 1;
  result = result / 0xFFFFFFFFFFFFFFFEull;
  return result;
}

template <>
bool Random::Rand<bool>() {
  return Rand(0, 1) == 1;
}

double Random::Gaussian(double mean, double standard_deviation) {
  // Creating a Normal distribution variable from two independent uniform
  // variables based on the Box-Muller transform, which is defined on the
  // interval (0, 1]. Note that we rely on NextOutput to generate integers
  // in the range [1, 2^64-1]. Normally this behavior is a bit frustrating,
  // but here it is exactly what we need.
  const double kPi = 3.14159265358979323846;
  double u1 = static_cast<double>(NextOutput()) / 0xFFFFFFFFFFFFFFFFull;
  double u2 = static_cast<double>(NextOutput()) / 0xFFFFFFFFFFFFFFFFull;
  return mean + standard_deviation * sqrt(-2 * log(u1)) * cos(2 * kPi * u2);
}

double Random::Exponential(double lambda) {
  double uniform = Rand<double>();
  return -log(uniform) / lambda;
}

}  // namespace yuntongxunwebrtc
