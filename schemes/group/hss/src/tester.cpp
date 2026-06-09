#include "tester.h"

void HSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    HSS_PK pk;
    HSS_EK ek0, ek1;
    int msg_bits = 256;
    int skLen = 1024;

    TimingResult timing;
    timing = MeasureTimeMs([&]() {
        HSS_Gen(pk, ek0, ek1, skLen);
    }, cyctimes);
    PrintTimeMs("HSS_Gen algo time", timing);

    ZZ X_single;
    RandomBits(X_single, msg_bits);
    HSS_CT X_Input;
    timing = MeasureTimeMs([&]() {
        HSS_Input(X_Input, pk, X_single);
    }, cyctimes);
    PrintTimeMs("HSS_Input algo time", timing);

    int prf_key = 0;

    ZZ Y_single;

    vector<vector<int>> F_TEST;
    Random_Func(F_TEST, msg_num, degree_f);

    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], msg_bits);
    }

    vector<HSS_CT> Ix;
    HSS_CT CTtmp;
    for (int i = 0; i < X.length(); ++i)
    {
        HSS_Input(CTtmp, pk, X[i]);
        Ix.push_back(CTtmp);
    }

    HSS_MV y0, y1;
    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        // HSS_Evaluate(y0, 0, Ix, pk, ek0, prf_key, F_TEST);
        HSS_Evaluate_P_d2(y0, 0, Ix, pk, ek0, prf_key, degree_f);
    }, cyctimes);
    PrintTimeMs("Evaluate0 algo time", timing);

    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        HSS_Evaluate_P_d2(y1, 1, Ix, pk, ek1, prf_key, degree_f);
    }, cyctimes);
    PrintTimeMs("Evaluate1 algo time", timing);

    timing = MeasureTimeMsAdaptive([&]() {
        HSS_Dec(Y_single, y0, y1);
    }, cyctimes);
    PrintTimeMs("Dec algo time", timing);
}
