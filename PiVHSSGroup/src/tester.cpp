#include "tester.h"

void VHSSElg_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for VHSSElg time        " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    VHSSElg_PK pk;
    VHSSElg_EK ek0, ek1;
    VHSSElg_VK vk;
    int skLen = 1024;
    int vkLen = 256;
    int msg_bits = 32;

    auto *Time = new double[cyctimes];
    double time, mean, stdev;

    // Setup Phase
    for (int i = 0; i < cyctimes; i++)
    {
        VHSSElg_PK pk000;
        VHSSElg_EK ek000, ek100;
        VHSSElg_VK vk000;
        time = GetTime();
        VHSSElg_Gen(pk000, vk000, ek000, ek100, skLen, vkLen);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);

    cout << "Gen algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    // Key Generation Phase
    VHSSElg_Gen(pk, vk, ek0, ek1, skLen, vkLen);

    // Input Generation Phase
    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], msg_bits);
    }

    // Input Processing Phase
    vector<VHSSElg_CT> Ix;
    VHSSElg_CT CTtmp;
    for (int i = 0; i < cyctimes; i++)
    {
        Ix.clear();
        time = GetTime();
        for (int j = 0; j < msg_num; ++j)
        {
            VHSSElg_Input(CTtmp, pk, X[i]);
            Ix.push_back(CTtmp);
        }
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Input algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    VHSSElg_MV y_0_res, y_1_res;
    int prf_key = 0;
    // Evaluation Phase for Server 0
    for (int i = 0; i < cyctimes; i++)
    {
        prf_key = 0;
        time = GetTime();
        VHSSElg_Evaluate_P_d2(y_0_res, 0, Ix, pk, ek0, prf_key, degree_f);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluation 0 algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    // Evaluation Phase for Server 1
    for (int i = 0; i < cyctimes; i++)
    {
        prf_key = 0;
        time = GetTime();
        VHSSElg_Evaluate_P_d2(y_1_res, 1, Ix, pk, ek1, prf_key, degree_f);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluation 1 algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification Phase
    bool acc;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        acc = VHSSElg_Verify(y_0_res, y_1_res, vk);
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
}
