#pragma once

// Tool functions specific to the CZ pairing-based PVHSS scheme.
// Common helpers (timing, integer conversion, NTL wrappers) come from helper.h.

#include <gmp.h>
extern "C" {
#include <relic/relic.h>
}
#include "helper.h"
#include <sstream>

// Convert a decimal ZZ to a binary ZZ_pX polynomial (bit-length = bits).
inline void DecimalToBinary(NTL::ZZ_pX &out, NTL::ZZ in, int bits) {
    NTL::ZZ rem;
    NTL::ZZX out_zzx;
    for (int i = 0; i < bits; i++) {
        rem = in % 2;
        NTL::SetCoeff(out_zzx, i, rem);
        in = (in - rem) / 2;
    }
    NTL::conv(out, out_zzx);
}

// Evaluate a ZZX polynomial at x = 2 (Horner-style).
inline void EvalZZX(NTL::ZZ &y, NTL::ZZX f) {
    NTL::ZZ coeff, pow2;
    pow2 = 1;
    for (int i = 0; i < NTL::deg(f) + 1; i++) {
        NTL::GetCoeff(coeff, f, i);
        y = y + coeff * pow2;
        pow2 = pow2 * 2;
    }
}

// Generate a sparse secret key with hsk non-zero coefficients for CZ scheme.
inline void MakeSecretKey(NTL::ZZ_pX &sk, int N, int hsk) {
    int interval = N / hsk;
    int index = rand() % interval;
    for (int i = 0; i < hsk; i++) {
        NTL::SetCoeff(sk, index, 1);
        index = index + rand() % interval;
    }
}

