#include "tester.h"

void HSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    HSS_PK pk;
    HSS_EK ek0, ek1;
    int msg_bits = 256;
    int skLen = 1024;

    auto *Time = new double[cyctimes];
    double time, mean, stdev;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        HSS_Gen(pk, ek0, ek1, skLen);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "HSS_Gen algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    ZZ X_single;
    RandomBits(X_single, msg_bits);
    HSS_CT X_Input;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        HSS_Input(X_Input, pk, X_single);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "HSS_Input algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    // HSS_MV X_Memory;
    // HSS_Input(X_Input, pk, X_single);
    int prf_key = 0;
    // for (int i = 0; i < cyctimes; i++)
    // {
    //     time = GetTime();
    //     HSS_ConvertInput(X_Memory, 0, pk, ek0, X_Input, prf_key);
    //     Time[i] = GetTime() - time;
    // }
    // DataProcess(mean, stdev, Time, cyctimes);
    // cout << "HSS_ConvertInput algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    ZZ Y_single;
    // RandomBits(Y_single, msg_bits);
    // HSS_CT Y_Input;
    // HSS_Input(Y_Input, pk, Y_single);
    // prf_key = 0;
    // HSS_ConvertInput(X_Memory, 0, pk, ek0, X_Input, prf_key);
    // HSS_MV Y_Memory;
    // prf_key = 0;
    // HSS_ConvertInput(Y_Memory, 0, pk, ek0, Y_Input, prf_key);
    // for (int i = 0; i < cyctimes; i++)
    // {
    //     HSS_MV Z_Memory;
    //     time = GetTime();
    //     HSS_AddMemory(Z_Memory, pk, X_Memory, Y_Memory);
    //     Time[i] = GetTime() - time;
    // }
    // DataProcess(mean, stdev, Time, cyctimes);
    // cout << "HSS_AddMemory algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    // for (int i = 0; i < cyctimes; i++)
    // {
    //     HSS_MV Z_Memory;
    //     time = GetTime();
    //     HSS_Mul(Z_Memory, 0, pk, X_Input, Y_Memory, prf_key);
    //     Time[i] = GetTime() - time;
    // }
    // DataProcess(mean, stdev, Time, cyctimes);
    // cout << "HSS_Mul algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

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
    for (int i = 0; i < cyctimes; i++)
    {
        prf_key = 0;
        time = GetTime();
        // HSS_Evaluate(y0, 0, Ix, pk, ek0, prf_key, F_TEST);
        HSS_Evaluate_P_d2(y0, 0, Ix, pk, ek0, prf_key, degree_f);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluate0 algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    for (int i = 0; i < cyctimes; i++)
    {
        prf_key = 0;
        time = GetTime();
        HSS_Evaluate_P_d2(y1, 1, Ix, pk, ek1, prf_key, degree_f);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluate1 algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        HSS_Dec(Y_single, y0, y1);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Dec algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
}