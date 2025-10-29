#include "tester.h"

void PVHSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for PVHSS_time        " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    PVHSSPara param;
    vec_ZZ_pX pkePk;
    PVHSS_SK sk;

    PROOF pi0, pi1;
    bn_t ekp0, ekp1;

    auto *Time = new double[cyctimes];
    double time, mean, stdev;

    // Setup Phase
    for (int i = 0; i < cyctimes; i++)
    {
        PVHSSPara param00;
        vec_ZZ_pX pkePk00;
        PVHSS_SK sk00;
        time = GetTime();
        Setup(param00, pkePk00, msg_num, degree_f);
        ZZ_pXModulus modulus00(param00.pkePara.xN);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);

    cout << "Setup algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    // Key Generation Phase
    for (int i = 0; i < cyctimes; i++)
    {
        PVHSSPara param00;
        vec_ZZ_pX pkePk00;
        PVHSS_SK sk00;
        bn_t ekp000, ekp100;
        Setup(param00, pkePk00, msg_num, degree_f);
        ZZ_pXModulus modulus00(param00.pkePara.xN);
          time = GetTime();
        KeyGen(param00, sk00, modulus00, pkePk00, ekp000, ekp100);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);

    cout << "KeyGen algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;


    Setup(param, pkePk, msg_num, degree_f);
    ZZ_pXModulus modulus(param.pkePara.xN);
    KeyGen(param, sk, modulus, pkePk, ekp0, ekp1);

    // Input Generation Phase
    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.pkePara.msg_bit);
    }

    // Input Processing Phase
    vector<PVHSS_CT> Ix;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        ProbGen(Ix, param, X, modulus, pkePk);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Input algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // Polynomial Generation Phase
    vector<vector<int>> F_TEST;
    Random_Func(F_TEST, msg_num, degree_f);

    PVHSS_CT C1;
    PVHSS_MV M1, M2, M3, M4;
    VHSS_Enc(C1, param.pkePara, modulus, pkePk, ZZ(1));
    HSS_ConvertInput(M1, param.pkePara, modulus, param.vhssPara.vhssEk_1, C1);
    HSS_ConvertInput(M2, param.pkePara, modulus, param.vhssPara.vhssEk_2, C1);
    HSS_ConvertInput(M3, param.pkePara, modulus, param.vhssPara.vhssEk_3, C1);
    HSS_ConvertInput(M4, param.pkePara, modulus, param.vhssPara.vhssEk_4, C1);

    // Evaluation Phase for Server 0
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        Compute(pi0, 0, param, param.vhssPara.vhssEk_1, param.vhssPara.vhssEk_3, Ix, modulus, M1, M3, F_TEST, ekp0);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluation 0 algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Phase for Server 1
    for (int i = 0; i < 1; i++)
    {
        time = GetTime();
        Compute(pi1, 1, param, param.vhssPara.vhssEk_2, param.vhssPara.vhssEk_4, Ix, modulus, M2, M4, F_TEST, ekp1);
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
        acc = Verify(pi0, pi1, param.ck);
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
    ZZ y;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        Decode(y, pi0, pi1, sk);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Decryption algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;
}
