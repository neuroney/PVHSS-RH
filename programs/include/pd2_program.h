#pragma once

#include "rms_program.h"
#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace programs {

/// Build an RMS program for the PD2 (Power-sum Degree-2) computation.
///
/// Uses the optimized complete-homogeneous recurrence:
///   H_0[0] = 1,  H_0[s] = 0 for s > 0
///   H_i[0] = 1  for all i
///   H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]   for s = 1..d
///
/// Output: y = sum_{s=1}^{d} H_m[s]
///
/// Creates exactly msg_num * degree_f RMS multiplication gates.
///
/// Memory layout:
///   slot 0: constant 1
///   slots 1..degree_f:     H_prev[1..degree_f]  (H_{i-1})
///   slots degree_f+1 .. 2*degree_f: H_curr[1..degree_f]  (H_i)
///
/// The evaluator pre-copies H_prev[*] into H_curr[*] at the start of each
/// input iteration via init_copies, then gates accumulate into H_curr[*].
RmsProgram BuildPd2RmsProgram(int msg_num, int degree_f);

}} // namespace pvhss::programs
