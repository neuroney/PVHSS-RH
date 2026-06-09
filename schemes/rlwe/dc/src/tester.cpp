#include "tester.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace rlwe { namespace dc {

void PVHSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for PVHSSRLWE_DC       " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    PVHSSPara param;
    vec_ZZ_pX pkePk;

    PROOF pi0, pi1;
    bn_t ekp0[2], ekp1[2];


    TimingResult timing;

    // Setup Phase
    timing = MeasureTimeMs([&]() {
        PVHSSPara param00;
        vec_ZZ_pX pkePk00;
        PVHSS_SK sk00;
        Setup(param00, pkePk00, msg_num, degree_f);
        ZZ_pXModulus modulus00(param00.pkePara.xN);
    }, cyctimes);

    PrintTimeMs("Setup algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;
    // Key Generation Phase
    timing = MeasureTimeMs([&]() {
        PVHSSPara param00;
        vec_ZZ_pX pkePk00;
        PVHSS_SK sk00;
        bn_t ekp000[2], ekp100[2];

        Setup(param00, pkePk00, msg_num, degree_f);
        ZZ_pXModulus modulus00(param00.pkePara.xN);
        KeyGen(param00, sk00, modulus00, pkePk00, ekp000, ekp100);
    }, cyctimes);

    PrintTimeMs("KeyGen algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;





    Setup(param, pkePk, msg_num, degree_f);
    ZZ_pXModulus modulus(param.pkePara.xN);
    KeyGen(param, param.f_sk, modulus, pkePk, ekp0, ekp1);

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

    // Polynomial Generation Phase
    vector<vector<int>> F_TEST;
    GenerateRandomFunc(F_TEST, msg_num, degree_f);

    PVHSS_CT C1;
    PVHSS_MV M1, M2, M3, M4;
    VHSS_Enc(C1, param.pkePara, modulus, pkePk, ZZ(1));
    HssConvertInput(M1, param.pkePara, modulus, param.vhssPara.vhssEk_1, C1);
    HssConvertInput(M2, param.pkePara, modulus, param.vhssPara.vhssEk_2, C1);
    HssConvertInput(M3, param.pkePara, modulus, param.vhssPara.vhssEk_3, C1);
    HssConvertInput(M4, param.pkePara, modulus, param.vhssPara.vhssEk_4, C1);

    // Evaluation Phase for Server 0
    timing = MeasureTimeMs([&]() {
        Compute(pi0, 0, param, param.vhssPara.vhssEk_1, param.vhssPara.vhssEk_3, Ix, modulus, M1, M3, F_TEST, ekp0);
    }, cyctimes);
    PrintTimeMs("Evaluation 0 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Phase for Server 1
    timing = MeasureTimeMs([&]() {
        Compute(pi1, 1, param, param.vhssPara.vhssEk_2, param.vhssPara.vhssEk_4, Ix, modulus, M2, M4, F_TEST, ekp1);
    }, 1);
    PrintTimeMs("Evaluation 1 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification Phase
    bool acc;
    timing = MeasureTimeMsAdaptive([&]() {
        acc = Verify(pi0, pi1, param);
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
        Decode(y, pi0, pi1, param.f_sk);
    }, cyctimes);
    PrintTimeMs("Decryption algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;
}

}}} // namespace pvhss::rlwe::dc
