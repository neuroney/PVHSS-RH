#include "helper.h"
#include "VHSSRLWE.h"

#include <chrono>
#include <cmath>
#include <functional>
#include <iomanip>
#include <string>

using namespace NTL;
using namespace std;

using BenchFn = std::function<void()>;
using Clock = std::chrono::steady_clock;

struct BenchConfig
{
    int cheap_samples = 10;
    int cheap_iters = 1;
    int expensive_samples = 1;
    int expensive_iters = 1;
    double min_sample_ms = 25.0;
    int max_adaptive_iters = 10000000;
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

static void print_result(const BenchResult &r)
{
    cout << r.category << ","
         << r.primitive << ","
         << r.samples << ","
         << r.iterations_per_sample << ","
         << (r.adaptive ? "adaptive" : "fixed") << ","
         << fixed << setprecision(6) << r.mean_ms << ","
         << fixed << setprecision(6) << r.median_ms << ","
         << fixed << setprecision(6) << r.min_ms << ","
         << fixed << setprecision(6) << r.rsd * 100.0 << "\n";
}

static void bench_rlwe_vhss(const BenchConfig &cfg)
{
    PKE_Para pke_para;
    pke_para.msg_bit = 32;
    pke_para.num_data = 5;
    pke_para.d = 5;

    vec_ZZ_pX pke_pk;
    vec_ZZ_pX pke_sk;
    pke_pk.SetLength(2);
    pke_sk.SetLength(2);

    PKE_Gen(pke_para, pke_pk, pke_sk);
    ZZ_pXModulus modulus(pke_para.xN);

    VHSS_Para vhss_para;
    VHSS_Gen(vhss_para, pke_para, modulus, pke_sk);

    vec_ZZ_pX ct;
    vec_ZZ_pX mx;
    vec_ZZ_pX my;
    vec_ZZ_pX mz;
    mx.SetLength(2);
    my.SetLength(2);
    mz.SetLength(2);

    VHSS_Enc(ct, pke_para, modulus, pke_pk, ZZ(12345));
    HssConvertInput(mx, pke_para, modulus, vhss_para.vhssEk_1, ct);
    HssConvertInput(my, pke_para, modulus, vhss_para.vhssEk_1, ct);

    print_result(run_bench("vhss-rlwe", "VHSS AddMemory", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
        HssAddMemory(mz, mx, my);
    }));

    print_result(run_bench("vhss-rlwe", "VHSS Enc/OKDM", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
        VHSS_Enc(ct, pke_para, modulus, pke_pk, ZZ(12345));
    }));

    print_result(run_bench("vhss-rlwe", "VHSS Multiply/DDec", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
        VHSS_Mult(mz, pke_para, modulus, vhss_para.vhssEk_1, ct);
    }));
}

int main(int argc, char **argv)
{
    BenchConfig cfg = parse_config(argc, argv);
    cout << "category,primitive,samples,iterations_per_sample,mode,mean_ms,median_ms,min_ms,rsd_percent\n";
    bench_rlwe_vhss(cfg);
    return 0;
}
