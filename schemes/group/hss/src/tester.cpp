#include "tester.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace group { namespace hss {

void HSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    HssPublicKey pk;
    HssEvalKey ek0, ek1;
    int msg_bits = 256;
    int skLen = 1024;

    TimingResult timing;
    timing = MeasureTimeMs([&]() {
        HssGen(pk, ek0, ek1, skLen);
    }, cyctimes);
    PrintTimeMs("HssGen algo time", timing);

    ZZ X_single;
    NTL::RandomBits(X_single, msg_bits);
    HssCiphertext X_Input;
    timing = MeasureTimeMs([&]() {
        HssInput(X_Input, pk, X_single);
    }, cyctimes);
    PrintTimeMs("HssInput algo time", timing);

    int prf_key = 0;

    ZZ Y_single;

    vector<vector<int>> F_TEST;
    GenerateRandomFunc(F_TEST, msg_num, degree_f);

    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        NTL::RandomBits(X[i], msg_bits);
    }

    vector<HssCiphertext> Ix;
    HssCiphertext CTtmp;
    for (int i = 0; i < X.length(); ++i)
    {
        HssInput(CTtmp, pk, X[i]);
        Ix.push_back(CTtmp);
    }

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
