#include "tester.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace group { namespace tcc25 {

void Tcc25_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for TCC25             " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num
              << "        cyctimes: " << cyctimes << std::endl;

    TimingResult timing;
    timing = MeasureTimeMs([&]() {
        Tcc25Param setup_param;
        Tcc25Server setup_server0, setup_server1;
        setup_param.skLen = 1024;
        setup_param.msg_bits = 32;
        setup_param.degree_f = degree_f;
        setup_param.msg_num = msg_num;
        Tcc25_Setup(setup_param, setup_server0, setup_server1);
        core_clean();
    }, cyctimes);
    PrintTimeMs("Setup algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    Tcc25Param param;
    Tcc25Server server0, server1;
    param.skLen = 1024;
    param.msg_bits = 32;
    param.degree_f = degree_f;
    param.msg_num = msg_num;
    Tcc25_Setup(param, server0, server1);

    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.msg_bits);
    }

    vector<Tcc25Ciphertext> Ix;
    timing = MeasureTimeMs([&]() {
        Tcc25_ProbGen(Ix, param, X);
    }, cyctimes);
    PrintTimeMs("Input algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    Tcc25ProofShare pi0, pi1;
    timing = MeasureTimeMs([&]() {
        Tcc25_Compute(pi0, 0, param, server0, Ix);
    }, cyctimes);
    PrintTimeMs("Evaluation 0 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    timing = MeasureTimeMs([&]() {
        Tcc25_Compute(pi1, 1, param, server1, Ix);
    }, cyctimes);
    PrintTimeMs("Evaluation 1 algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    Tcc25Proof proof;
    bool verified = false;
    timing = MeasureTimeMsAdaptive([&]() {
        verified = Tcc25_Verify(proof, pi0, pi1, param, X);
    }, cyctimes);
    PrintTimeMs("Verification algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    ZZ y;
    timing = MeasureTimeMsAdaptive([&]() {
        Tcc25_Decode(y, pi0, pi1, param);
    }, cyctimes);
    PrintTimeMs("Decryption algorithm time", timing);
    std::cout << "-------------------------------------------------------" << std::endl;

    (void)verified;
    core_clean();
}

}}} // namespace pvhss::group::tcc25
