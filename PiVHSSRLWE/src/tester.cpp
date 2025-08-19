#include "tester.h"

void VHSSRLWE_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for VHSSRLWE_time        " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    int msg_bit = 32;
    VHSS_Para vhssPara;
    vec_ZZ_pX pkePk;
    vec_ZZ_pX pkeSk;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    PKE_Para param;

    PKE_Gen(param, pkePk, pkeSk);
    ZZ_pXModulus modulus(param.xN);
    VHSS_Gen(vhssPara, param, modulus, pkeSk);

    auto *Time = new double[cyctimes];
    double time, mean, stdev;

    // Setup Phase
    for (int i = 0; i < cyctimes; i++)
    {
        VHSS_Para vhssPara00;
        vec_ZZ_pX pkePk00;
        vec_ZZ_pX pkeSk00;
        pkePk00.SetLength(2);
        pkeSk00.SetLength(2);
        PKE_Para param00;
        time = GetTime();
        PKE_Gen(param00, pkePk00, pkeSk00);
        ZZ_pXModulus modulus(param00.xN);
        VHSS_Gen(vhssPara00, param00, modulus, pkeSk00);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);

    cout << "Setup algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    // Input Generation Phase
    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], msg_bit);
    }

    // Input Processing Phase
    vector<vec_ZZ_pX> Ix;
    for (int i = 0; i < cyctimes; i++)
    {
        Ix.clear();
        time = GetTime();
        vec_ZZ_pX CTtmp;
        for (int j = 0; j < X.length(); ++j)
        {
            VHSS_Enc(CTtmp, param, modulus, pkePk, X[j]);
            Ix.push_back(CTtmp);
        }
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Input algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // Polynomial Generation Phase
    vector<vector<int>> F_TEST;
    Random_Func(F_TEST, msg_num, degree_f);

    vec_ZZ_pX C1;
    vec_ZZ_pX M1, M2, M3, M4;
    VHSS_Enc(C1, param, modulus, pkePk, ZZ(1));
    HSS_ConvertInput(M1, param, modulus, vhssPara.vhssEk_1, C1);
    HSS_ConvertInput(M2, param, modulus, vhssPara.vhssEk_2, C1);
    HSS_ConvertInput(M3, param, modulus, vhssPara.vhssEk_3, C1);
    HSS_ConvertInput(M4, param, modulus, vhssPara.vhssEk_4, C1);

    // Evaluation Phase for Server 0
    vec_ZZ_pX y_0_res, Y_0_res;
    int prf_key;
    for (int i = 0; i < cyctimes; i++)
    {
        prf_key = 0;
        time = GetTime();
        HSS_Evaluate_P_d2(y_0_res, 0, Ix, param, modulus, vhssPara.vhssEk_1, prf_key, degree_f, M1);
        HSS_Evaluate_P_d2(Y_0_res, 0, Ix, param, modulus, vhssPara.vhssEk_3, prf_key, degree_f, M2);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluation 0 algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Phase for Server 1
    vec_ZZ_pX y_1_res, Y_1_res;
    for (int i = 0; i < 1; i++)
    {
        prf_key = 0;
        time = GetTime();
        HSS_Evaluate_P_d2(y_1_res, 1, Ix, param, modulus, vhssPara.vhssEk_2, prf_key, degree_f, M1);
        HSS_Evaluate_P_d2(Y_1_res, 1, Ix, param, modulus, vhssPara.vhssEk_4, prf_key, degree_f, M2);
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
        acc = VHSS_Verify(y_0_res, y_1_res, Y_0_res, Y_1_res, vhssPara);
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
