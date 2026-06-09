#include "helper.h"
#include "relic_cp_decped.h"
#include "elgamal.h"
#include "VHSSElg.h"
#include "HSSRLWE.h"

#include <chrono>
#include <functional>
#include <iomanip>
#include <string>

using BenchFn = std::function<void()>;

using HSSGroup_PK = Elgamal_PK;
using HSSGroup_EK = ZZ;
using HSSGroup_CT = array<Elgamal_CT, 2>;
using HSSGroup_MV = array<ZZ, 2>;

void HSS_Gen(HSSGroup_PK &pk, HSSGroup_EK &ek0, HSSGroup_EK &ek1, int skLen);
void HSS_Input(HSSGroup_CT &I, const HSSGroup_PK &pk, const ZZ &x);
void HSS_ConvertInput(HSSGroup_MV &Mx, int idx, const HSSGroup_PK &pk, const HSSGroup_EK &ek, const HSSGroup_CT &Ix, int &prf_key);
void HSS_Mul(HSSGroup_MV &Mz, int idx, const HSSGroup_PK &pk, const HSSGroup_CT &Ix, const HSSGroup_MV &My, int &prf_key);
void HSS_AddMemory(HSSGroup_MV &Mz, const HSSGroup_PK &pk, const HSSGroup_MV &Mx, const HSSGroup_MV &My);

struct BenchConfig
{
    int cheap_samples = 10;
    int cheap_iters = 1;
    int expensive_samples = 1;
    int expensive_iters = 1;
    double min_sample_ms = 25.0;
    int max_adaptive_iters = 10000000;
    bool csv = true;
};

struct BenchResult
{
    string category;
    string primitive;
    int samples;
    int iterations_per_sample;
    bool adaptive;
    double mean_ms;
    double median_ms;
    double min_ms;
    double rsd;
};

using Clock = std::chrono::steady_clock;

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

static BenchConfig parse_config(int argc, char **argv)
{
    BenchConfig cfg;
    cfg.cheap_samples = read_int_arg(argc, argv, "--cheap-samples", cfg.cheap_samples);
    cfg.cheap_iters = read_int_arg(argc, argv, "--cheap-iters", cfg.cheap_iters);
    cfg.expensive_samples = read_int_arg(argc, argv, "--expensive-samples", cfg.expensive_samples);
    cfg.expensive_iters = read_int_arg(argc, argv, "--expensive-iters", cfg.expensive_iters);
    cfg.max_adaptive_iters = read_int_arg(argc, argv, "--max-adaptive-iters", cfg.max_adaptive_iters);
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (argv[i] == string("--min-sample-ms"))
        {
            cfg.min_sample_ms = atof(argv[i + 1]);
        }
    }
    cfg.csv = !has_arg(argc, argv, "--pretty");
    return cfg;
}

static BenchResult run_bench(const string &category, const string &primitive,
                             int samples, int iterations_per_sample,
                             bool adaptive, const BenchConfig &cfg,
                             const BenchFn &fn)
{
    if (samples < 1)
    {
        samples = 1;
    }
    if (iterations_per_sample < 1)
    {
        iterations_per_sample = 1;
    }

    vector<double> sample_ms(samples);
    int measured_iters = iterations_per_sample;
    for (int i = 0; i < samples; ++i)
    {
        int iters = iterations_per_sample;
        double elapsed_ms = 0.0;

        do
        {
            const Clock::time_point start = Clock::now();
            for (int j = 0; j < iters; ++j)
            {
                fn();
            }
            const Clock::time_point end = Clock::now();
            elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();

            if (!adaptive || elapsed_ms >= cfg.min_sample_ms || iters >= cfg.max_adaptive_iters)
            {
                break;
            }

            double scale = cfg.min_sample_ms / (elapsed_ms > 0.0 ? elapsed_ms : 0.001);
            if (scale < 2.0)
            {
                scale = 2.0;
            }
            if (scale > 16.0)
            {
                scale = 16.0;
            }
            long long next_iters = static_cast<long long>(iters * scale) + 1;
            if (next_iters <= iters)
            {
                next_iters = static_cast<long long>(iters) + 1;
            }
            if (next_iters > cfg.max_adaptive_iters)
            {
                next_iters = cfg.max_adaptive_iters;
            }
            iters = static_cast<int>(next_iters);
        } while (true);

        measured_iters = iters;
        sample_ms[i] = elapsed_ms / iters;
    }

    double mean = 0.0;
    for (int i = 0; i < samples; ++i)
    {
        mean += sample_ms[i];
    }
    mean /= samples;

    double variance = 0.0;
    for (int i = 0; i < samples; ++i)
    {
        const double diff = sample_ms[i] - mean;
        variance += diff * diff;
    }
    variance /= samples;

    vector<double> sorted = sample_ms;
    sort(sorted.begin(), sorted.end());
    double median = sorted[samples / 2];
    if (samples % 2 == 0)
    {
        median = (sorted[samples / 2 - 1] + sorted[samples / 2]) / 2.0;
    }

    BenchResult result;
    result.category = category;
    result.primitive = primitive;
    result.samples = samples;
    result.iterations_per_sample = measured_iters;
    result.adaptive = adaptive;
    result.mean_ms = mean;
    result.median_ms = median;
    result.min_ms = sorted.front();
    result.rsd = (mean == 0.0) ? 0.0 : sqrt(variance) / mean;
    return result;
}

static void print_result(const BenchResult &r, bool csv)
{
    if (csv)
    {
        auto csv_escape = [](const string &value) {
            string escaped;
            bool needs_quotes = false;
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
        };

        cout << csv_escape(r.category) << ","
             << csv_escape(r.primitive) << ","
             << r.samples << ","
             << r.iterations_per_sample << ","
             << (r.adaptive ? "adaptive" : "fixed") << ","
             << fixed << setprecision(6) << r.mean_ms << ","
             << fixed << setprecision(6) << r.median_ms << ","
             << fixed << setprecision(6) << r.min_ms << ","
             << fixed << setprecision(6) << r.rsd * 100.0 << "\n";
    }
    else
    {
        cout << left << setw(14) << r.category
             << setw(24) << r.primitive
             << "samples=" << setw(4) << r.samples
             << "iters=" << setw(6) << r.iterations_per_sample
             << setw(10) << (r.adaptive ? "adaptive" : "fixed")
             << "mean=" << fixed << setprecision(6) << setw(12) << r.mean_ms << " ms "
             << "median=" << fixed << setprecision(6) << setw(12) << r.median_ms << " ms "
             << "min=" << fixed << setprecision(6) << setw(12) << r.min_ms << " ms "
             << "RSD=" << fixed << setprecision(4) << r.rsd * 100.0 << "%\n";
    }
}

static void init_relic()
{
    core_init();
    ep_curve_init();
    ep_param_set(B12_P381);
    ep2_curve_init();
    ep2_curve_set_twist(RLC_EP_MTYPE);
}

static void bench_pairing_primitives(const BenchConfig &cfg)
{
    g1_t g1_base;
    g2_t g2_base;
    gt_t gt_out;
    fp12_t gt_pow_out;
    bn_t scalar;
    g1_t g1_out;
    g2_t g2_out;

    g1_new(g1_base);
    g2_new(g2_base);
    gt_new(gt_out);
    fp12_new(gt_pow_out);
    bn_new(scalar);
    g1_new(g1_out);
    g2_new(g2_out);

    g1_get_gen(g1_base);
    g2_get_gen(g2_base);
    pp_map_oatep_k12(gt_out, g1_base, g2_base);
    pc_get_ord(scalar);
    bn_rand_mod(scalar, scalar);

    print_result(run_bench("pairing", "e(G1,G2)", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     pp_map_oatep_k12(gt_out, g1_base, g2_base);
                 }),
                 cfg.csv);

    print_result(run_bench("group", "G1 exponentiation", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     g1_mul(g1_out, g1_base, scalar);
                 }),
                 cfg.csv);

    print_result(run_bench("group", "G2 exponentiation", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     g2_mul(g2_out, g2_base, scalar);
                 }),
                 cfg.csv);

    print_result(run_bench("group", "GT exponentiation", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     fp12_exp(gt_pow_out, gt_out, scalar);
                 }),
                 cfg.csv);
}

static void bench_commitments(const BenchConfig &cfg)
{
    bn_t order;
    bn_t x;
    bn_t rho;
    g1_t g;
    g1_t h;
    g1_t t1;
    g1_t t2;
    g1_t ped_out;

    bn_new(order);
    bn_new(x);
    bn_new(rho);
    g1_new(g);
    g1_new(h);
    g1_new(t1);
    g1_new(t2);
    g1_new(ped_out);

    pc_get_ord(order);
    bn_rand_mod(x, order);
    bn_rand_mod(rho, order);
    g1_get_gen(g);
    g1_mul_gen(h, rho);

    print_result(run_bench("commitment", "Pedersen commitment", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     g1_mul(t1, h, rho);
                     g1_mul(t2, g, x);
                     g1_add(ped_out, t1, t2);
                     g1_norm(ped_out, ped_out);
                 }),
                 cfg.csv);

    bgn_t pub;
    bgn_t prv;
    g1_t decped_out[2];
    dig_t decoded = 0;
    bgn_new(pub);
    bgn_new(prv);
    g1_new(decped_out[0]);
    g1_new(decped_out[1]);
    cp_decped_gen(pub, prv);

    print_result(run_bench("commitment", "DecPed commitment", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     cp_decped_enc3(decped_out, rho, x, pub);
                 }),
                 cfg.csv);

    cp_decped_enc3(decped_out, rho, x, pub);
    print_result(run_bench("commitment", "DecPed decryption", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     cp_decped_dec1(decoded, decped_out, prv);
                 }),
                 cfg.csv);
}

static void bench_prf(const BenchConfig &cfg)
{
    ZZ modulus;
    power(modulus, ZZ(2), 256);
    ZZ out;

    print_result(run_bench("prf", "PRF_ZZ", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     PRF_ZZ(out, 7, modulus);
                 }),
                 cfg.csv);

    bn_t out_bn;
    bn_new(out_bn);
    print_result(run_bench("prf", "PRF_bn", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     PRF_bn(out_bn, 7, modulus);
                 }),
                 cfg.csv);
}

static void bench_group_hss(const BenchConfig &cfg)
{
    HSSGroup_PK pk;
    HSSGroup_EK ek0, ek1;
    HSSGroup_CT ct;
    HSSGroup_MV mx;
    HSSGroup_MV my;
    HSSGroup_MV mz;
    int prf_key = 0;

    HSS_Gen(pk, ek0, ek1, 1024);
    HSS_Input(ct, pk, ZZ(12345));
    HSS_ConvertInput(mx, 0, pk, ek0, ct, prf_key);
    HSS_ConvertInput(my, 0, pk, ek0, ct, prf_key);

    print_result(run_bench("hss-group", "HSS AddMemory", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     HSS_AddMemory(mz, pk, mx, my);
                 }),
                 cfg.csv);

    print_result(run_bench("hss-group", "HSS Multiply", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     HSS_Mul(mz, 0, pk, ct, mx, prf_key);
                 }),
                 cfg.csv);
}

static void bench_group_vhss(const BenchConfig &cfg)
{
    VHSSElg_PK pk;
    VHSSElg_EK ek0, ek1;
    VHSSElg_VK vk;
    VHSSElg_CT ct;
    VHSSElg_MV mx;
    VHSSElg_MV my;
    VHSSElg_MV mz;
    int prf_key = 0;

    VHSSElg_Gen(pk, vk, ek0, ek1, 1024, 256);
    VHSSElg_Input(ct, pk, ZZ(12345));
    VHSSElg_ConvertInput(mx, 0, pk, ek0, ct, prf_key);
    VHSSElg_ConvertInput(my, 0, pk, ek0, ct, prf_key);

    print_result(run_bench("vhss-group", "VHSS AddMemory", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     VHSSElg_AddMemory(mz, pk, mx, my);
                 }),
                 cfg.csv);

    print_result(run_bench("vhss-group", "VHSS Multiply", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     VHSSElg_Mul(mz, 0, pk, ct, mx, prf_key);
                 }),
                 cfg.csv);
}

static void bench_rlwe_hss(const BenchConfig &cfg)
{
    PKE_Para pke_para;
    pke_para.msg_bit = 32;
    pke_para.num_data = 5;
    pke_para.d = 5;

    vec_ZZ_pX pke_pk;
    vec_ZZ_pX pke_sk;
    vec_ZZ_pX hss_ek0;
    vec_ZZ_pX hss_ek1;
    pke_pk.SetLength(2);
    pke_sk.SetLength(2);
    hss_ek0.SetLength(2);
    hss_ek1.SetLength(2);

    PKE_Gen(pke_para, pke_pk, pke_sk);
    ZZ_pXModulus modulus(pke_para.xN);
    HSS_Gen(hss_ek0, hss_ek1, pke_para, pke_sk);

    vec_ZZ_pX ct;
    vec_ZZ_pX mx;
    vec_ZZ_pX my;
    vec_ZZ_pX mz;
    mx.SetLength(2);
    my.SetLength(2);
    mz.SetLength(2);

    HSS_Enc(ct, pke_para, modulus, pke_pk, ZZ(12345));
    HSS_Mult(mx, pke_para, modulus, hss_ek0, ct);
    HSS_Mult(my, pke_para, modulus, hss_ek0, ct);

    print_result(run_bench("hss-rlwe", "HSS AddMemory", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     HSS_AddMemory(mz, mx, my);
                 }),
                 cfg.csv);

    print_result(run_bench("hss-rlwe", "HSS Enc/OKDM", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     HSS_Enc(ct, pke_para, modulus, pke_pk, ZZ(12345));
                 }),
                 cfg.csv);

    print_result(run_bench("hss-rlwe", "HSS Multiply/DDec", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     HSS_Mult(mz, pke_para, modulus, hss_ek0, ct);
                 }),
                 cfg.csv);
}

int main(int argc, char **argv)
{
    BenchConfig cfg = parse_config(argc, argv);
    init_relic();

    if (cfg.csv)
    {
        cout << "category,primitive,samples,iterations_per_sample,mode,mean_ms,median_ms,min_ms,rsd_percent\n";
    }

    bench_pairing_primitives(cfg);
    bench_commitments(cfg);
    bench_prf(cfg);
    bench_group_hss(cfg);
    bench_group_vhss(cfg);
    bench_rlwe_hss(cfg);

    core_clean();
    return 0;
}
