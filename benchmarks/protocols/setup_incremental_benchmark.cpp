#include "../../common/include/helper.h"
#include "../../schemes/cz/function.h"
#include "../../schemes/group/dc/include/PiDCGroup.h"
#include "../../schemes/group/hss/include/HSSElg.h"
#include "../../schemes/group/ot/include/PiOTGroup.h"
#include "../../schemes/rlwe/dc/include/PiDCRLWE.h"
#include "../../schemes/rlwe/hss/include/HSSRLWE.h"
#include "../../schemes/rlwe/ot/include/PiOTRLWE.h"
#include "../../schemes/rlwe/vhss/include/VHSSRLWE.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <vector>

using namespace NTL;
using namespace std;

namespace
{
struct Row
{
    string backend;
    string scheme;
    string phase;
    string server;
    int samples;
    double mean_ms;
    double rsd_percent;
};

using BenchFn = std::function<void()>;

static string csv_escape(const string &value)
{
    bool needs_quotes = false;
    string escaped;
    for (size_t i = 0; i < value.size(); ++i)
    {
        const char c = value[i];
        if (c == '"' || c == ',' || c == '\n' || c == '\r')
        {
            needs_quotes = true;
        }
        if (c == '"')
        {
            escaped += "\"\"";
        }
        else
        {
            escaped += c;
        }
    }
    return needs_quotes ? "\"" + escaped + "\"" : escaped;
}

static string dirname_of(const string &path)
{
    const size_t pos = path.find_last_of('/');
    if (pos == string::npos)
    {
        return "";
    }
    return path.substr(0, pos);
}

static void make_dirs(const string &path)
{
    if (path.empty())
    {
        return;
    }

    size_t pos = (path[0] == '/') ? 1 : 0;
    while (pos <= path.size())
    {
        pos = path.find('/', pos);
        const string dir = path.substr(0, pos);
        if (!dir.empty() && mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST)
        {
            throw runtime_error("mkdir failed for " + dir + ": " + strerror(errno));
        }
        if (pos == string::npos)
        {
            break;
        }
        ++pos;
    }
}

static void write_rows(const string &path, const vector<Row> &rows)
{
    make_dirs(dirname_of(path));
    ofstream out(path.c_str());
    if (!out)
    {
        throw runtime_error("failed to write " + path);
    }

    out << "backend,scheme,phase,server,samples,mean_ms,rsd_percent\n";
    out << fixed << setprecision(6);
    for (size_t i = 0; i < rows.size(); ++i)
    {
        out << csv_escape(rows[i].backend) << ","
            << csv_escape(rows[i].scheme) << ","
            << csv_escape(rows[i].phase) << ","
            << csv_escape(rows[i].server) << ","
            << rows[i].samples << ","
            << rows[i].mean_ms << ","
            << rows[i].rsd_percent << "\n";
    }
}

static Row run_row(const string &backend, const string &scheme, const string &phase,
                   const string &server, int samples, const BenchFn &fn)
{
    if (getenv("PVHSS_BENCH_TRACE") != NULL)
    {
        cerr << "running " << backend << "," << scheme << "," << phase;
        if (!server.empty())
        {
            cerr << "," << server;
        }
        cerr << "\n";
    }
    TimingResult timing = MeasureTimeMs(fn, samples);
    return Row{backend, scheme, phase, server, timing.samples, timing.mean_ms, timing.rsd * 100.0};
}

static int read_int_arg(int argc, char **argv, const string &name, int fallback)
{
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (argv[i] == name)
        {
            return atoi(argv[i + 1]);
        }
    }
    return fallback;
}

static string read_string_arg(int argc, char **argv, const string &name, const string &fallback)
{
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (argv[i] == name)
        {
            return argv[i + 1];
        }
    }
    return fallback;
}

static bool has_arg(int argc, char **argv, const string &name)
{
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == name)
        {
            return true;
        }
    }
    return false;
}

static void init_group_ot_params(pvhss::group::ot::PVHSSElg1_Para &param, int msg_num, int degree)
{
    param.skLen = 1024;
    param.vkLen = 256;
    param.msg_bits = 32;
    param.degree_f = degree;
    param.msg_num = msg_num;
}

static void init_group_dc_params(pvhss::group::dc::PVHSSElg2_Para &param, int msg_num, int degree)
{
    param.skLen = 1024;
    param.vkLen = 256;
    param.msg_bits = 1;
    param.degree_f = degree;
    param.msg_num = msg_num;
}

static void add_setup_gen_rows(vector<Row> &rows, int samples, int msg_num, int degree)
{
    namespace ghss = pvhss::group::hss;
    namespace got = pvhss::group::ot;
    namespace gdc = pvhss::group::dc;
    namespace rhss = pvhss::rlwe::hss;
    namespace rvhss = pvhss::rlwe::vhss;
    namespace rot = pvhss::rlwe::ot;
    namespace rdc = pvhss::rlwe::dc;

    rows.push_back(run_row("group", "hss", "setup", "", samples, [&]() {
        ghss::HssPublicKey pk;
        ghss::HssEvalKey ek0, ek1;
        ghss::HssGen(pk, ek0, ek1, 1024);
    }));

    rows.push_back(run_row("group", "vhss", "setup", "", samples, [&]() {
        VhssElgamalPk pk;
        VhssElgamalVk vk;
        VhssElgamalEk ek0, ek1;
        VhssElgamalGen(pk, vk, ek0, ek1, 1024, 256);
    }));

    rows.push_back(run_row("group", "ot", "setup", "", samples, [&]() {
        got::PVHSSElg1_Para param;
        got::PVHSSElg1_EK ek0, ek1;
        init_group_ot_params(param, msg_num, degree);
        got::PVHSSElg1_Setup(param, ek0, ek1);
    }));

    {
        got::PVHSSElg1_Para param;
        got::PVHSSElg1_EK ek0, ek1;
        got::PVHSSElg1_SK sk;
        init_group_ot_params(param, msg_num, degree);
        got::PVHSSElg1_Setup(param, ek0, ek1);
        bn_t ekp0, ekp1;
        bn_new(ekp0);
        bn_new(ekp1);
        rows.push_back(run_row("group", "ot", "gen", "", samples, [&]() {
            got::PVHSSElg1_KeyGen(param, sk, ekp0, ekp1);
        }));
    }

    rows.push_back(run_row("group", "dc", "setup", "", samples, [&]() {
        gdc::PVHSSElg2_Para param;
        gdc::PVHSSElg2_EK ek0, ek1;
        gdc::PVHSSElg2_SK sk;
        init_group_dc_params(param, msg_num, degree);
        gdc::PVHSSElg2_Setup(param, ek0, ek1, sk);
    }));

    {
        gdc::PVHSSElg2_Para param;
        gdc::PVHSSElg2_EK ek0, ek1;
        gdc::PVHSSElg2_SK sk;
        init_group_dc_params(param, msg_num, degree);
        gdc::PVHSSElg2_Setup(param, ek0, ek1, sk);
        bn_t ekp0[2], ekp1[2];
        for (int i = 0; i < 2; ++i)
        {
            bn_new(ekp0[i]);
            bn_new(ekp1[i]);
        }
        rows.push_back(run_row("group", "dc", "gen", "", samples, [&]() {
            gdc::PVHSSElg2_KeyGen(param, sk, ekp0, ekp1);
        }));
    }

    rows.push_back(run_row("rlwe", "hss", "setup", "", samples, [&]() {
        rhss::PKE_Para param;
        param.msg_bit = 32;
        param.num_data = msg_num;
        param.d = degree;
        vec_ZZ_pX pkePk, pkeSk, ek0, ek1;
        pkePk.SetLength(2);
        pkeSk.SetLength(2);
        ek0.SetLength(2);
        ek1.SetLength(2);
        rhss::PKE_Gen(param, pkePk, pkeSk);
        rhss::HssGen(ek0, ek1, param, pkeSk);
    }));

    rows.push_back(run_row("rlwe", "vhss", "setup", "", samples, [&]() {
        rvhss::PKE_Para param;
        param.msg_bit = 32;
        param.num_data = msg_num;
        param.d = degree;
        vec_ZZ_pX pkePk, pkeSk;
        pkePk.SetLength(2);
        pkeSk.SetLength(2);
        rvhss::VHSS_Para vhss;
        rvhss::PKE_Gen(param, pkePk, pkeSk);
        ZZ_pXModulus modulus(param.xN);
        rvhss::VHSS_Gen(vhss, param, modulus, pkeSk);
    }));

    rows.push_back(run_row("rlwe", "ot", "setup", "", samples, [&]() {
        rot::PVHSSPara param;
        vec_ZZ_pX pkePk;
        rot::Setup(param, pkePk, msg_num, degree);
    }));

    {
        rot::PVHSSPara param;
        vec_ZZ_pX pkePk;
        rot::Setup(param, pkePk, msg_num, degree);
        ZZ_pXModulus modulus(param.pkePara.xN);
        rot::PVHSS_SK sk;
        bn_t ekp0, ekp1;
        bn_new(ekp0);
        bn_new(ekp1);
        rows.push_back(run_row("rlwe", "ot", "gen", "", samples, [&]() {
            rot::KeyGen(param, sk, modulus, pkePk, ekp0, ekp1);
        }));
    }

    rows.push_back(run_row("rlwe", "dc", "setup", "", samples, [&]() {
        rdc::PVHSSPara param;
        vec_ZZ_pX pkePk;
        rdc::Setup(param, pkePk, msg_num, degree);
    }));

    {
        rdc::PVHSSPara param;
        vec_ZZ_pX pkePk;
        rdc::Setup(param, pkePk, msg_num, degree);
        ZZ_pXModulus modulus(param.pkePara.xN);
        rdc::PVHSS_SK sk;
        bn_t ekp0[2], ekp1[2];
        for (int i = 0; i < 2; ++i)
        {
            bn_new(ekp0[i]);
            bn_new(ekp1[i]);
        }
        rows.push_back(run_row("rlwe", "dc", "gen", "", samples, [&]() {
            rdc::KeyGen(param, sk, modulus, pkePk, ekp0, ekp1);
        }));
    }

    rows.push_back(run_row("rlwe", "cz", "setup", "", samples, [&]() {
        PkeParams params;
        PvhssParams pvhss_params;
        vec_ZZ_pX pk, sk, hss_ek1, hss_ek2, C_alpha;
        pk.SetLength(2);
        sk.SetLength(2);
        hss_ek1.SetLength(2);
        hss_ek2.SetLength(2);
        C_alpha.SetLength(4);
        params.msg_bit = 32;
        params.d = degree;
        params.num_data = msg_num;
        PkeGen(params, pk, sk, 1);
        ZZ_pXModulus modulus(params.xN);
        PvhssGen(hss_ek1, hss_ek2, C_alpha, pvhss_params, params, modulus, pk, sk);
    }));
}

static void add_group_ot_incremental_rows(vector<Row> &rows, int samples, int msg_num, int degree)
{
    namespace got = pvhss::group::ot;

    VhssElgamalPk pk;
    VhssElgamalEk ek0, ek1;
    VhssElgamalGen(pk, ek0, ek1, 1024, 256);

    rows.push_back(run_row("group", "ot", "setup_incremental", "", samples, [&]() {
        got::CK ck;
        got::Ped_ComGen(ck);
        bn_t A;
        bn_new(A);
        ep2_new(ck.g2_A);
        ZZtoBn(A, (ek1[1] - ek0[1]) % pk.N);
        ep2_mul_gen(ck.g2_A, A);
    }));

    got::PVHSSElg1_Para param;
    got::PVHSSElg1_SK sk;
    init_group_ot_params(param, msg_num, degree);
    got::PVHSSElg1_Setup(param, ek0, ek1);
    bn_t ekp0, ekp1;
    bn_new(ekp0);
    bn_new(ekp1);
    got::PVHSSElg1_KeyGen(param, sk, ekp0, ekp1);

    rows.push_back(run_row("group", "ot", "gen_incremental", "", samples, [&]() {
        got::PVHSSElg1_KeyGen(param, sk, ekp0, ekp1);
    }));

    vec_ZZ X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.msg_bits);
    }
    vector<got::PVHSSElg1_CT> Ix;
    got::PVHSSElg1_ProbGen(Ix, param, X);

    VhssElgamalMv base0, base1;
    int prf_key0 = 0;
    int prf_key1 = 0;
    VhssElgamalEvaluatePD2(base0, 0, Ix, param.pk, ek0, prf_key0, param.degree_f);
    VhssElgamalEvaluatePD2(base1, 1, Ix, param.pk, ek1, prf_key1, param.degree_f);

    got::PROOF pi0, pi1;
    rows.push_back(run_row("group", "ot", "eval_incremental", "0", samples, [&]() {
        int prf_key = prf_key0;
        VhssElgamalMv y = base0;
        VhssElgamalMv sk_b;
        VhssElgamalConvertInput(sk_b, 0, param.pk, ek0, param.pk_f, prf_key);
        y[0] += sk_b[0];
        y[2] += sk_b[2];
        got::Ped_Prove(pi0, 0, y[0], y[2], param.ck, prf_key, ekp0);
    }));
    rows.push_back(run_row("group", "ot", "eval_incremental", "1", samples, [&]() {
        int prf_key = prf_key1;
        VhssElgamalMv y = base1;
        VhssElgamalMv sk_b;
        VhssElgamalConvertInput(sk_b, 1, param.pk, ek1, param.pk_f, prf_key);
        y[0] += sk_b[0];
        y[2] += sk_b[2];
        got::Ped_Prove(pi1, 1, y[0], y[2], param.ck, prf_key, ekp1);
    }));

    rows.push_back(run_row("group", "ot", "verify_incremental", "", samples, [&]() {
        volatile bool ok = got::PVHSSElg1_Verify(pi0, pi1, param.ck);
        (void)ok;
    }));
    rows.push_back(run_row("group", "ot", "decode_incremental", "", samples, [&]() {
        ZZ y;
        got::PVHSSElg1_Decode(y, pi0, pi1, sk);
    }));
}

static void add_group_dc_incremental_rows(vector<Row> &rows, int samples, int msg_num, int degree)
{
    namespace gdc = pvhss::group::dc;

    VhssElgamalPk pk;
    VhssElgamalEk ek0, ek1;
    VhssElgamalGen(pk, ek0, ek1, 1024, 256);

    rows.push_back(run_row("group", "dc", "setup_incremental", "", samples, [&]() {
        gdc::CK ck;
        gdc::PVHSSElg2_SK sk;
        gdc::DecPed_ComGen(ck, sk);
        bn_t A;
        bn_new(A);
        g2_t vk;
        ep2_new(vk);
        ZZtoBn(A, (ek1[1] - ek0[1]) % pk.N);
        ep2_mul_gen(vk, A);
    }));

    gdc::PVHSSElg2_Para param;
    gdc::PVHSSElg2_SK sk;
    init_group_dc_params(param, msg_num, degree);
    gdc::PVHSSElg2_Setup(param, ek0, ek1, sk);
    bn_t ekp0[2], ekp1[2];
    for (int i = 0; i < 2; ++i)
    {
        bn_new(ekp0[i]);
        bn_new(ekp1[i]);
    }
    gdc::PVHSSElg2_KeyGen(param, sk, ekp0, ekp1);

    rows.push_back(run_row("group", "dc", "gen_incremental", "", samples, [&]() {
        gdc::PVHSSElg2_KeyGen(param, sk, ekp0, ekp1);
    }));

    vec_ZZ X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.msg_bits);
    }
    vector<gdc::PVHSSElg2_CT> Ix;
    gdc::PVHSSElg2_ProbGen(Ix, param, X);

    VhssElgamalMv base0, base1;
    int prf_key0 = 0;
    int prf_key1 = 0;
    VhssElgamalEvaluatePD2(base0, 0, Ix, param.pk, ek0, prf_key0, param.degree_f);
    VhssElgamalEvaluatePD2(base1, 1, Ix, param.pk, ek1, prf_key1, param.degree_f);

    gdc::PROOF pi0, pi1;
    rows.push_back(run_row("group", "dc", "eval_incremental", "0", samples, [&]() {
        gdc::PVHSSElg2_Prove(pi0, 0, base0[0], base0[2], param, ekp0);
    }));
    rows.push_back(run_row("group", "dc", "eval_incremental", "1", samples, [&]() {
        gdc::PVHSSElg2_Prove(pi1, 1, base1[0], base1[2], param, ekp1);
    }));
    rows.push_back(run_row("group", "dc", "verify_incremental", "", samples, [&]() {
        volatile bool ok = gdc::PVHSSElg2_Verify(pi0, pi1, param);
        (void)ok;
    }));
    rows.push_back(run_row("group", "dc", "decode_incremental", "", samples, [&]() {
        dig_t y;
        gdc::PVHSSElg2_Decode(y, pi0, pi1, sk);
    }));
}

static void add_rlwe_ot_incremental_rows(vector<Row> &rows, int samples, int msg_num, int degree)
{
    namespace rot = pvhss::rlwe::ot;

    rot::PKE_Para pkePara;
    pkePara.msg_bit = 32;
    pkePara.num_data = msg_num;
    pkePara.d = degree;
    vec_ZZ_pX pkePk, pkeSk;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    rot::PKE_Gen(pkePara, pkePk, pkeSk);
    ZZ_pXModulus base_modulus(pkePara.xN);
    rot::VHSS_Para vhssPara;
    rot::VHSS_Gen(vhssPara, pkePara, base_modulus, pkeSk);

    rows.push_back(run_row("rlwe", "ot", "setup_incremental", "", samples, [&]() {
        rot::CK ck;
        rot::Ped_ComGen(ck);
        ZZ A_ZZ = rot::HssOutputCoeff(vhssPara.alpha[0], pkePara, ck.g1_order_ZZ);
        bn_t A;
        bn_new(A);
        ep2_new(ck.g2_A);
        ZZtoBn(A, A_ZZ);
        ep2_mul_gen(ck.g2_A, A);
    }));

    rot::PVHSSPara param;
    vec_ZZ_pX setupPk;
    rot::Setup(param, setupPk, msg_num, degree);
    ZZ_pXModulus modulus(param.pkePara.xN);
    rot::PVHSS_SK sk;
    bn_t ekp0, ekp1;
    bn_new(ekp0);
    bn_new(ekp1);
    rot::KeyGen(param, sk, modulus, setupPk, ekp0, ekp1);

    rows.push_back(run_row("rlwe", "ot", "gen_incremental", "", samples, [&]() {
        rot::KeyGen(param, sk, modulus, setupPk, ekp0, ekp1);
    }));

    vec_ZZ X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.pkePara.msg_bit);
    }
    vector<rot::PVHSS_CT> Ix;
    rot::ProbGen(Ix, param, X, modulus, setupPk);

    rot::PVHSS_CT C1;
    rot::PVHSS_MV M1_0, M1_1, M3_0, M3_1;
    rot::VHSS_Enc(C1, param.pkePara, modulus, setupPk, ZZ(1));
    rot::HssConvertInput(M1_0, param.pkePara, modulus, param.vhssPara.vhssEk_1, C1);
    rot::HssConvertInput(M1_1, param.pkePara, modulus, param.vhssPara.vhssEk_2, C1);
    rot::HssConvertInput(M3_0, param.pkePara, modulus, param.vhssPara.vhssEk_3, C1);
    rot::HssConvertInput(M3_1, param.pkePara, modulus, param.vhssPara.vhssEk_4, C1);

    rot::PVHSS_MV base_y0, base_y1, base_Y0, base_Y1;
    int prf_key0 = 0;
    int prf_key1 = 0;
    rot::HssEvaluatePolyD2(base_y0, 0, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_1,
                           prf_key0, param.pkePara.d, M1_0);
    rot::HssEvaluatePolyD2(base_Y0, 0, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_3,
                           prf_key0, param.pkePara.d, M3_0);
    rot::HssEvaluatePolyD2(base_y1, 1, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_2,
                           prf_key1, param.pkePara.d, M1_1);
    rot::HssEvaluatePolyD2(base_Y1, 1, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_4,
                           prf_key1, param.pkePara.d, M3_1);

    rot::PROOF pi0, pi1;
    rows.push_back(run_row("rlwe", "ot", "eval_incremental", "0", samples, [&]() {
        vec_ZZ_pX y = base_y0;
        vec_ZZ_pX Y = base_Y0;
        vec_ZZ_pX sk_b, SK_b;
        rot::HssConvertInput(sk_b, param.pkePara, modulus, param.vhssPara.vhssEk_1, param.pk_f);
        rot::HssConvertInput(SK_b, param.pkePara, modulus, param.vhssPara.vhssEk_3, param.pk_f);
        rot::HssAddMemory(y, y, sk_b);
        rot::HssAddMemory(Y, Y, SK_b);
        rot::Prove(pi0, 0, y, Y, param, ekp0);
    }));
    rows.push_back(run_row("rlwe", "ot", "eval_incremental", "1", samples, [&]() {
        vec_ZZ_pX y = base_y1;
        vec_ZZ_pX Y = base_Y1;
        vec_ZZ_pX sk_b, SK_b;
        rot::HssConvertInput(sk_b, param.pkePara, modulus, param.vhssPara.vhssEk_2, param.pk_f);
        rot::HssConvertInput(SK_b, param.pkePara, modulus, param.vhssPara.vhssEk_4, param.pk_f);
        rot::HssAddMemory(y, y, sk_b);
        rot::HssAddMemory(Y, Y, SK_b);
        rot::Prove(pi1, 1, y, Y, param, ekp1);
    }));
    rows.push_back(run_row("rlwe", "ot", "verify_incremental", "", samples, [&]() {
        volatile bool ok = rot::Verify(pi0, pi1, param.ck);
        (void)ok;
    }));
    rows.push_back(run_row("rlwe", "ot", "decode_incremental", "", samples, [&]() {
        ZZ y;
        rot::Decode(y, pi0, pi1, sk);
    }));
}

static void add_rlwe_dc_incremental_rows(vector<Row> &rows, int samples, int msg_num, int degree)
{
    namespace rdc = pvhss::rlwe::dc;

    rdc::PKE_Para pkePara;
    pkePara.msg_bit = 1;
    pkePara.num_data = msg_num;
    pkePara.d = degree;
    vec_ZZ_pX pkePk, pkeSk;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    rdc::PKE_Gen(pkePara, pkePk, pkeSk);
    ZZ_pXModulus base_modulus(pkePara.xN);
    rdc::VHSS_Para vhssPara;
    rdc::VHSS_Gen(vhssPara, pkePara, base_modulus, pkeSk);

    rows.push_back(run_row("rlwe", "dc", "setup_incremental", "", samples, [&]() {
        rdc::CK ck;
        rdc::PVHSS_SK sk;
        rdc::DecPed_ComGen(ck, sk);
        ZZ A_ZZ = rdc::HssOutputCoeff(vhssPara.alpha[0], pkePara, ck.g1_order_ZZ);
        bn_t A;
        bn_new(A);
        g2_t vk;
        ep2_new(vk);
        ZZtoBn(A, A_ZZ);
        ep2_mul_gen(vk, A);
    }));

    rdc::PVHSSPara param;
    vec_ZZ_pX setupPk;
    rdc::Setup(param, setupPk, msg_num, degree);
    ZZ_pXModulus modulus(param.pkePara.xN);
    rdc::PVHSS_SK sk;
    bn_t ekp0[2], ekp1[2];
    for (int i = 0; i < 2; ++i)
    {
        bn_new(ekp0[i]);
        bn_new(ekp1[i]);
    }
    rdc::KeyGen(param, sk, modulus, setupPk, ekp0, ekp1);

    rows.push_back(run_row("rlwe", "dc", "gen_incremental", "", samples, [&]() {
        rdc::KeyGen(param, sk, modulus, setupPk, ekp0, ekp1);
    }));

    vec_ZZ X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.pkePara.msg_bit);
    }
    vector<rdc::PVHSS_CT> Ix;
    rdc::ProbGen(Ix, param, X, modulus, setupPk);

    rdc::PVHSS_CT C1;
    rdc::PVHSS_MV M1, M2, M3, M4;
    rdc::VHSS_Enc(C1, param.pkePara, modulus, setupPk, ZZ(1));
    rdc::HssConvertInput(M1, param.pkePara, modulus, param.vhssPara.vhssEk_1, C1);
    rdc::HssConvertInput(M2, param.pkePara, modulus, param.vhssPara.vhssEk_2, C1);
    rdc::HssConvertInput(M3, param.pkePara, modulus, param.vhssPara.vhssEk_3, C1);
    rdc::HssConvertInput(M4, param.pkePara, modulus, param.vhssPara.vhssEk_4, C1);

    rdc::PVHSS_MV base_y0, base_y1, base_Y0, base_Y1;
    int prf_key0 = 0;
    int prf_key1 = 0;
    rdc::HssEvaluatePolyD2(base_y0, 0, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_1,
                           prf_key0, param.pkePara.d, M1);
    rdc::HssEvaluatePolyD2(base_Y0, 0, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_3,
                           prf_key0, param.pkePara.d, M3);
    rdc::HssEvaluatePolyD2(base_y1, 1, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_2,
                           prf_key1, param.pkePara.d, M2);
    rdc::HssEvaluatePolyD2(base_Y1, 1, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_4,
                           prf_key1, param.pkePara.d, M4);

    rdc::PROOF pi0, pi1;
    rows.push_back(run_row("rlwe", "dc", "eval_incremental", "0", samples, [&]() {
        rdc::Prove(pi0, 0, base_y0, base_Y0, param, ekp0);
    }));
    rows.push_back(run_row("rlwe", "dc", "eval_incremental", "1", samples, [&]() {
        rdc::Prove(pi1, 1, base_y1, base_Y1, param, ekp1);
    }));
    rows.push_back(run_row("rlwe", "dc", "verify_incremental", "", samples, [&]() {
        volatile bool ok = rdc::Verify(pi0, pi1, param);
        (void)ok;
    }));
    rows.push_back(run_row("rlwe", "dc", "decode_incremental", "", samples, [&]() {
        dig_t y;
        rdc::Decode(y, pi0, pi1, param.f_sk);
    }));
}

static void add_incremental_rows(vector<Row> &rows, int samples, int msg_num, int degree)
{
    add_group_ot_incremental_rows(rows, samples, msg_num, degree);
    add_group_dc_incremental_rows(rows, samples, msg_num, degree);
    add_rlwe_ot_incremental_rows(rows, samples, msg_num, degree);
    add_rlwe_dc_incremental_rows(rows, samples, msg_num, degree);
}

static void print_usage(const char *program)
{
    cerr << "Usage: " << program
         << " --mode setup-gen|incremental [--samples N] [--msg-num N] [--degree N] [--out PATH]\n";
}
}

int main(int argc, char **argv)
{
    try
    {
        const string mode = read_string_arg(argc, argv, "--mode", "");
        if (mode.empty() || has_arg(argc, argv, "--help"))
        {
            print_usage(argv[0]);
            return mode.empty() ? 2 : 0;
        }

        const int samples = read_int_arg(argc, argv, "--samples", 100);
        const int msg_num = read_int_arg(argc, argv, "--msg-num", 5);
        const int degree = read_int_arg(argc, argv, "--degree", 5);
        const string default_out =
            mode == "setup-gen"
                ? "benchmarks/results/protocols/setup_gen_timing.csv"
                : "benchmarks/results/overhead/incremental_component_timing.csv";
        const string out_path = read_string_arg(argc, argv, "--out", default_out);

        vector<Row> rows;
        if (mode == "setup-gen")
        {
            add_setup_gen_rows(rows, samples, msg_num, degree);
        }
        else if (mode == "incremental")
        {
            add_incremental_rows(rows, samples, msg_num, degree);
        }
        else
        {
            print_usage(argv[0]);
            return 2;
        }

        write_rows(out_path, rows);
        cout << "Wrote " << rows.size() << " rows to " << out_path << "\n";
        return 0;
    }
    catch (const exception &ex)
    {
        cerr << "error: " << ex.what() << "\n";
        return 1;
    }
}
