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
    PKE_Para pkePara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    pkePara.msg_bit = 32;
    pkePara.num_data = msg_num;
    pkePara.d = degree_f;

    int msg_bit = pkePara.msg_bit;

    TimingResult timing;

    // Setup Phase
    timing = MeasureTimeMs([&]() {
        PKE_Para pkePara00;
        vec_ZZ_pX pkePk00, pkeSk00, hssEk00_1, hssEk00_2;
        pkePk00.SetLength(2);
        pkeSk00.SetLength(2);
        hssEk00_1.SetLength(2);
        hssEk00_2.SetLength(2);
        pkePara00.msg_bit = 32;
        pkePara00.num_data = msg_num;
        pkePara00.d = degree_f;
        PKE_Gen(pkePara00, pkePk00, pkeSk00);
        ZZ_pXModulus modulus(pkePara00.xN);
        HssGen(hssEk00_1, hssEk00_2, pkePara00, pkeSk00);
    }, cyctimes);
    PrintTimeMs("Setup algorithm time", timing);

    // Pre-setup for other phases
    PKE_Gen(pkePara, pkePk, pkeSk);
    ZZ_pXModulus modulus(pkePara.xN);
    HssGen(hssEk_1, hssEk_2, pkePara, pkeSk);

    // Input Generation Phase
    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        NTL::RandomBits(X[i], msg_bit);
    }

    // Input Processing Phase
    Vec<vec_ZZ_pX> Ix;
    timing = MeasureTimeMs([&]() {
        Ix.SetLength(0);
        vec_ZZ_pX CTtmp;
        for (int j = 0; j < X.length(); ++j)
        {
            HSS_Enc(CTtmp, pkePara, modulus, pkePk, X[j]);
            Ix.append(CTtmp);
        }
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);

    // Evaluation Phase for Server 0
    vec_ZZ_pX M1;
    HssConvertInput(M1, pkePara, modulus, hssEk_1, Ix[0]);

    vec_ZZ_pX y0, y1;
    int prf_key = 0;
    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        HssEvaluatePolyD2(y0, 0, Ix, pkePara, modulus, hssEk_1, prf_key, degree_f, M1);
    }, cyctimes);
    PrintTimeMs("Evaluation 0 algorithm time", timing);

    // Evaluation Phase for Server 1
    vec_ZZ_pX M1b;
    HssConvertInput(M1b, pkePara, modulus, hssEk_2, Ix[0]);

    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        HssEvaluatePolyD2(y1, 1, Ix, pkePara, modulus, hssEk_2, prf_key, degree_f, M1b);
    }, cyctimes);
    PrintTimeMs("Evaluation 1 algorithm time", timing);
}

}}} // namespace pvhss::rlwe::hss

