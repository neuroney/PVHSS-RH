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

    TimingResult timing;

    // Setup Phase
    timing = MeasureTimeMs([&]() {
        VHSSElg_PK pk000;
        VHSSElg_EK ek000, ek100;
        VHSSElg_VK vk000;
        VHSSElg_Gen(pk000, vk000, ek000, ek100, skLen, vkLen);
    }, cyctimes);

    PrintTimeMs("Gen algorithm time", timing);
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
    timing = MeasureTimeMs([&]() {
        Ix.clear();
        for (int j = 0; j < msg_num; ++j)
        {
            VHSSElg_Input(CTtmp, pk, X[j]);
            Ix.push_back(CTtmp);
        }
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    VHSSElg_MV y_0_res, y_1_res;
    int prf_key = 0;
    // Evaluation Phase for Server 0
    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        VHSSElg_Evaluate_P_d2(y_0_res, 0, Ix, pk, ek0, prf_key, degree_f);
    }, cyctimes);
    PrintTimeMs("Evaluation 0 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;
    // Evaluation Phase for Server 1
    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        VHSSElg_Evaluate_P_d2(y_1_res, 1, Ix, pk, ek1, prf_key, degree_f);
    }, cyctimes);
    PrintTimeMs("Evaluation 1 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification Phase
    bool acc;
    timing = MeasureTimeMsAdaptive([&]() {
        acc = VHSSElg_Verify(y_0_res, y_1_res, vk);
    }, cyctimes);
    PrintTimeMs("Verification algorithm time", timing);
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
