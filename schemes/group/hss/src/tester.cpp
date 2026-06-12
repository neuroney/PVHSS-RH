#include "tester.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace group { namespace hss {

void HSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for HSSElg            " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    HssPublicKey pk;
    HssEvalKey ek0, ek1;
    int msg_bits = 32;
    int skLen = 1024;

    TimingResult timing;
    timing = MeasureTimeMs([&]() {
        HssGen(pk, ek0, ek1, skLen);
    }, cyctimes);
    PrintTimeMs("Setup algorithm time", timing);

    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        NTL::RandomBits(X[i], msg_bits);
    }

    vector<HssCiphertext> Ix;
    timing = MeasureTimeMs([&]() {
        Ix.clear();
        HssCiphertext CTtmp;
        for (int j = 0; j < X.length(); ++j)
        {
            HssInput(CTtmp, pk, X[j]);
            Ix.push_back(CTtmp);
        }
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);

    int prf_key = 0;
    ZZ Y_single;

    vector<vector<int>> F_TEST;
    GenerateRandomFunc(F_TEST, msg_num, degree_f);

    HssMemoryValue y0, y1;
    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        // HssEvaluate(y0, 0, Ix, pk, ek0, prf_key, F_TEST);
        HssEvaluatePolyD2(y0, 0, Ix, pk, ek0, prf_key, degree_f);
    }, cyctimes);
    PrintTimeMs("Evaluate0 algo time", timing);

    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        HssEvaluatePolyD2(y1, 1, Ix, pk, ek1, prf_key, degree_f);
    }, cyctimes);
    PrintTimeMs("Evaluate1 algo time", timing);

    timing = MeasureTimeMsAdaptive([&]() {
        HssDec(Y_single, y0, y1);
    }, cyctimes);
    PrintTimeMs("Dec algo time", timing);
}

}}} // namespace pvhss::group::hss
