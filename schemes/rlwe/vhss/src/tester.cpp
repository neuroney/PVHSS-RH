#include "tester.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace rlwe { namespace vhss {

void VHSSRLWE_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for VHSSRLWE           " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    int msg_bit = 32;
    VHSS_Para vhssPara;
    vec_ZZ_pX pkePk;
    vec_ZZ_pX pkeSk;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    PKE_Para param;

    PKE_Gen(param, pkePk, pkeSk);
    ZZ_pXModulus modulus(param.xN);
    VHSS_Gen(vhssPara, param, modulus, pkeSk);

    TimingResult timing;

    // Setup Phase
    timing = MeasureTimeMs([&]() {
        VHSS_Para vhssPara00;
        vec_ZZ_pX pkePk00;
        vec_ZZ_pX pkeSk00;
        pkePk00.SetLength(2);
        pkeSk00.SetLength(2);
        PKE_Para param00;
        PKE_Gen(param00, pkePk00, pkeSk00);
        ZZ_pXModulus modulus(param00.xN);
        VHSS_Gen(vhssPara00, param00, modulus, pkeSk00);
    }, cyctimes);

    PrintTimeMs("Setup algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;
    // Input Generation Phase
    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        NTL::RandomBits(X[i], msg_bit);
    }

    // Input Processing Phase
    vector<vec_ZZ_pX> Ix;
    timing = MeasureTimeMs([&]() {
        Ix.clear();
        vec_ZZ_pX CTtmp;
        for (int j = 0; j < X.length(); ++j)
        {
            VHSS_Enc(CTtmp, param, modulus, pkePk, X[j]);
            Ix.push_back(CTtmp);
        }
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Polynomial Generation Phase
    vector<vector<int>> F_TEST;
    GenerateRandomFunc(F_TEST, msg_num, degree_f);

    vec_ZZ_pX C1;
    vec_ZZ_pX M1, M2, M3, M4;
    VHSS_Enc(C1, param, modulus, pkePk, ZZ(1));
    HssConvertInput(M1, param, modulus, vhssPara.vhssEk_1, C1);
    HssConvertInput(M2, param, modulus, vhssPara.vhssEk_2, C1);
    HssConvertInput(M3, param, modulus, vhssPara.vhssEk_3, C1);
    HssConvertInput(M4, param, modulus, vhssPara.vhssEk_4, C1);

    // Evaluation Phase for Server 0
    vec_ZZ_pX y_0_res, Y_0_res;
    int prf_key;
    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        HssEvaluatePolyD2(y_0_res, 0, Ix, param, modulus, vhssPara.vhssEk_1, prf_key, degree_f, M1);
        HssEvaluatePolyD2(Y_0_res, 0, Ix, param, modulus, vhssPara.vhssEk_3, prf_key, degree_f, M3);
    }, cyctimes);
    PrintTimeMs("Evaluation 0 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Phase for Server 1
    vec_ZZ_pX y_1_res, Y_1_res;
    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        HssEvaluatePolyD2(y_1_res, 1, Ix, param, modulus, vhssPara.vhssEk_2, prf_key, degree_f, M2);
        HssEvaluatePolyD2(Y_1_res, 1, Ix, param, modulus, vhssPara.vhssEk_4, prf_key, degree_f, M4);
    }, cyctimes);
    PrintTimeMs("Evaluation 1 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification Phase
    bool acc;
    timing = MeasureTimeMsAdaptive([&]() {
        acc = VHSS_Verify(y_0_res, y_1_res, Y_0_res, Y_1_res, vhssPara);
    }, cyctimes);
    PrintTimeMs("Verification algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // else
    // {
    //     cout << "Verification Failed\n";
    // }
}

bool VHSSRLWE_ACC_TEST(int msg_num, int degree_f)
{
    VHSS_Para vhssPara;
    vec_ZZ_pX pkePk;
    vec_ZZ_pX pkeSk;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);

    PKE_Para param;
    param.msg_bit = 32;
    param.d = degree_f;
    param.num_data = msg_num;
    PKE_Gen(param, pkePk, pkeSk);
    ZZ_pXModulus modulus(param.xN);
    VHSS_Gen(vhssPara, param, modulus, pkeSk);

    vec_ZZ X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        NTL::RandomBits(X[i], param.msg_bit);
    }

    vector<vec_ZZ_pX> Ix;
    vec_ZZ_pX CTtmp;
    for (int i = 0; i < msg_num; ++i)
    {
        VHSS_Enc(CTtmp, param, modulus, pkePk, X[i]);
        Ix.push_back(CTtmp);
    }

    vec_ZZ_pX C1;
    vec_ZZ_pX M1, M2, M3, M4;
    VHSS_Enc(C1, param, modulus, pkePk, ZZ(1));
    HssConvertInput(M1, param, modulus, vhssPara.vhssEk_1, C1);
    HssConvertInput(M2, param, modulus, vhssPara.vhssEk_2, C1);
    HssConvertInput(M3, param, modulus, vhssPara.vhssEk_3, C1);
    HssConvertInput(M4, param, modulus, vhssPara.vhssEk_4, C1);

    vec_ZZ_pX y_0_res, y_1_res, Y_0_res, Y_1_res;
    int prf_key = 0;
    HssEvaluatePolyD2(y_0_res, 0, Ix, param, modulus, vhssPara.vhssEk_1, prf_key, degree_f, M1);
    prf_key = 0;
    HssEvaluatePolyD2(Y_0_res, 0, Ix, param, modulus, vhssPara.vhssEk_3, prf_key, degree_f, M3);
    prf_key = 0;
    HssEvaluatePolyD2(y_1_res, 1, Ix, param, modulus, vhssPara.vhssEk_2, prf_key, degree_f, M2);
    prf_key = 0;
    HssEvaluatePolyD2(Y_1_res, 1, Ix, param, modulus, vhssPara.vhssEk_4, prf_key, degree_f, M4);

    const bool verified = VHSS_Verify(y_0_res, y_1_res, Y_0_res, Y_1_res, vhssPara);

    ZZ_pX y_sum = y_0_res[0] + y_1_res[0];
    ZZ y_eval = rep(y_sum[0]);
    if (y_eval > param.q / 2)
    {
        y_eval -= param.q;
    }

    ZZ y_native = PolyD(X, degree_f);
    cout << "True result: " << y_native << endl;
    cout << "Eval result: " << y_eval << endl;
    if (!verified)
    {
        cout << "P5 VHSS verification failed." << endl;
    }
    if (y_native != y_eval)
    {
        cout << "P5 VHSS accuracy check failed: decoded result differs from native computation." << endl;
    }
    return verified && (y_native == y_eval);
}

}}} // namespace pvhss::rlwe::vhss
