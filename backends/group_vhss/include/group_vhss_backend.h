#pragma once

#include "rms_program.h"
#include "VHSSElg.h"
#include "elgamal.h"

#include <NTL/ZZ.h>
#include <vector>
#include <array>

namespace pvhss { namespace backend {

/// Group VHSS backend adapter.
///
/// Wraps the existing VhssElgamal primitives behind a uniform interface
/// suitable for the generic RMS evaluator.
struct GroupVhssBackend
{
    // --- Type aliases ---

    using PublicKey  = VhssElgamalPk;
    using EvalKey    = VhssElgamalEk;     // std::array<NTL::ZZ, 3>
    using VerifKey   = VhssElgamalVk;     // NTL::ZZ
    using Ciphertext = VhssElgamalCt;     // std::array<ElgamalCiphertext, 2>
    using Memory     = VhssElgamalMv;     // std::array<NTL::ZZ, 4>

    // --- Key generation ---

    static void GenerateKeys(PublicKey& pk, VerifKey& vk, EvalKey& ek0, EvalKey& ek1,
                             int sk_len, int vk_len)
    {
        VhssElgamalGen(pk, vk, ek0, ek1, sk_len, vk_len);
    }

    static void GenerateKeys(PublicKey& pk, EvalKey& ek0, EvalKey& ek1,
                             int sk_len, int vk_len)
    {
        VhssElgamalGen(pk, ek0, ek1, sk_len, vk_len);
    }

    // --- Input encoding ---

    static Ciphertext EncodeInput(const PublicKey& pk, const NTL::ZZ& x)
    {
        Ciphertext ct;
        VhssElgamalInput(ct, pk, x);
        return ct;
    }

    // --- Constant memory slot ---

    static Memory ConstantMemory(int server_id, const EvalKey& ek, const NTL::ZZ& c)
    {
        Memory mem;
        if (IsZero(c))
        {
            mem[0] = 0; mem[1] = 0; mem[2] = 0; mem[3] = 0;
        }
        else
        {
            mem[0] = server_id;
            mem[1] = ek[0];
            mem[2] = ek[1];
            mem[3] = ek[2];
            if (c != 1)
            {
                mul(mem[1], mem[1], c);
                mul(mem[3], mem[3], c);
            }
        }
        return mem;
    }

    // --- Ciphertext operations ---

    static void AddCiphertextsInPlace(Ciphertext& acc, const Ciphertext& rhs,
                                      const PublicKey& pk)
    {
        NTL::MulMod(acc[0][0], acc[0][0], rhs[0][0], pk.N2);
        NTL::MulMod(acc[0][1], acc[0][1], rhs[0][1], pk.N2);
        NTL::MulMod(acc[1][0], acc[1][0], rhs[1][0], pk.N2);
        NTL::MulMod(acc[1][1], acc[1][1], rhs[1][1], pk.N2);
    }

    static void ScaleCiphertextInPlace(Ciphertext& ct, const NTL::ZZ& coeff,
                                       const PublicKey& pk)
    {
        NTL::PowerMod(ct[0][0], ct[0][0], coeff, pk.N2);
        NTL::PowerMod(ct[0][1], ct[0][1], coeff, pk.N2);
        NTL::PowerMod(ct[1][0], ct[1][0], coeff, pk.N2);
        NTL::PowerMod(ct[1][1], ct[1][1], coeff, pk.N2);
    }

    static Ciphertext EvalInputLinComb(
        const PublicKey& pk,
        const std::vector<Ciphertext>& inputs,
        const programs::InputLinComb& lc)
    {
        if (lc.terms.size() == 1 && lc.terms[0].coeff == 1 && lc.terms[0].index >= 1)
        {
            return inputs[lc.terms[0].index - 1];
        }

        Ciphertext acc;
        bool first = true;
        for (const auto& t : lc.terms)
        {
            if (t.index == 0)
            {
                Ciphertext ct_const;
                VhssElgamalInput(ct_const, pk, t.coeff);
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

    static Memory EvalMemLinComb(
        const PublicKey& pk,
        const std::vector<Memory>& memory,
        const programs::MemLinComb& lc)
    {
        Memory acc;
        acc[0] = 0; acc[1] = 0; acc[2] = 0; acc[3] = 0;
        for (const auto& t : lc.terms)
        {
            Memory scaled = memory[t.index];
            if (t.coeff != 1)
            {
                NTL::MulMod(scaled[0], scaled[0], t.coeff, pk.N);
                NTL::MulMod(scaled[1], scaled[1], t.coeff, pk.N);
                NTL::MulMod(scaled[2], scaled[2], t.coeff, pk.N);
                NTL::MulMod(scaled[3], scaled[3], t.coeff, pk.N);
            }
            VhssElgamalAddMemory(acc, pk, acc, scaled);
        }
        return acc;
    }

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
        VhssElgamalMul(result, server_id, pk, a, b, prf_key);
        return result;
    }

    static void AddInPlace(Memory& acc, const Memory& rhs, const PublicKey& pk)
    {
        VhssElgamalAddMemory(acc, pk, acc, rhs);
    }

    static void ScalarMulInPlace(Memory& mem, const NTL::ZZ& c, const PublicKey& pk)
    {
        (void)pk;
        NTL::MulMod(mem[0], mem[0], c, pk.N);
        NTL::MulMod(mem[1], mem[1], c, pk.N);
        NTL::MulMod(mem[2], mem[2], c, pk.N);
        NTL::MulMod(mem[3], mem[3], c, pk.N);
    }

    // --- Verification ---

    static bool Verify(const Memory& y0, const Memory& y1, const VerifKey& vk)
    {
        return VhssElgamalVerify(y0, y1, vk);
    }
};

}} // namespace pvhss::backend
