#include "tester.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace group { namespace vhss {

void VHSSElg_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for VHSSElg            " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    VhssElgamalPk pk;
    VhssElgamalEk ek0, ek1;
    VhssElgamalVk vk;
    int skLen = 1024;
    int vkLen = 256;
    int msg_bits = 32;

    TimingResult timing;

    // Setup Phase
    timing = MeasureTimeMs([&]() {
        VhssElgamalPk pk000;
        VhssElgamalEk ek000, ek100;
        VhssElgamalVk vk000;
        VhssElgamalGen(pk000, vk000, ek000, ek100, skLen, vkLen);
    }, cyctimes);

    PrintTimeMs("Gen algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;
    // Key Generation Phase
    VhssElgamalGen(pk, vk, ek0, ek1, skLen, vkLen);

    // Input Generation Phase
    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        NTL::RandomBits(X[i], msg_bits);
    }

    // Input Processing Phase
    vector<VhssElgamalCt> Ix;
    VhssElgamalCt CTtmp;
    timing = MeasureTimeMs([&]() {
        Ix.clear();
        for (int j = 0; j < msg_num; ++j)
        {
            VhssElgamalInput(CTtmp, pk, X[j]);
            Ix.push_back(CTtmp);
        }
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    VhssElgamalMv y_0_res, y_1_res;
    int prf_key = 0;
    // Evaluation Phase for Server 0
    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        VhssElgamalEvaluatePD2(y_0_res, 0, Ix, pk, ek0, prf_key, degree_f);
    }, cyctimes);
    PrintTimeMs("Evaluation 0 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;
    // Evaluation Phase for Server 1
    timing = MeasureTimeMs([&]() {
        prf_key = 0;
        VhssElgamalEvaluatePD2(y_1_res, 1, Ix, pk, ek1, prf_key, degree_f);
    }, cyctimes);
    PrintTimeMs("Evaluation 1 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification Phase
    bool acc;
    timing = MeasureTimeMsAdaptive([&]() {
        acc = VhssElgamalVerify(y_0_res, y_1_res, vk);
    }, cyctimes);
    PrintTimeMs("Verification algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    // else
    // {
    //     cout << "Verification Failed\n";
    // }
}

}}} // namespace pvhss::group::vhss
