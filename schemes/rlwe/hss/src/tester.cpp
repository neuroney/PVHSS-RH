#include "tester.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace rlwe { namespace hss {

void HSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for HSSRLWE           " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;

    TimingResult timing;

    timing = MeasureTimeMs([&]() {
        PKE_Para param;
        param.msg_bit = 32;
        param.num_data = msg_num;
        param.d = degree_f;

        vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2;
        pkePk.SetLength(2);
        pkeSk.SetLength(2);
        hssEk_1.SetLength(2);
        hssEk_2.SetLength(2);

        PKE_Gen(param, pkePk, pkeSk);
        HssGen(hssEk_1, hssEk_2, param, pkeSk);
    }, cyctimes);
    PrintTimeMs("Setup algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    PKE_Para param;
    param.msg_bit = 32;
    param.num_data = msg_num;
    param.d = degree_f;

    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);

    PKE_Gen(param, pkePk, pkeSk);
    ZZ_pXModulus modulus(param.xN);
    HssGen(hssEk_1, hssEk_2, param, pkeSk);

    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.msg_bit);
    }

    Vec<vec_ZZ_pX> Ix;
    timing = MeasureTimeMs([&]() {
        Ix.SetLength(0);
        vec_ZZ_pX CTtmp;
        for (int j = 0; j < X.length(); ++j)
        {
            HSS_Enc(CTtmp, param, modulus, pkePk, X[j]);
            Ix.append(CTtmp);
        }
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    vec_ZZ_pX C1;
    vec_ZZ_pX M1, M2;
    HSS_Enc(C1, param, modulus, pkePk, ZZ(1));
    HssConvertInput(M1, param, modulus, hssEk_1, C1);
    HssConvertInput(M2, param, modulus, hssEk_2, C1);

    vec_ZZ_pX y_0_res, y_1_res;
    int prf_key = 0;

    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        HssEvaluatePolyD2(y_0_res, 0, Ix, param, modulus, hssEk_1, prf_key, degree_f, M1);
    }, cyctimes);
    PrintTimeMs("Evaluation 0 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        HssEvaluatePolyD2(y_1_res, 1, Ix, param, modulus, hssEk_2, prf_key, degree_f, M2);
    }, cyctimes);
    PrintTimeMs("Evaluation 1 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    ZZ_pX decoded;
    timing = MeasureTimeMsAdaptive([&]() {
        decoded = y_0_res[0] + y_1_res[0];
    }, cyctimes);
    PrintTimeMs("Dec algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;
}

}}} // namespace pvhss::rlwe::hss
