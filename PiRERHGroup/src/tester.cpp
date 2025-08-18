#include "tester.h"

void PVHSSElg2_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for PVHSSElg2_time        " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    PVHSSElg2_Para param;
    PVHSSElg2_EK ek0, ek1;
    PVHSSElg2_SK sk;
    param.skLen = 1024;
    param.vkLen = 256;
    param.msg_bits = 32;
    param.degree_f = degree_f;
    param.msg_num = msg_num;

    PROOF pi0, pi1;

    auto *Time = new double[cyctimes];
    double time, mean, stdev;

    // Setup Phase
    for (int i = 0; i < cyctimes; i++)
    {
        PVHSSElg2_Para param00;
        PVHSSElg2_EK ek000, ek100;
        PVHSSElg2_SK sk000;
        time = GetTime();
        param00.skLen = 1024;
        param00.vkLen = 256;
        param00.msg_bits = 32;
        param00.degree_f = degree_f;
        param00.msg_num = msg_num;
        PVHSSElg2_Setup(param00, ek000, ek100, sk000);
        PVHSSElg2_KeyGen(param00, sk000);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);

    cout << "Setup algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    // Key Generation Phase
    PVHSSElg2_Setup(param, ek0, ek1, sk);
    PVHSSElg2_KeyGen(param, sk);

    // Input Generation Phase
    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.msg_bits);
    }

    // Input Processing Phase
    vector<PVHSSElg2_CT> Ix;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        PVHSSElg2_ProbGen(Ix, param, X);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Input algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // Polynomial Generation Phase
    vector<vector<int>> F_TEST;
    Random_Func(F_TEST, msg_num, degree_f);

    // Evaluation Phase for Server 0
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        PVHSSElg2_Compute(pi0, 0, param, ek0, Ix, F_TEST);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluation 0 algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Phase for Server 1
    for (int i = 0; i < 1; i++)
    {
        time = GetTime();
        PVHSSElg2_Compute(pi1, 1, param, ek1, Ix, F_TEST);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, 1);
    cout << "Evaluation 1 algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification Phase
    bool acc;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        acc = PVHSSElg2_Verify(pi0, pi1, param);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Verification algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // if (acc)
    // {
    //     cout << "Verification Passed\n";
    // }
    // else
    // {
    //     cout << "Verification Failed\n";
    // }

    // Decryption Phase
    dig_t y;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        PVHSSElg2_Decode(y, pi0, pi1, sk);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Decryption algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;
}
