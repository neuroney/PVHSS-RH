#include "tester.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace rlwe { namespace ot {

void PVHSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for PVHSSRLWE_OT       " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    PVHSSPara param;
    vec_ZZ_pX pkePk;
    PVHSS_SK sk;

    PROOF pi0, pi1;
    bn_t ekp0, ekp1;

    TimingResult timing;

    // Setup Phase: VHSS base plus OT-specific public verification state.
    timing = MeasureTimeMs([&]() {
        PVHSSPara param00;
        vec_ZZ_pX pkePk00;
        vec_ZZ_pX pkeSk00;
        pkePk00.SetLength(2);
        pkeSk00.SetLength(2);
        param00.pkePara.msg_bit = 32;
        param00.pkePara.num_data = msg_num;
        param00.pkePara.d = degree_f;
        PKE_Gen(param00.pkePara, pkePk00, pkeSk00);
        ZZ_pXModulus modulus00(param00.pkePara.xN);
        VHSS_Gen(param00.vhssPara, param00.pkePara, modulus00, pkeSk00);
    }, cyctimes);
    PrintTimeMs("Setup base VHSS algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    PVHSSPara setup_param;
    vec_ZZ_pX setup_pkePk, setup_pkeSk;
    setup_pkePk.SetLength(2);
    setup_pkeSk.SetLength(2);
    setup_param.pkePara.msg_bit = 32;
    setup_param.pkePara.num_data = msg_num;
    setup_param.pkePara.d = degree_f;
    PKE_Gen(setup_param.pkePara, setup_pkePk, setup_pkeSk);
    ZZ_pXModulus setup_modulus(setup_param.pkePara.xN);
    VHSS_Gen(setup_param.vhssPara, setup_param.pkePara, setup_modulus, setup_pkeSk);
    timing = MeasureTimeMs([&]() {
        Ped_ComGen(setup_param.ck);
        NTL::RandomBits(setup_param.sk_alpha, 32);
        ZZ A_ZZ = HssOutputPolyAtTwo(setup_param.vhssPara.alpha, setup_param.pkePara, setup_param.ck.g1_order_ZZ);
        bn_t A;
        bn_new(A);
        ep2_new(setup_param.ck.g2_A);
        ZZtoBn(A, A_ZZ);
        ep2_mul_gen(setup_param.ck.g2_A, A);
    }, cyctimes);
    PrintTimeMs("Setup incremental OT algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Key Generation Phase: OT-specific result-hiding material.
    PVHSSPara keygen_param;
    vec_ZZ_pX keygen_pkePk;
    Setup(keygen_param, keygen_pkePk, msg_num, degree_f);
    ZZ_pXModulus keygen_modulus(keygen_param.pkePara.xN);
    timing = MeasureTimeMs([&]() {
        PVHSS_SK sk00;
        bn_t ekp000, ekp100;
        KeyGen(keygen_param, sk00, keygen_modulus, keygen_pkePk, ekp000, ekp100);
    }, cyctimes);
    PrintTimeMs("KeyGen incremental OT algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;


    Setup(param, pkePk, msg_num, degree_f);
    ZZ_pXModulus modulus(param.pkePara.xN);
    KeyGen(param, sk, modulus, pkePk, ekp0, ekp1);

    // Input Generation Phase
    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        NTL::RandomBits(X[i], param.pkePara.msg_bit);
    }

    // Input Processing Phase
    vector<PVHSS_CT> Ix;
    timing = MeasureTimeMs([&]() {
        ProbGen(Ix, param, X, modulus, pkePk);
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    PVHSS_CT C1;
    PVHSS_MV M1, M2, M3, M4;
    VHSS_Enc(C1, param.pkePara, modulus, pkePk, ZZ(1));
    HssConvertInput(M1, param.pkePara, modulus, param.vhssPara.vhssEk_1, C1);
    HssConvertInput(M2, param.pkePara, modulus, param.vhssPara.vhssEk_2, C1);
    HssConvertInput(M3, param.pkePara, modulus, param.vhssPara.vhssEk_3, C1);
    HssConvertInput(M4, param.pkePara, modulus, param.vhssPara.vhssEk_4, C1);

    // Evaluation Phase for Server 0: VHSS base computation plus OT result-hiding proof.
    PVHSS_MV y0_base, Y0_base, y1_base, Y1_base;
    int prf_key0 = 0;
    int prf_key1 = 0;
    timing = MeasureTimeMs([&]() {
        prf_key0 = 0;
        HssEvaluatePolyD2(y0_base, 0, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_1, prf_key0, param.pkePara.d, M1);
        HssEvaluatePolyD2(Y0_base, 0, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_3, prf_key0, param.pkePara.d, M3);
    }, cyctimes);
    PrintTimeMs("Evaluation base 0 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    timing = MeasureTimeMs([&]() {
        PVHSS_MV y = y0_base;
        PVHSS_MV Y = Y0_base;
        PVHSS_MV sk_b, SK_b;
        HssConvertInput(sk_b, param.pkePara, modulus, param.vhssPara.vhssEk_1, param.pk_f);
        HssConvertInput(SK_b, param.pkePara, modulus, param.vhssPara.vhssEk_3, param.pk_f);
        HssAddMemory(y, y, sk_b);
        HssAddMemory(Y, Y, SK_b);
        Prove(pi0, 0, y, Y, param, ekp0);
    }, cyctimes);
    PrintTimeMs("Evaluation incremental 0 OT algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Phase for Server 1.
    timing = MeasureTimeMs([&]() {
        prf_key1 = 0;
        HssEvaluatePolyD2(y1_base, 1, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_2, prf_key1, param.pkePara.d, M2);
        HssEvaluatePolyD2(Y1_base, 1, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_4, prf_key1, param.pkePara.d, M4);
    }, cyctimes);
    PrintTimeMs("Evaluation base 1 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    timing = MeasureTimeMs([&]() {
        PVHSS_MV y = y1_base;
        PVHSS_MV Y = Y1_base;
        PVHSS_MV sk_b, SK_b;
        HssConvertInput(sk_b, param.pkePara, modulus, param.vhssPara.vhssEk_2, param.pk_f);
        HssConvertInput(SK_b, param.pkePara, modulus, param.vhssPara.vhssEk_4, param.pk_f);
        HssAddMemory(y, y, sk_b);
        HssAddMemory(Y, Y, SK_b);
        Prove(pi1, 1, y, Y, param, ekp1);
    }, cyctimes);
    PrintTimeMs("Evaluation incremental 1 OT algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification Phase
    bool acc;
    timing = MeasureTimeMsAdaptive([&]() {
        acc = Verify(pi0, pi1, param.ck);
    }, cyctimes);
    PrintTimeMs("Verification algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // else
    // {
    //     cout << "Verification Failed\n";
    // }

    // Decryption Phase
    ZZ y;
    timing = MeasureTimeMsAdaptive([&]() {
        Decode(y, pi0, pi1, sk);
    }, cyctimes);
    PrintTimeMs("Decryption algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;
}

}}} // namespace pvhss::rlwe::ot
