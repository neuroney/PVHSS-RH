#pragma once

/// CZ backend: low-level PKE and pairing primitives.
///
/// The CZ protocol uses a dual-Regev PKE scheme combined with bilinear pairings.
/// This backend exposes the low-level operations; the protocol flow lives in
/// protocols/cz.

#include "PKE.h"
#include "PVHSS.h"
#include "GenData.h"
#include "tool.h"
#include "helper.h"

#include <NTL/ZZ.h>
#include <NTL/ZZ_pX.h>

namespace pvhss { namespace backend {

struct CzBackend
{
    using PkeParams    = PkeParams;
    using PvhssParams  = PvhssParams;
    using PublicKey    = NTL::vec_ZZ_pX;
    using SecretKey    = NTL::vec_ZZ_pX;
    using EvalKey      = NTL::vec_ZZ_pX;
    using Ciphertext   = NTL::vec_ZZ_pX;  // size 4
    using Memory       = NTL::vec_ZZ_pX;  // size 2
    using EncodedData  = ::EncodedData;

    // --- PKE key generation ---

    static void PkeKeyGen(PkeParams& params, PublicKey& pk, SecretKey& sk,
                          int degree_f)
    {
        pk.SetLength(2);
        sk.SetLength(2);
        PkeGen(params, pk, sk, degree_f);
        params.d = degree_f;
    }

    // --- PVHSS setup ---

    static void PvhssSetup(EvalKey& ek1, EvalKey& ek2,
                           NTL::vec_ZZ_pX& C_alpha,
                           PvhssParams& pvhss_params,
                           const PkeParams& params,
                           const NTL::ZZ_pXModulus& modulus,
                           const PublicKey& pk,
                           const SecretKey& sk)
    {
        ek1.SetLength(2);
        ek2.SetLength(2);
        C_alpha.SetLength(4);
        PvhssGen(ek1, ek2, C_alpha, pvhss_params, params, modulus, pk, sk);
    }

    // --- Input encoding ---

    static Ciphertext EncodeInput(const PkeParams& params,
                                  const NTL::ZZ_pXModulus& modulus,
                                  const PublicKey& pk,
                                  const NTL::ZZ_pX& x_px)
    {
        Ciphertext ct;
        ct.SetLength(4);
        PvhssEnc(ct, params, modulus, pk, x_px);
        return ct;
    }

    // --- Multiply (decrypt share) ---

    static void Multiply(Memory& out, const PkeParams& params,
                         const NTL::ZZ_pXModulus& modulus,
                         const EvalKey& ek, const Ciphertext& ct)
    {
        out.SetLength(2);
        PvhssMult(out, params, modulus, ek, ct);
    }

    // --- Memory operations ---

    static void AddInPlace(Memory& acc, const Memory& rhs)
    {
        PvhssAddInPlace(acc, rhs);
    }

    // --- Pairing verification ---

    static bool VerifyPairing(const NTL::ZZ_pX& y1, const NTL::ZZ_pX& y2,
                              ep_t g1T1, ep2_t g2T2,
                              const PvhssParams& pvhss_params)
    {
        return PvhssVerify(y1, y2, g1T1, g2T2, pvhss_params);
    }
};

}} // namespace pvhss::backend
