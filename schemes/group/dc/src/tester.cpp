#include "tester.h"

using namespace NTL;
using namespace std;

void PVHSSElg2_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for PVHSSElg2_time        " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    PVHSSElg2_Para param;
    PVHSSElg2_EK ek0, ek1;
    PVHSSElg2_SK sk;
    bn_t ekp0[2], ekp1[2];
    param.skLen = 1024;
    param.vkLen = 256;
    param.msg_bits = 32;
    param.degree_f = degree_f;
    param.msg_num = msg_num;

    PROOF pi0, pi1;

    TimingResult timing;

    // Setup Phase: VHSS base plus DC-specific decryptable commitment state.
    timing = MeasureTimeMs([&]() {
        VhssElgamalPk pk00;
        PVHSSElg2_EK ek000, ek100;
        VhssElgamalGen(pk00, ek000, ek100, 1024, 256);
    }, cyctimes);
    PrintTimeMs("Setup base VHSS algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    PVHSSElg2_Para setup_param;
    PVHSSElg2_EK setup_ek0, setup_ek1;
    PVHSSElg2_SK setup_sk;
    setup_param.skLen = 1024;
    setup_param.vkLen = 256;
    setup_param.msg_bits = 32;
    setup_param.degree_f = degree_f;
    setup_param.msg_num = msg_num;
    VhssElgamalGen(setup_param.pk, setup_ek0, setup_ek1, setup_param.skLen, setup_param.vkLen);
    timing = MeasureTimeMs([&]() {
        DecPed_ComGen(setup_param.ck, setup_sk);
        bn_t A;
        bn_new(A);
        ep2_new(setup_param.vk);
        ZZtoBn(A, (setup_ek1[1] - setup_ek0[1]) % setup_param.pk.N);
        ep2_mul_gen(setup_param.vk, A);
    }, cyctimes);
    PrintTimeMs("Setup incremental DC algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Gen phase: DC-specific result-hiding material.
    PVHSSElg2_Para keygen_param;
    PVHSSElg2_EK keygen_ek0, keygen_ek1;
    PVHSSElg2_SK keygen_sk;
    keygen_param.skLen = 1024;
    keygen_param.vkLen = 256;
    keygen_param.msg_bits = 32;
    keygen_param.degree_f = degree_f;
    keygen_param.msg_num = msg_num;
    PVHSSElg2_Setup(keygen_param, keygen_ek0, keygen_ek1, keygen_sk);
    timing = MeasureTimeMs([&]() {
        bn_t ekp000[2], ekp100[2];
        PVHSSElg2_KeyGen(keygen_param, keygen_sk, ekp000, ekp100);
    }, cyctimes);
    PrintTimeMs("Gen incremental DC algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    PVHSSElg2_Setup(param, ek0, ek1, sk);
    PVHSSElg2_KeyGen(param, sk, ekp0, ekp1);

    // Input Generation Phase
    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        NTL::RandomBits(X[i], param.msg_bits);
    }

    // Input Processing Phase
    vector<PVHSSElg2_CT> Ix;
    timing = MeasureTimeMs([&]() {
        PVHSSElg2_ProbGen(Ix, param, X);
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Polynomial Generation Phase
    vector<vector<int>> F_TEST;
    GenerateRandomFunc(F_TEST, msg_num, degree_f);

    // Evaluation Phase for Server 0: VHSS base computation plus DC proof.
    VhssElgamalMv y0_base, y1_base;
    int prf_key0 = 0;
    int prf_key1 = 0;
    timing = MeasureTimeMs([&]() {
        prf_key0 = 0;
        VhssElgamalEvaluatePD2(y0_base, 0, Ix, param.pk, ek0, prf_key0, param.degree_f);
    }, cyctimes);
    PrintTimeMs("Evaluation base 0 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    timing = MeasureTimeMs([&]() {
        PVHSSElg2_Prove(pi0, 0, y0_base[0], y0_base[2], param, ekp0);
    }, cyctimes);
    PrintTimeMs("Evaluation incremental 0 DC algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Phase for Server 1.
    timing = MeasureTimeMs([&]() {
        prf_key1 = 0;
        VhssElgamalEvaluatePD2(y1_base, 1, Ix, param.pk, ek1, prf_key1, param.degree_f);
    }, 1);
    PrintTimeMs("Evaluation base 1 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    timing = MeasureTimeMs([&]() {
        PVHSSElg2_Prove(pi1, 1, y1_base[0], y1_base[2], param, ekp1);
    }, 1);
    PrintTimeMs("Evaluation incremental 1 DC algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification Phase
    bool acc;
    timing = MeasureTimeMsAdaptive([&]() {
        acc = PVHSSElg2_Verify(pi0, pi1, param);
    }, cyctimes);
    PrintTimeMs("Verification algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // else
    // {
    //     cout << "Verification Failed\n";
    // }

    // Decryption Phase
    dig_t y;
    timing = MeasureTimeMsAdaptive([&]() {
        PVHSSElg2_Decode(y, pi0, pi1, sk);
    }, cyctimes);
    PrintTimeMs("Decryption algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;
}
