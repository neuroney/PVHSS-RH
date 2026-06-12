#pragma once

#include "rms_program.h"
#include "HSSRLWE.h"

#include <NTL/ZZ.h>
#include <NTL/ZZ_pX.h>
#include <NTL/ZZX.h>
#include <vector>

namespace pvhss { namespace backend {

/// RLWE HSS backend adapter.
///
/// Wraps the existing pvhss::rlwe::hss primitives.
struct RlweHssBackend
{
    using PkeParams  = pvhss::rlwe::hss::PKE_Para;
    using PublicKey  = NTL::vec_ZZ_pX;       // pk[0], pk[1]
    using EvalKey    = NTL::vec_ZZ_pX;       // hssEk
    using Ciphertext = NTL::vec_ZZ_pX;       // encrypted input (size 2)
    using Memory     = NTL::vec_ZZ_pX;       // memory value (size 2)

    // --- Key generation ---

    static void GenerateKeys(PkeParams& params, PublicKey& pk, EvalKey& ek0,
                             EvalKey& ek1, int /*sk_len*/)
    {
        NTL::vec_ZZ_pX sk;
        pk.SetLength(2);
        sk.SetLength(2);
        ek0.SetLength(2);
        ek1.SetLength(2);
        pvhss::rlwe::hss::SetParams(params);
        pvhss::rlwe::hss::PKE_Gen(params, pk, sk);
        pvhss::rlwe::hss::HssGen(ek0, ek1, params, sk);
    }

    // --- Modulus ---

    static NTL::ZZ_pXModulus MakeModulus(const PkeParams& params)
    {
        return NTL::ZZ_pXModulus(params.xN);
    }

    // --- Input encoding ---

    static Ciphertext EncodeInput(const PkeParams& params,
                                  const NTL::ZZ_pXModulus& modulus,
                                  const PublicKey& pk,
                                  const NTL::ZZ& x)
    {
        Ciphertext ct;
        ct.SetLength(4);
        pvhss::rlwe::hss::HSS_Enc(ct, params, modulus, pk, x);
        return ct;
    }

    // --- Constant memory ---

    static Memory ConstantMemory(int /*server_id*/, const PkeParams& params,
                                 const NTL::ZZ_pXModulus& modulus,
                                 const EvalKey& ek, const NTL::ZZ& c)
    {
        // Encode constant c: encrypt, then convert to memory
        Memory mem;
        mem.SetLength(2);
        if (IsZero(c))
        {
            mem[0] = 0;
            mem[1] = 0;
        }
        else
        {
            // Encode as HSS ciphertext then convert
            NTL::vec_ZZ_pX ct;
            ct.SetLength(4);
            NTL::ZZ_pX c_px;
            NTL::conv(c_px, c);
            // Use OKDM encoding for constant
            pvhss::rlwe::hss::PKE_OKDM(ct, params, modulus, pk, c_px);
            pvhss::rlwe::hss::HssConvertInput(mem, params, modulus, ek, ct);
        }
        return mem;
    }

    // --- Ciphertext linear combination ---

    static Ciphertext EvalInputLinComb(
        const PkeParams& /*params*/,
        const std::vector<Ciphertext>& inputs,
        const programs::InputLinComb& lc)
    {
        // Fast path: single term
        if (lc.terms.size() == 1 && lc.terms[0].coeff == 1 && lc.terms[0].index >= 1)
        {
            return inputs[lc.terms[0].index - 1];
        }
        // General case: combine
        Ciphertext acc;
        acc.SetLength(4);
        bool first = true;
        for (const auto& t : lc.terms)
        {
            if (t.index == 0)
            {
                // Constant term — not fully supported in RLWE without context;
                // for PD2 this path is never taken
                continue;
            }
            Ciphertext ct = inputs[t.index - 1];
            if (t.coeff != 1)
            {
                // Scale by coefficient: multiply polynomial components
                for (long j = 0; j < ct.length(); ++j)
                {
                    NTL::ZZ_pX scaled;
                    NTL::conv(scaled, NTL::conv<NTL::ZZ>(t.coeff));
                    NTL::mul(ct[j], ct[j], scaled);
                }
            }
            if (first) { acc = ct; first = false; }
            else
            {
                for (long j = 0; j < acc.length(); ++j)
                    acc[j] += ct[j];
            }
        }
        return acc;
    }

    // --- Memory linear combination ---

    static Memory EvalMemLinComb(
        const std::vector<Memory>& memory,
        const programs::MemLinComb& lc)
    {
        Memory acc;
        acc.SetLength(2);
        acc[0] = 0; acc[1] = 0;
        for (const auto& t : lc.terms)
        {
            Memory scaled = memory[t.index];
            if (t.coeff != 1)
            {
                NTL::ZZ_pX c_px;
                NTL::conv(c_px, NTL::conv<NTL::ZZ>(t.coeff));
                scaled[0] *= c_px;
                scaled[1] *= c_px;
            }
            pvhss::rlwe::hss::HssAddMemoryInPlace(acc, scaled);
        }
        return acc;
    }

    // --- Multiplication ---

    static Memory MulInputMemory(
        int /*server_id*/,
        const PkeParams& params,
        const NTL::ZZ_pXModulus& modulus,
        const EvalKey& ek,
        const Ciphertext& a,
        const Memory& b,
        int& /*prf_key*/)
    {
        // In RLWE HSS, multiplication converts ciphertext using eval key
        // The PRF is not used the same way as group HSS.
        Memory result;
        result.SetLength(2);
        NTL::vec_ZZ_pX tmp;
        tmp.SetLength(2);
        pvhss::rlwe::hss::HssConvertInput(tmp, params, modulus, ek, a);
        result[0] = tmp[0] + b[0];
        result[1] = tmp[1] + b[1];
        return result;
    }

    // --- Add / Scale ---

    static void AddInPlace(Memory& acc, const Memory& rhs)
    {
        pvhss::rlwe::hss::HssAddMemoryInPlace(acc, rhs);
    }

    static void ScalarMulInPlace(Memory& mem, const NTL::ZZ& c)
    {
        NTL::ZZ_pX c_px;
        NTL::conv(c_px, NTL::conv<NTL::ZZ>(c));
        mem[0] *= c_px;
        mem[1] *= c_px;
    }

    // --- Decode ---

    static NTL::ZZ Decode(const PkeParams& params, const Memory& m0,
                          const Memory& m1)
    {
        // Subtract server shares and decode
        NTL::ZZ_pX diff;
        diff = m1[0] - m0[0];
        NTL::ZZ result;
        NTL::conv(result, NTL::coeff(diff, 0));
        return result;
    }
};

}} // namespace pvhss::backend
