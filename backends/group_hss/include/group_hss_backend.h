#pragma once

#include "rms_program.h"
#include "HSSElg.h"

#include <NTL/ZZ.h>
#include <vector>
#include <array>

namespace pvhss { namespace backend {

/// Group HSS backend adapter.
///
/// Wraps the existing pvhss::group::hss primitives behind a uniform
/// interface suitable for the generic RMS evaluator.
struct GroupHssBackend
{
    // --- Type aliases (thin wrappers around existing types) ---

    using PublicKey  = pvhss::group::hss::HssPublicKey;
    using EvalKey    = pvhss::group::hss::HssEvalKey;
    using Ciphertext = pvhss::group::hss::HssCiphertext;
    using Memory     = pvhss::group::hss::HssMemoryValue;

    // --- Key generation ---

    static void GenerateKeys(PublicKey& pk, EvalKey& ek0, EvalKey& ek1, int sk_len)
    {
        pvhss::group::hss::HssGen(pk, ek0, ek1, sk_len);
    }

    // --- Input encoding ---

    static Ciphertext EncodeInput(const PublicKey& pk, const NTL::ZZ& x)
    {
        Ciphertext ct;
        pvhss::group::hss::HssInput(ct, pk, x);
        return ct;
    }

    // --- Constant memory slot ---

    static Memory ConstantMemory(int server_id, const EvalKey& ek, const NTL::ZZ& c)
    {
        Memory mem;
        if (IsZero(c))
        {
            mem[0] = 0;
            mem[1] = 0;
        }
        else
        {
            // Encode constant c as (server_id, c*ek) memory value.
            mem[0] = server_id;
            mem[1] = ek;
            if (c != 1)
            {
                mul(mem[1], mem[1], c);
            }
        }
        return mem;
    }

    // --- Ciphertext operations (for input linear combinations) ---

    /// Add two ciphertexts (component-wise multiplication mod N²).
    static void AddCiphertextsInPlace(Ciphertext& acc, const Ciphertext& rhs,
                                      const PublicKey& pk)
    {
        NTL::MulMod(acc[0][0], acc[0][0], rhs[0][0], pk.N2);
        NTL::MulMod(acc[0][1], acc[0][1], rhs[0][1], pk.N2);
        NTL::MulMod(acc[1][0], acc[1][0], rhs[1][0], pk.N2);
        NTL::MulMod(acc[1][1], acc[1][1], rhs[1][1], pk.N2);
    }

    /// Scale ciphertext by integer coefficient (component-wise exponentiation).
    static void ScaleCiphertextInPlace(Ciphertext& ct, const NTL::ZZ& coeff,
                                       const PublicKey& pk)
    {
        NTL::PowerMod(ct[0][0], ct[0][0], coeff, pk.N2);
        NTL::PowerMod(ct[0][1], ct[0][1], coeff, pk.N2);
        NTL::PowerMod(ct[1][0], ct[1][0], coeff, pk.N2);
        NTL::PowerMod(ct[1][1], ct[1][1], coeff, pk.N2);
    }

    /// Evaluate an input-linear-combination at the ciphertext level.
    /// For single-term PD2 gates this simply returns the raw ciphertext.
    static Ciphertext EvalInputLinComb(
        const PublicKey& pk,
        const std::vector<Ciphertext>& inputs,
        const programs::InputLinComb& lc)
    {
        // Single-term fast path (the common PD2 case)
        if (lc.terms.size() == 1 && lc.terms[0].coeff == 1 && lc.terms[0].index >= 1)
        {
            return inputs[lc.terms[0].index - 1];  // x_index
        }

        // General case: combine ciphertexts
        Ciphertext acc;
        bool first = true;
        for (const auto& t : lc.terms)
        {
            if (t.index == 0)
            {
                // Constant term — encode as ciphertext of constant
                Ciphertext ct_const;
                pvhss::group::hss::HssInput(ct_const, pk, t.coeff);
                if (first) { acc = ct_const; first = false; }
                else       { AddCiphertextsInPlace(acc, ct_const, pk); }
            }
            else
            {
                Ciphertext ct = inputs[t.index - 1];
                if (t.coeff != 1)
                {
                    ScaleCiphertextInPlace(ct, t.coeff, pk);
                }
                if (first) { acc = ct; first = false; }
                else       { AddCiphertextsInPlace(acc, ct, pk); }
            }
        }
        return acc;
    }

    // --- Memory operations ---

    /// Evaluate a memory-linear-combination.
    static Memory EvalMemLinComb(
        const PublicKey& pk,
        const std::vector<Memory>& memory,
        const programs::MemLinComb& lc)
    {
        Memory acc;
        acc[0] = 0;
        acc[1] = 0;
        for (const auto& t : lc.terms)
        {
            Memory scaled = memory[t.index];
            if (t.coeff != 1)
            {
                NTL::MulMod(scaled[0], scaled[0], t.coeff, pk.N);
                NTL::MulMod(scaled[1], scaled[1], t.coeff, pk.N);
            }
            pvhss::group::hss::HssAddMemory(acc, pk, acc, scaled);
        }
        return acc;
    }

    /// Multiply ciphertext × memory → memory (the core HSS operation).
    static Memory MulInputMemory(
        int server_id,
        const PublicKey& pk,
        const EvalKey& ek,
        const Ciphertext& a,
        const Memory& b,
        int& prf_key)
    {
        (void)server_id;
        (void)ek;
        Memory result;
        pvhss::group::hss::HssMul(result, server_id, pk, a, b, prf_key);
        return result;
    }

    /// Add rhs into acc (in-place).
    static void AddInPlace(Memory& acc, const Memory& rhs, const PublicKey& pk)
    {
        pvhss::group::hss::HssAddMemory(acc, pk, acc, rhs);
    }

    /// Multiply memory by scalar (in-place, mod N).
    static void ScalarMulInPlace(Memory& mem, const NTL::ZZ& c, const PublicKey& pk)
    {
        (void)pk;
        NTL::MulMod(mem[0], mem[0], c, pk.N);
        NTL::MulMod(mem[1], mem[1], c, pk.N);
    }

    // --- Decode ---

    static NTL::ZZ Decode(const Memory& m0, const Memory& m1)
    {
        NTL::ZZ result;
        pvhss::group::hss::HssDec(result, m0, m1);
        return result;
    }
};

}} // namespace pvhss::backend
