#pragma once

#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace programs {

/// Plaintext reference evaluator for PD2.
///
/// Computes the complete homogeneous symmetric polynomial:
///   y = sum_{s=1}^{d} H_m[s]
/// where H_i[s] follows the recurrence H_i[s] = H_{i-1}[s] + x_i * H_i[s-1].
///
/// @param x       private input vector of length m
/// @param degree_f  maximum degree d
/// @param modulus   modulus for reduction (0 = no reduction, i.e. exact integer)
/// @return the PD2 output
NTL::ZZ PlaintextPd2(
    const std::vector<NTL::ZZ>& x,
    int degree_f,
    const NTL::ZZ& modulus
);

}} // namespace pvhss::programs
