#pragma once

#include <gmp.h>

extern "C" {
#include <relic/relic.h>
}

#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <NTL/ZZ_pX.h>
#include "PKE.h"

// PVHSS pairing parameters.
struct PvhssParams {
    NTL::ZZ g1_order_ZZ;
    NTL::ZZ g2_order_ZZ;
    NTL::ZZ_pX alpha;
    bn_t g1_order;
    bn_t g2_order;
    ep_t g1_gen;
    ep2_t g2_gen;
    fp12_t gT_gen;
    fp12_t A_gT;
    fp12_t const_gT;
};

// Workspace for Eval DP to avoid repeated allocations and copies.
struct PvhssEvalWorkspace {
    NTL::Mat<NTL::ZZ_pX> dp_prev;
    NTL::Mat<NTL::ZZ_pX> dp_curr;
    NTL::vec_ZZ_pX chain;
    NTL::vec_ZZ_pX next;
    NTL::vec_ZZ_pX acc;
    PkeWorkspace pke_ws;
};

inline void InitPvhssEvalWorkspace(PvhssEvalWorkspace &ws, int degree_f) {
    ws.dp_prev.SetDims(degree_f + 1, 2);
    ws.dp_curr.SetDims(degree_f + 1, 2);
    ws.chain.SetLength(2);
    ws.next.SetLength(2);
    ws.acc.SetLength(2);
}

// Forward declaration.
void PvhssEnc(NTL::vec_ZZ_pX &C, const PkeParams &params,
              const NTL::ZZ_pXModulus &modulus, const NTL::vec_ZZ_pX &pk,
              const NTL::ZZ_pX &x);

inline void PvhssGen(NTL::vec_ZZ_pX &hss_ek1, NTL::vec_ZZ_pX &hss_ek2,
                     NTL::vec_ZZ_pX &C_alpha, PvhssParams &pvhss_params,
                     const PkeParams &params, const NTL::ZZ_pXModulus &modulus,
                     const NTL::vec_ZZ_pX &pk, const NTL::vec_ZZ_pX &sk) {
    bn_new(pvhss_params.g1_order);
    bn_new(pvhss_params.g2_order);
    ep_new(pvhss_params.g1_gen);
    ep2_new(pvhss_params.g2_gen);
    fp12_new(pvhss_params.gT_gen);
    fp12_new(pvhss_params.A_gT);
    fp12_new(pvhss_params.const_gT);
    core_init();
    ep_curve_init();
    ep_param_set(B12_P381);
    ep2_curve_init();
    ep2_curve_set_twist(RLC_EP_MTYPE);
    ep_curve_get_gen(pvhss_params.g1_gen);
    ep2_curve_get_gen(pvhss_params.g2_gen);
    pp_map_oatep_k12(pvhss_params.gT_gen, pvhss_params.g1_gen, pvhss_params.g2_gen);

    // Evaluation keys
    RandomZZpx(hss_ek1[0], params.N, params.q_bit);
    RandomZZpx(hss_ek1[1], params.N, params.q_bit);
    hss_ek2[0] = sk[0] - hss_ek1[0];
    hss_ek2[1] = sk[1] - hss_ek1[1];

    // Alpha
    NTL::ZZ A;
    NTL::ZZ_pX alpha;
    A = NTL::RandomBits_ZZ(255);
    DecimalToBinary(alpha, A, 255);
    PvhssEnc(C_alpha, params, modulus, pk, alpha);
    pvhss_params.alpha = alpha;
    bn_t A_bn;
    ep_t temp;
    bn_new(A_bn);
    ep_new(temp);
    ZZtoBn(A_bn, A);
    ep_mul_gen(temp, A_bn);
    pp_map_oatep_k12(pvhss_params.A_gT, temp, pvhss_params.g2_gen);
    ep_curve_get_ord(pvhss_params.g1_order);
    ep2_curve_get_ord(pvhss_params.g2_order);
    int size = bn_size_str(pvhss_params.g1_order, 10);
    char *g1_order_str = new char[size];
    bn_write_str(g1_order_str, size, pvhss_params.g1_order, 10);
    NTL::ZZ g1_order_ZZ = NTL::conv<NTL::ZZ>(g1_order_str);
    pvhss_params.g1_order_ZZ = g1_order_ZZ;
    size = bn_size_str(pvhss_params.g2_order, 10);
    char *g2_order_str = new char[size];
    bn_write_str(g2_order_str, size, pvhss_params.g2_order, 10);
    NTL::ZZ g2_order_ZZ = NTL::conv<NTL::ZZ>(g2_order_str);
    pvhss_params.g2_order_ZZ = g2_order_ZZ;

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

// Global PRF counter (legacy).
static int prf_key = 1;

// Recursively evaluate all degree-d monomials over the encrypted data.
inline void EvaluateRecursive(NTL::vec_ZZ_pX &tb, int b, int d, int num_data,
                              int loop, int beg_ind, int *ind_var,
                              const PkeParams &params,
                              const NTL::ZZ_pXModulus &modulus,
                              const NTL::vec_ZZ_pX &ek,
                              const NTL::Vec<NTL::vec_ZZ_pX> &C_X,
                              const NTL::Vec<NTL::vec_ZZ_pX> &PRF) {
    if (loop == d) {
        NTL::vec_ZZ_pX tb_temp;
        tb_temp.SetLength(2);
        if (d == 1) {
            prf_key = (prf_key + 1) % 10;
            PvhssMult(tb_temp, params, modulus, ek, C_X[ind_var[0]]);
            if (b == 1) {
                tb_temp[0] = tb_temp[0] + PRF[prf_key][0];
                tb_temp[1] = tb_temp[1] + PRF[prf_key][1];
            } else {
                tb_temp[0] = tb_temp[0] - PRF[prf_key][0];
                tb_temp[1] = tb_temp[1] - PRF[prf_key][1];
            }
            tb[0] = tb[0] + tb_temp[0];
            tb[1] = tb[1] + tb_temp[1];
        } else {
            PvhssMult(tb_temp, params, modulus, ek, C_X[ind_var[0]]);
            prf_key = (prf_key + 1) % 10;
            if (b == 1) {
                tb_temp[0] = tb_temp[0] + PRF[prf_key][0];
                tb_temp[1] = tb_temp[1] + PRF[prf_key][1];
            } else {
                tb_temp[0] = tb_temp[0] - PRF[prf_key][0];
                tb_temp[1] = tb_temp[1] - PRF[prf_key][1];
            }
            for (int i = 1; i < d; i++) {
                PvhssMult(tb_temp, params, modulus, tb_temp, C_X[ind_var[i]]);
                prf_key = (prf_key + 1) % 10;
                if (b == 1) {
                    tb_temp[0] = tb_temp[0] + PRF[prf_key][0];
                    tb_temp[1] = tb_temp[1] + PRF[prf_key][1];
                } else {
                    tb_temp[0] = tb_temp[0] - PRF[prf_key][0];
                    tb_temp[1] = tb_temp[1] - PRF[prf_key][1];
                }
            }
            tb[0] = tb[0] + tb_temp[0];
            tb[1] = tb[1] + tb_temp[1];
        }
    } else {
        int next_loop = loop + 1;
        for (int i = beg_ind; i < num_data; i++) {
            ind_var[next_loop - 1] = i;
            EvaluateRecursive(tb, b, d, num_data, next_loop, i, ind_var,
                              params, modulus, ek, C_X, PRF);
        }
    }
}

inline void PvhssAdd(NTL::vec_ZZ_pX &tb, NTL::vec_ZZ_pX &tb1, NTL::vec_ZZ_pX &tb2) {
    tb.SetLength(2);
    tb[0] = tb1[0] + tb2[0];
    tb[1] = tb1[1] + tb2[1];
}

inline void PvhssAddInPlace(NTL::vec_ZZ_pX &acc, const NTL::vec_ZZ_pX &x) {
    acc[0] += x[0];
    acc[1] += x[1];
}

// Optimized chain-reuse DP: O(k*d^2) PvhssMult calls instead of O(k*d^3).
inline void PvhssEval(NTL::ZZ_pX &tby, ep_t g1T1, ep2_t g2T2, int b,
                      PvhssParams pvhss_params, const PkeParams &params,
                      const NTL::ZZ_pXModulus &modulus,
                      const NTL::vec_ZZ_pX &ek,
                      const NTL::vec_ZZ_pX &C_alpha,
                      const NTL::Vec<NTL::vec_ZZ_pX> &C_X,
                      const NTL::Vec<NTL::vec_ZZ_pX> &PRF,
                      const std::vector<std::vector<int>> &F_TEST,
                      int degree_f, const NTL::vec_ZZ_pX &C_1) {
    int k = C_X.length();

    PvhssEvalWorkspace ws;
    InitPvhssEvalWorkspace(ws, degree_f);

    // Base case: dp_prev[0] = M1 = decrypt(C_1)
    PvhssMult(ws.dp_prev[0], params, modulus, ek, C_1);
    for (int s = 1; s <= degree_f; s++) {
        ws.dp_prev[s].SetLength(2);
        ws.dp_prev[s][0] = 0;
        ws.dp_prev[s][1] = 0;
    }

    for (int i = 1; i <= k; i++) {
        for (int s = 0; s <= degree_f; s++) {
            ws.dp_curr[s].SetLength(2);
            ws.dp_curr[s][0] = 0;
            ws.dp_curr[s][1] = 0;
            PvhssAddInPlace(ws.dp_curr[s], ws.dp_prev[s]);
        }

        for (int r = 0; r < degree_f; r++) {
            ws.chain[0] = ws.dp_prev[r][0];
            ws.chain[1] = ws.dp_prev[r][1];

            for (int j = 1; r + j <= degree_f; j++) {
                PvhssMult(ws.next, params, modulus, ws.chain, C_X[i - 1]);
                ws.chain.swap(ws.next);
                PvhssAddInPlace(ws.dp_curr[r + j], ws.chain);
            }
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

inline void PvhssDecode(NTL::ZZ &y, PvhssParams pvhss_params,
                        const PkeParams params, const NTL::ZZ_pX &y1,
                        const NTL::ZZ_pX &y2) {
    NTL::ZZ_p::init(pvhss_params.g1_order_ZZ);
    NTL::ZZ_pXModulus modulus(params.xN);
    NTL::ZZ_p y_zz_p;
    NTL::ZZX y_zzx;
    NTL::ZZ_pX y_zz_px;
    NTL::add(y_zz_px, y1, y2);
    NTL::conv(y_zzx, y_zz_px);
    NTL::ZZ_p two_zz_p(2);
    NTL::conv(y_zz_px, y_zzx);
    NTL::eval(y_zz_p, y_zz_px, two_zz_p);
    NTL::conv(y, y_zz_p);
}

inline void PvhssVerify(const NTL::ZZ_pX &y1, const NTL::ZZ_pX &y2,
                        ep_t g1T1, ep2_t g2T2, PvhssParams pvhss_params) {
    fp12_t gtT1, gtT2, right_side, left_side;
    fp12_new(gtT1);
    fp12_new(gtT2);
    fp12_new(right_side);
    fp12_new(left_side);
    bn_t y_bn;
    bn_new(y_bn);
    NTL::ZZ y_zz;
    NTL::ZZ_p y_zz_p;
    NTL::ZZX y_zzx;
    NTL::ZZ_pX y_zz_px;

    y_zz_px = y1 + y2;
    NTL::conv(y_zzx, y_zz_px);

    NTL::ZZ_p::init(pvhss_params.g1_order_ZZ);
    NTL::ZZ_p two_zz_p(2);
    NTL::conv(y_zz_px, y_zzx);
    NTL::eval(y_zz_p, y_zz_px, two_zz_p);

    NTL::conv(y_zz, y_zz_p);
    ZZtoBn(y_bn, y_zz);

    fp12_exp(right_side, pvhss_params.A_gT, y_bn);
    fp12_mul(right_side, right_side, pvhss_params.const_gT);

    pp_map_oatep_k12(gtT1, g1T1, pvhss_params.g2_gen);
    pp_map_oatep_k12(gtT2, pvhss_params.g1_gen, g2T2);
    fp12_mul(left_side, gtT1, gtT2);

    std::cout << "The value of y= " << y_zz << "\n";
    if (fp12_cmp(left_side, right_side) == RLC_EQ) {
        std::cout << "Verification passed!\n";
    } else {
        std::printf("******************** ERROR ********************\n");
        std::exit(1);
    }
}