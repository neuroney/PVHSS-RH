#pragma once

#include "PVHSS.h"
#include "GenData.h"
#include "helper.h"
#include <algorithm>
#include <stdlib.h>

inline void EvalPoly(int deg, int num_data, int cyctimes) {
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for PVHSS_CZ           " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << deg << "        msg_num: " << num_data << "        cyctimes: " << cyctimes << std::endl;

    PkeParams params;
    PvhssParams pvhss_params;
    NTL::vec_ZZ_pX pk, sk, hss_ek1, hss_ek2, C_alpha;
    NTL::ZZ_pXModulus modulus;
    pk.SetLength(2);
    sk.SetLength(2);
    hss_ek1.SetLength(2);
    hss_ek2.SetLength(2);
    C_alpha.SetLength(4);
    params.msg_bit = 32;
    params.d = deg;
    params.num_data = num_data;

    TimingResult timing = MeasureTimeMs([&]() {
        PkeGen(params, pk, sk, 1);
        NTL::build(modulus, params.xN);
        PvhssGen(hss_ek1, hss_ek2, C_alpha, pvhss_params, params, modulus, pk, sk);
    }, cyctimes);
    PrintTimeMs("Setup algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Data generation
    EncodedData data;
    NTL::ZZ T1, T2;
    ep_t g1T1;
    ep2_t g2T2;
    NTL::ZZ_pX t1y, t2y;
    timing = MeasureTimeMs([&]() {
        GenerateData(data, params, modulus, pk);
        ep_new(g1T1);
        ep2_new(g2T2);
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Server 1
    timing = MeasureTimeMs([&]() {
        PvhssEval(t1y, g1T1, g2T2, 1, pvhss_params, params,
                  modulus, hss_ek1, C_alpha, data.C_X, data.PRF,
                  data.F_TEST, params.d, data.C_1);
    }, cyctimes);
    PrintTimeMs("Evaluation 0 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Server 2
    timing = MeasureTimeMs([&]() {
        PvhssEval(t2y, g1T1, g2T2, 2, pvhss_params, params,
                  modulus, hss_ek2, C_alpha, data.C_X, data.PRF,
                  data.F_TEST, params.d, data.C_1);
    }, cyctimes);
    PrintTimeMs("Evaluation 1 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification
    NTL::ZZ saved_mod = NTL::ZZ_p::modulus();
    timing = MeasureTimeMsAdaptive([&]() {
        PvhssVerify(t1y, t2y, g1T1, g2T2, pvhss_params);
        NTL::ZZ_p::init(saved_mod);
    }, cyctimes);
    PrintTimeMs("Verification algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Decoding
    NTL::ZZ res;
    timing = MeasureTimeMsAdaptive([&]() {
        PvhssDecode(res, pvhss_params, params, t1y, t2y);
        NTL::ZZ_p::init(saved_mod);
    }, cyctimes);
    PrintTimeMs("Decoding algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;
}

inline void RunGen(int msg_bit) {
    PkeParams params;
    PvhssParams pvhss_params;
    NTL::vec_ZZ_pX pk, sk, hss_ek1, hss_ek2, C_alpha;
    pk.SetLength(2);
    sk.SetLength(2);
    hss_ek1.SetLength(2);
    hss_ek2.SetLength(2);
    C_alpha.SetLength(4);
    params.msg_bit = msg_bit;
    params.num_data = 2;
    PkeGen(params, pk, sk, 0);
    NTL::ZZ_pXModulus modulus(params.xN);
    PvhssGen(hss_ek1, hss_ek2, C_alpha, pvhss_params, params, modulus, pk, sk);
}

inline void TimeGen(int msg_bit, int cyctimes) {
    TimingResult timing = MeasureTimeMs([&]() {
        RunGen(msg_bit);
    }, cyctimes);
    PrintTimeMs("Gen algo time", timing);
}

inline void TimeEnc(int msg_bit, int cyctimes) {
    PkeParams params;
    PvhssParams pvhss_params;
    NTL::vec_ZZ_pX pk, sk, hss_ek1, hss_ek2, C_alpha;
    pk.SetLength(2);
    sk.SetLength(2);
    hss_ek1.SetLength(2);
    hss_ek2.SetLength(2);
    C_alpha.SetLength(4);
    params.msg_bit = msg_bit;
    params.num_data = 2;
    PkeGen(params, pk, sk, 0);
    NTL::ZZ_pXModulus modulus(params.xN);
    PvhssGen(hss_ek1, hss_ek2, C_alpha, pvhss_params, params, modulus, pk, sk);

    NTL::vec_ZZ_pX C;
    NTL::ZZ_pX x;
    C.SetLength(4);
    TimingResult timing = MeasureTimeMs([&]() {
        RandomZZpx(x, params.msg_bit, 1);
        PvhssEnc(C, params, modulus, pk, x);
    }, cyctimes);
    PrintTimeMs("Enc algo time", timing);
}

inline void TimeVer(int msg_bit, int cyctimes) {
    PkeParams params;
    PvhssParams pvhss_params;
    NTL::vec_ZZ_pX pk, sk, hss_ek1, hss_ek2, C_alpha;
    pk.SetLength(2);
    sk.SetLength(2);
    hss_ek1.SetLength(2);
    hss_ek2.SetLength(2);
    C_alpha.SetLength(4);
    params.msg_bit = msg_bit;
    params.num_data = 2;
    PkeGen(params, pk, sk, 0);
    NTL::ZZ_pXModulus modulus(params.xN);
    PvhssGen(hss_ek1, hss_ek2, C_alpha, pvhss_params, params, modulus, pk, sk);

    NTL::ZZ_pX y_zz_px, y1_zz_px, y2_zz_px;
    NTL::ZZ_p y_zz_p, y1, y2, y;
    NTL::ZZ y_zz;
    ep_t g1T1;
    ep2_t g2T2;
    ep_new(g1T1);
    ep2_new(g2T2);
    RandomZZpx(y_zz_px, params.N, params.msg_bit);
    RandomZZpx(y1_zz_px, params.N, params.q_bit);
    y2_zz_px = y_zz_px - y1_zz_px;

    TimingResult timing = MeasureTimeMsAdaptive([&]() {
        fp12_t gtT1, gtT2, right_side, left_side;
        fp12_new(gtT1);
        fp12_new(gtT2);
        fp12_new(right_side);
        fp12_new(left_side);
        bn_t y_bn;
        bn_new(y_bn);
        NTL::ZZX y_zzx;
        NTL::ZZ_pX y_sum = y1_zz_px + y2_zz_px;
        NTL::conv(y_zzx, y_sum);
        NTL::ZZ_p::init(pvhss_params.g1_order_ZZ);

        NTL::ZZ_p two_zz_p(2);
        bn_t Tb_bn;
        bn_new(Tb_bn);
        NTL::conv(y_zz_px, y_zzx);
        NTL::eval(y_zz_p, y_zz_px, two_zz_p);

        NTL::conv(y_zz, y_zz_p);
        ZZtoBn(y_bn, y_zz);

        fp12_exp(right_side, pvhss_params.A_gT, y_bn);
        fp12_mul(right_side, right_side, pvhss_params.const_gT);

        pp_map_oatep_k12(gtT1, g1T1, pvhss_params.g2_gen);
        pp_map_oatep_k12(gtT2, pvhss_params.g1_gen, g2T2);
        fp12_mul(left_side, gtT1, gtT2);
    }, cyctimes);
    PrintTimeMs("Ver algo time", timing);
}

inline void TimeEvalSubalgo(int msg_bit, int cyctimes) {
    PkeParams params;
    PvhssParams pvhss_params;
    NTL::vec_ZZ_pX pk, sk, hss_ek1, hss_ek2, C_alpha;
    pk.SetLength(2);
    sk.SetLength(2);
    hss_ek1.SetLength(2);
    hss_ek2.SetLength(2);
    C_alpha.SetLength(4);
    params.msg_bit = msg_bit;
    params.num_data = 2;
    PkeGen(params, pk, sk, 0);
    NTL::ZZ_pXModulus modulus(params.xN);
    PvhssGen(hss_ek1, hss_ek2, C_alpha, pvhss_params, params, modulus, pk, sk);

    EncodedData data;
    GenerateData(data, params, modulus, pk);

    // Load benchmark
    NTL::vec_ZZ_pX db1, db2;
    db1.SetLength(2);
    db2.SetLength(2);
    TimingResult timing = MeasureTimeMs([&]() {
        int index = rand() % 10;
        PvhssMult(db1, params, modulus, hss_ek1, data.C_X[rand() % params.num_data]);
        db1[0] = db1[0] + data.PRF[index][0];
        db1[1] = db1[1] + data.PRF[index][1];
    }, cyctimes);
    PrintTimeMs("Load algo time", timing);

    // Add1 benchmark
    RandomZZpx(db1[0], params.N, params.q_bit);
    RandomZZpx(db1[1], params.N, params.q_bit);
    RandomZZpx(db2[0], params.N, params.q_bit);
    RandomZZpx(db2[1], params.N, params.q_bit);
    int index = rand() % 10;
    NTL::vec_ZZ_pX add_tmp;
    add_tmp.SetLength(2);
    timing = MeasureTimeMsAdaptive([&]() {
        add_tmp[0] = db1[0] + db2[0] + data.PRF[index][0];
        add_tmp[1] = db1[1] + db2[1] + data.PRF[index][1];
    }, cyctimes);
    PrintTimeMs("Add1 algo time", timing);

    // Add2 benchmark
    NTL::vec_ZZ_pX C;
    C.SetLength(4);
    timing = MeasureTimeMsAdaptive([&]() {
        C[0] = data.C_X[0][0] + data.C_X[1][0];
        C[1] = data.C_X[0][1] + data.C_X[1][1];
        C[2] = data.C_X[0][2] + data.C_X[1][2];
        C[3] = data.C_X[0][3] + data.C_X[1][3];
    }, cyctimes);
    PrintTimeMs("Add2 algo time", timing);

    // cmult
    NTL::ZZ constant = NTL::RandomBits_ZZ(msg_bit);
    PvhssMult(db1, params, modulus, hss_ek1, data.C_X[rand() % params.num_data]);
    index = rand() % 10;
    NTL::vec_ZZ_pX cmult_tmp;
    cmult_tmp.SetLength(2);
    timing = MeasureTimeMsAdaptive([&]() {
        ZZpxScaleMul(cmult_tmp[0], db1[0], constant);
        ZZpxScaleMul(cmult_tmp[1], db1[1], constant);
        cmult_tmp[0] = cmult_tmp[0] + data.PRF[index][0];
        cmult_tmp[1] = cmult_tmp[1] + data.PRF[index][1];
    }, cyctimes);
    PrintTimeMs("cMult algo time", timing);

    // mult
    timing = MeasureTimeMs([&]() {
        PvhssMult(db1, params, modulus, hss_ek1, data.C_X[rand() % params.num_data]);
        int index2 = rand() % 10;
        PvhssMult(db1, params, modulus, db1, data.C_X[rand() % params.num_data]);
        db1[0] = db1[0] + data.PRF[index2][0];
        db1[1] = db1[1] + data.PRF[index2][1];
    }, cyctimes);
    PrintTimeMs("Mult algo time", timing);

    // output
    int b = 1;
    ep_t g1T1;
    ep2_t g2T2;
    ep_new(g1T1);
    ep2_new(g2T2);
    NTL::ZZ_p yb;
    NTL::vec_ZZ_pX tb;
    tb.SetLength(2);
    PvhssMult(tb, params, modulus, hss_ek1, data.C_X[rand() % params.num_data]);

    timing = MeasureTimeMs([&]() {
        NTL::ZZX tb0;
        NTL::ZZ Tb;
        NTL::ZZ_p two_zz_p(2);
        bn_t Tb_bn;
        bn_new(Tb_bn);
        NTL::eval(yb, tb[0], two_zz_p);
        PvhssMult(tb, params, modulus, tb, C_alpha);
        NTL::conv(tb0, tb[0]);
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
    }, cyctimes);
    PrintTimeMs("Output algo time", timing);
}
