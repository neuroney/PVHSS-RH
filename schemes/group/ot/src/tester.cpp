#include "tester.h"

void PVHSSElg1_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for PVHSSElg1 time        " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    PVHSSElg1_Para param;
    PVHSSElg1_EK ek0, ek1;
    PVHSSElg1_SK sk;
    param.skLen = 1024;
    param.vkLen = 256;
    param.msg_bits = 32;
    param.degree_f = degree_f;
    param.msg_num = msg_num;
    bn_t ekp0, ekp1;

    PROOF pi0, pi1;

    TimingResult timing;

    // Setup Phase: VHSS base plus OT-specific public verification state.
    timing = MeasureTimeMs([&]() {
        VHSSElg_PK pk00;
        PVHSSElg1_EK ek000, ek100;
        VHSSElg_Gen(pk00, ek000, ek100, 1024, 256);
    }, cyctimes);
    PrintTimeMs("Setup base VHSS algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    PVHSSElg1_Para setup_param;
    PVHSSElg1_EK setup_ek0, setup_ek1;
    setup_param.skLen = 1024;
    setup_param.vkLen = 256;
    setup_param.msg_bits = 32;
    setup_param.degree_f = degree_f;
    setup_param.msg_num = msg_num;
    VHSSElg_Gen(setup_param.pk, setup_ek0, setup_ek1, setup_param.skLen, setup_param.vkLen);
    timing = MeasureTimeMs([&]() {
        Ped_ComGen(setup_param.ck);
        bn_t A;
        bn_new(A);
        ep2_new(setup_param.ck.g2_A);
        ZZ2bn(A, (setup_ek1[1] - setup_ek0[1]) % setup_param.pk.N);
        ep2_mul_gen(setup_param.ck.g2_A, A);
    }, cyctimes);
    PrintTimeMs("Setup incremental OT algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Key Generation Phase: OT-specific result-hiding material.
    PVHSSElg1_Para keygen_param;
    PVHSSElg1_EK keygen_ek0, keygen_ek1;
    keygen_param.skLen = 1024;
    keygen_param.vkLen = 256;
    keygen_param.msg_bits = 32;
    keygen_param.degree_f = degree_f;
    keygen_param.msg_num = msg_num;
    PVHSSElg1_Setup(keygen_param, keygen_ek0, keygen_ek1);
    timing = MeasureTimeMs([&]() {
        PVHSSElg1_SK sk000;
        bn_t ekp000, ekp100;
        PVHSSElg1_KeyGen(keygen_param, sk000, ekp000, ekp100);
    }, cyctimes);

    PrintTimeMs("KeyGen incremental OT algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    PVHSSElg1_Setup(param, ek0, ek1);
    PVHSSElg1_KeyGen(param, sk, ekp0, ekp1);

    // Input Generation Phase
    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.msg_bits);
    }

    // Input Processing Phase
    vector<PVHSSElg1_CT> Ix;
    timing = MeasureTimeMs([&]() {
        PVHSSElg1_ProbGen(Ix, param, X);
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Polynomial Generation Phase
    vector<vector<int>> F_TEST;
    Random_Func(F_TEST, msg_num, degree_f);

    // Evaluation Phase for Server 0: VHSS base computation plus OT result-hiding proof.
    VHSSElg_MV y0_base, y1_base;
    int prf_key0 = 0;
    int prf_key1 = 0;
    timing = MeasureTimeMs([&]() {
        prf_key0 = 0;
        VHSSElg_Evaluate_P_d2(y0_base, 0, Ix, param.pk, ek0, prf_key0, param.degree_f);
    }, cyctimes);
    PrintTimeMs("Evaluation base 0 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    timing = MeasureTimeMs([&]() {
        VHSSElg_MV y_tmp = y0_base;
        VHSSElg_MV sk_b;
        int proof_prf_key = prf_key0;
        VHSSElg_ConvertInput(sk_b, 0, param.pk, ek0, param.pk_f, proof_prf_key);
        y_tmp[0] = y_tmp[0] + sk_b[0];
        y_tmp[2] = y_tmp[2] + sk_b[2];
        Ped_Prove(pi0, 0, y_tmp[0], y_tmp[2], param.ck, proof_prf_key, ekp0);
    }, cyctimes);
    PrintTimeMs("Evaluation incremental 0 OT algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Phase for Server 1.
    timing = MeasureTimeMs([&]() {
        prf_key1 = 0;
        VHSSElg_Evaluate_P_d2(y1_base, 1, Ix, param.pk, ek1, prf_key1, param.degree_f);
    }, 1);
    PrintTimeMs("Evaluation base 1 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    timing = MeasureTimeMs([&]() {
        VHSSElg_MV y_tmp = y1_base;
        VHSSElg_MV sk_b;
        int proof_prf_key = prf_key1;
        VHSSElg_ConvertInput(sk_b, 1, param.pk, ek1, param.pk_f, proof_prf_key);
        y_tmp[0] = y_tmp[0] + sk_b[0];
        y_tmp[2] = y_tmp[2] + sk_b[2];
        Ped_Prove(pi1, 1, y_tmp[0], y_tmp[2], param.ck, proof_prf_key, ekp1);
    }, 1);
    PrintTimeMs("Evaluation incremental 1 OT algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification Phase
    bool acc;
    timing = MeasureTimeMsAdaptive([&]() {
        acc = PVHSSElg1_Verify(pi0, pi1, param.ck);
    }, cyctimes);
    PrintTimeMs("Verification algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // if (acc)
    // {
    //     cout << "Verification Passed\n";
    // }
    // else
    // {
    //     cout << "Verification Failed\n";
    // }

    // Decryption Phase
    ZZ y;
    timing = MeasureTimeMsAdaptive([&]() {
        PVHSSElg1_Decode(y, pi0, pi1, sk);
    }, cyctimes);
    PrintTimeMs("Decryption algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;
}
