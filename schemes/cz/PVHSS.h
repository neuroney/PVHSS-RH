#pragma once

#include <gmp.h>

extern "C" {
#include <relic/relic.h>
}

#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <NTL/ZZ_pX.h>
#include <NTL/matrix.h>
#include "PKE.h"

// PVHSS pairing parameters.
struct PvhssParams {
    NTL::ZZ g1_order_ZZ;
    NTL::ZZ g2_order_ZZ;
    ep_t g1_gen;
    ep2_t g2_gen;
    fp12_t A_gT;
    fp12_t const_gT;
};

// Workspace for Eval DP to avoid repeated allocations and copies.
struct PvhssEvalWorkspace {
    NTL::Mat<NTL::ZZ_pX> dp_prev;
    NTL::Mat<NTL::ZZ_pX> dp_curr;
    NTL::vec_ZZ_pX chain;
    NTL::vec_ZZ_pX next;
};

inline void InitPvhssEvalWorkspace(PvhssEvalWorkspace &ws, int degree_f) {
    ws.dp_prev.SetDims(degree_f + 1, 2);
    ws.dp_curr.SetDims(degree_f + 1, 2);
    ws.chain.SetLength(2);
    ws.next.SetLength(2);
}

// Forward declaration.
void PvhssEnc(NTL::vec_ZZ_pX &C, const PkeParams &params,
              const NTL::ZZ_pXModulus &modulus, const NTL::vec_ZZ_pX &pk,
              const NTL::ZZ_pX &x);

inline void PvhssGen(NTL::vec_ZZ_pX &hss_ek1, NTL::vec_ZZ_pX &hss_ek2,
                     NTL::vec_ZZ_pX &C_alpha, PvhssParams &pvhss_params,
                     const PkeParams &params, const NTL::ZZ_pXModulus &modulus,
                     const NTL::vec_ZZ_pX &pk, const NTL::vec_ZZ_pX &sk) {
    ep_new(pvhss_params.g1_gen);
    ep2_new(pvhss_params.g2_gen);
    fp12_new(pvhss_params.A_gT);
    fp12_new(pvhss_params.const_gT);
    core_init();
    ReseedRelicBenchmarkRandomness();
    ep_curve_init();
    ep_param_set(B12_P381);
    ep2_curve_init();
    ep2_curve_set_twist(RLC_EP_MTYPE);
    ep_curve_get_gen(pvhss_params.g1_gen);
    ep2_curve_get_gen(pvhss_params.g2_gen);

    // Evaluation keys
    RandomZZpx(hss_ek1[0], params.N, params.q_bit);
    RandomZZpx(hss_ek1[1], params.N, params.q_bit);
    hss_ek2[0] = sk[0] + hss_ek1[0];
    hss_ek2[1] = sk[1] + hss_ek1[1];

    // Alpha
    NTL::ZZ A;
    NTL::ZZ_pX alpha;
    A = NTL::RandomBits_ZZ(255);
    DecimalToBinary(alpha, A, 255);
    PvhssEnc(C_alpha, params, modulus, pk, alpha);
    bn_t A_bn;
    ep_t temp;
    bn_new(A_bn);
    ep_new(temp);
    ZZtoBn(A_bn, A);
    ep_mul_gen(temp, A_bn);
    pp_map_oatep_k12(pvhss_params.A_gT, temp, pvhss_params.g2_gen);
    bn_t g1_order;
    bn_t g2_order;
    bn_new(g1_order);
    bn_new(g2_order);
    ep_curve_get_ord(g1_order);
    ep2_curve_get_ord(g2_order);
    int size = bn_size_str(g1_order, 10);
    char *g1_order_str = new char[size];
    bn_write_str(g1_order_str, size, g1_order, 10);
    NTL::ZZ g1_order_ZZ = NTL::conv<NTL::ZZ>(g1_order_str);
    pvhss_params.g1_order_ZZ = g1_order_ZZ;
    delete[] g1_order_str;
    size = bn_size_str(g2_order, 10);
    char *g2_order_str = new char[size];
    bn_write_str(g2_order_str, size, g2_order, 10);
    NTL::ZZ g2_order_ZZ = NTL::conv<NTL::ZZ>(g2_order_str);
    pvhss_params.g2_order_ZZ = g2_order_ZZ;
    delete[] g2_order_str;

    // Compute e(g1, g2)^{q(2^N-1)}
    bn_t const_bn;
    bn_new(const_bn);
    NTL::ZZ temp1;
    temp1 = params.q * (NTL::power_ZZ(2, params.N) - 1);
    temp1 = temp1 % pvhss_params.g1_order_ZZ;
    ZZtoBn(const_bn, temp1);
    ep_mul_gen(temp, const_bn);
    pp_map_oatep_k12(pvhss_params.const_gT, temp, pvhss_params.g2_gen);
}

inline void PvhssEnc(NTL::vec_ZZ_pX &C, const PkeParams &params,
                     const NTL::ZZ_pXModulus &modulus, const NTL::vec_ZZ_pX &pk,
                     const NTL::ZZ_pX &x) {
    PkeOkdm(C, params, modulus, pk, x);
}

inline void PvhssMult(NTL::vec_ZZ_pX &db, const PkeParams &params,
                      const NTL::ZZ_pXModulus &modulus,
                      const NTL::vec_ZZ_pX &sk, const NTL::vec_ZZ_pX &C) {
    PkeDualDecrypt(db, params, modulus, sk, C);
}

inline void PvhssAddInPlace(NTL::vec_ZZ_pX &acc, const NTL::vec_ZZ_pX &x) {
    acc[0] += x[0];
    acc[1] += x[1];
}

// RMS-optimized recurrence: H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]
// Reduces PvhssMult calls from O(k*d^2) to O(k*d).
inline void PvhssEval(NTL::ZZ_pX &tby, ep_t g1T1, ep2_t g2T2, int b,
                      PvhssParams pvhss_params, const PkeParams &params,
                      const NTL::ZZ_pXModulus &modulus,
                      const NTL::vec_ZZ_pX &ek,
                      const NTL::vec_ZZ_pX &C_alpha,
                      const NTL::Vec<NTL::vec_ZZ_pX> &C_X,
                      int degree_f, const NTL::vec_ZZ_pX &M1) {
    int k = C_X.length();

    PvhssEvalWorkspace ws;
    InitPvhssEvalWorkspace(ws, degree_f);

    // Base case: M1 is the precomputed HSS share of the constant input 1.
    ws.dp_prev[0] = M1;
    for (int s = 1; s <= degree_f; s++) {
        ws.dp_prev[s].SetLength(2);
        ws.dp_prev[s][0] = 0;
        ws.dp_prev[s][1] = 0;
    }

    for (int i = 1; i <= k; i++) {
        // dp_curr[0] = constant 1 (inherited from dp_prev[0])
        ws.dp_curr[0] = ws.dp_prev[0];

        // H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]  for s = 1..degree_f
        for (int s = 1; s <= degree_f; s++) {
            ws.dp_curr[s].SetLength(2);
            ws.dp_curr[s][0] = 0;
            ws.dp_curr[s][1] = 0;
            // Start with H_{i-1}[s]
            PvhssAddInPlace(ws.dp_curr[s], ws.dp_prev[s]);
            // Add x_i * H_i[s-1]
            PvhssMult(ws.next, params, modulus, ws.dp_curr[s - 1], C_X[i - 1]);
            PvhssAddInPlace(ws.dp_curr[s], ws.next);
        }
        ws.dp_prev.swap(ws.dp_curr);
    }

    NTL::vec_ZZ_pX y_b_res;
    y_b_res.SetLength(2);
    y_b_res[0] = 0;
    y_b_res[1] = 0;
    for (int s = 1; s <= degree_f; s++) {
        PvhssAddInPlace(y_b_res, ws.dp_prev[s]);
    }
    tby = y_b_res[0];

    NTL::ZZX tb0;
    bn_t Tb_bn;
    bn_new(Tb_bn);

    NTL::ZZ Tb;
    PvhssMult(y_b_res, params, modulus, y_b_res, C_alpha);
    NTL::conv(tb0, y_b_res[0]);
    EvalZZX(Tb, tb0);
    if (b == 1) {
        Tb = Tb % pvhss_params.g1_order_ZZ;
        ZZtoBn(Tb_bn, Tb);
        ep_mul_gen(g1T1, Tb_bn);
    } else {
        Tb = Tb % pvhss_params.g2_order_ZZ;
        ZZtoBn(Tb_bn, Tb);
        ep2_mul_gen(g2T2, Tb_bn);
    }
}

inline bool PvhssVerify(const NTL::ZZ_pX &y1, const NTL::ZZ_pX &y2,
                        ep_t g1T1, ep2_t g2T2, PvhssParams pvhss_params) {
    fp12_t gtT1, gtT2, right_side, left_side;
    fp12_new(gtT1);
    fp12_new(gtT2);
    fp12_new(right_side);
    fp12_new(left_side);
    bn_t y_bn;
    bn_new(y_bn);
    NTL::ZZ y_zz;

    // Convert to ZZX first (modulus-independent), then clear ZZ_pX before
    // changing ZZ_p modulus to avoid destructor crashes.
    NTL::ZZX y_zzx;
    {
        NTL::ZZ_pX y_zz_px;
        y_zz_px = y2 - y1;
        NTL::conv(y_zzx, y_zz_px);
    }  // y_zz_px destroyed here with original modulus

    NTL::ZZ_p::init(pvhss_params.g1_order_ZZ);
    NTL::ZZ_p y_zz_p;
    NTL::ZZ_p two_zz_p(2);
    NTL::ZZ_pX y_zz_px2;
    NTL::conv(y_zz_px2, y_zzx);
    NTL::eval(y_zz_p, y_zz_px2, two_zz_p);

    NTL::conv(y_zz, y_zz_p);
    ZZtoBn(y_bn, y_zz);

    fp12_exp(right_side, pvhss_params.A_gT, y_bn);

    pp_map_oatep_k12(gtT1, g1T1, pvhss_params.g2_gen);
    pp_map_oatep_k12(gtT2, pvhss_params.g1_gen, g2T2);
    fp12_inv(gtT1, gtT1);
    fp12_mul(left_side, gtT2, gtT1);

    return fp12_cmp(left_side, right_side) == RLC_EQ;
}
