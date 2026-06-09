#pragma once

#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <NTL/ZZ_pX.h>
#include "tool.h"

// Public-key encryption parameters for the dual-Regev PKE scheme.
struct PkeParams {
    int N, msg_bit, p_bit, q_bit;
    NTL::ZZ msg, p, q, coeff, twice_p, twice_q, half_p;
    NTL::ZZ_pX xN;
    int hsk = 64;
    int d;
    int num_data;
};

// Workspace for PkeDualDecrypt to avoid repeated temporary allocations.
struct PkeWorkspace {
    NTL::ZZ coeff;
    NTL::ZZX temp;
    NTL::ZZ_pX temp1;
    NTL::ZZ_pX temp2;
};

void SetParams(PkeParams &params, int eval_poly);

inline void PkeGen(PkeParams &params, NTL::vec_ZZ_pX &pk, NTL::vec_ZZ_pX &sk, int eval_poly) {
    SetParams(params, eval_poly);
    params.msg_bit = 32;
    NTL::power(params.p, 2, params.p_bit);
    NTL::power(params.q, 2, params.q_bit);
    NTL::ZZ_p::init(params.q);
    NTL::SetCoeff(params.xN, 0, 1);
    NTL::SetCoeff(params.xN, params.N, 1);
    NTL::ZZ_pXModulus modulus(params.xN);
    params.twice_p = 2 * params.p;
    params.twice_q = 2 * params.q;
    params.half_p = params.p / 2;

    NTL::ZZ_pX hat_s, e;
    RandomZZpx(pk[0], params.N, params.q_bit);
    MakeSecretKey(hat_s, params.N, params.hsk);
    GaussRandom(e, params.N);
    NTL::MulMod(pk[1], pk[0], hat_s, modulus);
    pk[1] = pk[1] + e;
    NTL::SetCoeff(sk[0], 0, 1);
    sk[1] = hat_s;
}

inline void PkeEnc(NTL::vec_ZZ_pX &c, const PkeParams &params,
                   const NTL::ZZ_pXModulus &modulus, const NTL::vec_ZZ_pX &pk,
                   const NTL::ZZ_pX x) {
    NTL::ZZ_pX v, e1, e2, temp;
    NTL::ZZ q_div_p = params.q / params.p;
    MakeSecretKey(v, params.N, params.hsk);
    GaussRandom(e1, params.N);
    GaussRandom(e2, params.N);
    NTL::MulMod(c[0], pk[1], v, modulus);
    c[0] = c[0] + e1;
    ZZpxScaleMul(temp, x, q_div_p);
    c[0] = c[0] + temp;
    NTL::MulMod(c[1], pk[0], v, modulus);
    c[1] = e2 - c[1];
}

inline void PkeOkdm(NTL::vec_ZZ_pX &C, const PkeParams &params,
                    const NTL::ZZ_pXModulus &modulus, const NTL::vec_ZZ_pX &pk,
                    const NTL::ZZ_pX x) {
    NTL::ZZ_pX zero;
    zero = 0;
    NTL::ZZ q_div_p = params.q / params.p;
    NTL::vec_ZZ_pX c_xs1, c_xs2;
    NTL::ZZ_pX temp;
    c_xs1.SetLength(2);
    c_xs2.SetLength(2);

    PkeEnc(c_xs1, params, modulus, pk, x);
    PkeEnc(c_xs2, params, modulus, pk, zero);

    ZZpxScaleMul(temp, x, q_div_p);
    c_xs2[1] = c_xs2[1] + temp;
    C[0] = c_xs1[0];
    C[1] = c_xs1[1];
    C[2] = c_xs2[0];
    C[3] = c_xs2[1];
}

// Round one polynomial from mod q to mod p.
static inline void RoundQToP(NTL::ZZ_pX &out, const NTL::ZZ_pX &in,
                             const PkeParams &params, PkeWorkspace &ws) {
    NTL::conv(ws.temp, in);
    for (int i = 0; i < params.N; i++) {
        NTL::GetCoeff(ws.coeff, ws.temp, i);
        ws.coeff = (ws.coeff * params.twice_p + params.q) / (params.twice_q);
        ws.coeff = ws.coeff % params.p;
        if (ws.coeff > params.half_p) ws.coeff -= params.p;
        NTL::SetCoeff(ws.temp, i, ws.coeff);
    }
    NTL::conv(out, ws.temp);
}

// Decrypt and round one half of the dual ciphertext.
static inline void DDecOneHalf(NTL::ZZ_pX &out,
                               const NTL::ZZ_pX &sk0, const NTL::ZZ_pX &sk1,
                               const NTL::ZZ_pX &c0, const NTL::ZZ_pX &c1,
                               const PkeParams &params,
                               const NTL::ZZ_pXModulus &modulus,
                               PkeWorkspace &ws) {
    NTL::MulMod(ws.temp1, sk0, c0, modulus);
    NTL::MulMod(ws.temp2, sk1, c1, modulus);
    ws.temp1 += ws.temp2;
    RoundQToP(out, ws.temp1, params, ws);
}

inline void PkeDualDecrypt(NTL::vec_ZZ_pX &db, const PkeParams &params,
                           const NTL::ZZ_pXModulus &modulus,
                           const NTL::vec_ZZ_pX &sk, const NTL::vec_ZZ_pX &C) {
    PkeWorkspace ws;
    DDecOneHalf(db[0], sk[0], sk[1], C[0], C[1], params, modulus, ws);
    DDecOneHalf(db[1], sk[0], sk[1], C[2], C[3], params, modulus, ws);
}

inline void SetParams(PkeParams &params, int eval_poly) {
    params.N = 32768;
    params.p_bit = 319;
    params.q_bit = 662;
}