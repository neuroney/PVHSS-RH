#include "tester.h"
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <chrono>
#include <functional>
#include <string>

namespace pvhss { namespace rlwe { namespace hss {

using Clock = std::chrono::steady_clock;

static double MeasureMs(const std::function<void()>& fn, int iters)
{
    auto start = Clock::now();
    for (int i = 0; i < iters; ++i) fn();
    auto end = Clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count() / iters;
}

static void PrintMs(const std::string& label, double ms)
{
    printf("  %-16s %10.3f ms\n", label.c_str(), ms);
}

void HSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    printf("===== HSS-RLWE (NTL) =====\n");
    printf("  msg_num=%d  degree_f=%d  cyctimes=%d\n", msg_num, degree_f, cyctimes);

    // --- Gen ---
    double t_gen = MeasureMs([&]() {
        PKE_Para pkePara;
        vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2;
        pkePk.SetLength(2);
        pkeSk.SetLength(2);
        hssEk_1.SetLength(2);
        hssEk_2.SetLength(2);
        pkePara.msg_bit = 32;
        pkePara.d = degree_f;
        pkePara.num_data = msg_num;
        PKE_Gen(pkePara, pkePk, pkeSk);
        ZZ_pXModulus modulus(pkePara.xN);
        HssGen(hssEk_1, hssEk_2, pkePara, pkeSk);
    }, cyctimes);
    PrintMs("Setup", t_gen);

    // --- Pre-setup ---
    PKE_Para pkePara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    pkePara.msg_bit = 32;
    pkePara.num_data = msg_num;
    pkePara.d = degree_f;
    PKE_Gen(pkePara, pkePk, pkeSk);
    ZZ_pXModulus modulus(pkePara.xN);
    HssGen(hssEk_1, hssEk_2, pkePara, pkeSk);

    // --- Enc ---
    vec_ZZ_pX ct;
    double t_enc = MeasureMs([&]() {
        ZZ x;
        RandomBits(x, pkePara.msg_bit);
        HSS_Enc(ct, pkePara, modulus, pkePk, x);
    }, cyctimes);
    PrintMs("Encrypt", t_enc);

    // --- Load (HSS_Mult) ---
    HSS_Enc(ct, pkePara, modulus, pkePk, ZZ(12345));
    vec_ZZ_pX mem_out;
    double t_load = MeasureMs([&]() {
        HSS_Mult(mem_out, pkePara, modulus, hssEk_1, ct);
    }, cyctimes);
    PrintMs("Load(Mult)", t_load);

    // --- Eval ---
    Data data;
    GenerateData(data, pkePara, pkePk);
    vec_ZZ_pX M1;
    HssConvertInput(M1, pkePara, modulus, hssEk_1, data.C_X[0]);

    double t_eval = MeasureMs([&]() {
        vec_ZZ_pX y_out;
        int prf_key = 0;
        HssEvaluatePolyD2(y_out, 1, data.C_X, pkePara, modulus, hssEk_1, prf_key, degree_f, M1);
    }, cyctimes);
    PrintMs("Eval", t_eval);
}

}}} // namespace pvhss::rlwe::hss

