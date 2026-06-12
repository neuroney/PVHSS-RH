#pragma once

#include "PVHSS.h"
#include "GenData.h"
#include "helper.h"
#include <cstdio>
#include <cstdlib>

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
    ep_t g1T1;
    ep2_t g2T2;
    NTL::ZZ_pX t1y, t2y;
    NTL::vec_ZZ X_decimal;
    X_decimal.SetLength(num_data);
    for (int i = 0; i < num_data; ++i) {
        X_decimal[i] = NTL::RandomBits_ZZ(params.msg_bit);
    }
    timing = MeasureTimeMs([&]() {
        GenerateInputData(data, params, modulus, pk, X_decimal);
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    ep_new(g1T1);
    ep2_new(g2T2);

    NTL::vec_ZZ_pX C1, M1, M2;
    C1.SetLength(4);
    M1.SetLength(2);
    M2.SetLength(2);
    NTL::ZZ_pX M1_px;
    DecimalToBinary(M1_px, NTL::ZZ(1), 1);
    PvhssEnc(C1, params, modulus, pk, M1_px);
    PvhssMult(M1, params, modulus, hss_ek1, C1);
    PvhssMult(M2, params, modulus, hss_ek2, C1);

    // Evaluation Server 1
    timing = MeasureTimeMs([&]() {
        PvhssEval(t1y, g1T1, g2T2, 1, pvhss_params, params,
                  modulus, hss_ek1, C_alpha, data.C_X, params.d, M1);
    }, cyctimes);
    PrintTimeMs("Evaluation 0 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Server 2
    timing = MeasureTimeMs([&]() {
        PvhssEval(t2y, g1T1, g2T2, 2, pvhss_params, params,
                  modulus, hss_ek2, C_alpha, data.C_X, params.d, M2);
    }, cyctimes);
    PrintTimeMs("Evaluation 1 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification
    NTL::ZZ saved_mod = NTL::ZZ_p::modulus();
    bool acc = false;
    timing = MeasureTimeMsAdaptive([&]() {
        acc = PvhssVerify(t1y, t2y, g1T1, g2T2, pvhss_params);
        NTL::ZZ_p::init(saved_mod);
    }, cyctimes);
    PrintTimeMs("Verification algorithm time", timing);
    if (!acc) {
        std::printf("******************** ERROR ********************\n");
        std::exit(1);
    }
    std::cout << "-------------------------------------------------------" << std::endl;
}
