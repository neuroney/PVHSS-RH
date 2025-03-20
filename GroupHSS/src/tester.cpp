#include "tester.h"

void HSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    HSS_PK pk;
    HSS_EK ek0, ek1;
    int msg_bits = 256;
        int skLen = 256;

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

    for (int i = 0; i < cyctimes; i++)
    {
        ZZ X_single;
        RandomBits(X_single, msg_bits);
        HSS_CT X_Input;
        time = GetTime();
        HSS_Input(X_Input, pk, X_single);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "HSS_Input algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    for (int i = 0; i < cyctimes; i++)
    {
        ZZ X_single;
        RandomBits(X_single, msg_bits);
        HSS_CT X_Input;
        HSS_MV X_Memory;
        HSS_Input(X_Input, pk, X_single);
        int prf_key = 0;
        time = GetTime();
        HSS_ConvertInput(X_Memory, 0, pk, ek0, X_Input, prf_key);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "HSS_ConvertInput algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    for (int i = 0; i < cyctimes; i++)
    {
        ZZ X1, X2;
        RandomBits(X1, msg_bits);
        RandomBits(X2, msg_bits);
        HSS_CT X_Input_1, X_Input_2;
        HSS_Input(X_Input_1, pk, X1);
        HSS_Input(X_Input_2, pk, X2);

        HSS_MV z_Memory, X_Memory_1, X_Memory_2;
        int prf_key = 0;
        HSS_ConvertInput(X_Memory_1, 0, pk, ek0, X_Input_1, prf_key);
        HSS_ConvertInput(X_Memory_2, 0, pk, ek0, X_Input_2, prf_key);

        time = GetTime();
        HSS_AddMemory(z_Memory, pk, X_Memory_1, X_Memory_2);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "HSS_AddMemory algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    for (int i = 0; i < cyctimes; i++)
    {
        ZZ X;
        RandomBits(X, msg_bits);
        HSS_CT X_Input;
        HSS_Input(X_Input, pk, X);

        HSS_MV z_Memory, X_Memory;
        int prf_key = 0;
        HSS_ConvertInput(X_Memory, 0, pk, ek0, X_Input, prf_key);

        time = GetTime();
        HSS_Mul(z_Memory, 0,pk, X_Input, X_Memory, prf_key);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "HSS_Mul algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

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
    for (int i = 0; i < Ix.size(); ++i)
    {
        HSS_Input(CTtmp,pk,X[i]);
        Ix.push_back(CTtmp);
    }

    HSS_MV y0, y1;
    for (int i = 0; i < cyctimes; i++)
    {
        int prf_key = 0;
        time = GetTime();
        //HSS_Evaluate(y0, 0, Ix, pk, ek0, prf_key, F_TEST);
        HSS_Evaluate_P_d2(y0, 0, Ix, pk, ek0, prf_key, degree_f);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluate0 algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    for (int i = 0; i < cyctimes; i++)
    {
        int prf_key = 0;
        time = GetTime();
        HSS_Evaluate_P_d2(y1, 1, Ix, pk, ek1, prf_key, degree_f);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluate1 algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
}