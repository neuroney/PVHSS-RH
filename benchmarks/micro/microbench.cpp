#include "helper.h"
#include "relic_cp_decped.h"
#include "elgamal.h"
#include "VHSSElg.h"
#include "HSSElg.h"
#include "HSSRLWE.h"
#include "VHSSRLWE.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <unordered_map>

using namespace NTL;
using namespace std;

using BenchFn = std::function<void()>;

namespace group_hss = pvhss::group::hss;
namespace rlwe_hss = pvhss::rlwe::hss;
namespace rlwe_vhss = pvhss::rlwe::vhss;

using HSSGroup_PK = group_hss::HssPublicKey;
using HSSGroup_EK = group_hss::HssEvalKey;
using HSSGroup_CT = group_hss::HssCiphertext;
using HSSGroup_MV = group_hss::HssMemoryValue;

struct BenchConfig
{
    int cheap_samples = 10;
    int cheap_iters = 1;
    int expensive_samples = 1;
    int expensive_iters = 1;
    double min_sample_ms = 25.0;
    int max_adaptive_iters = 10000000;
    bool csv = true;
    bool compact = false;
    bool header = true;
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
    cfg.compact = has_arg(argc, argv, "--compact");
    cfg.header = !has_arg(argc, argv, "--no-header");
    if (cfg.compact)
    {
        cfg.csv = true;
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

static void print_result(const BenchResult &r, const BenchConfig &cfg)
{
    if (cfg.csv)
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

        if (cfg.compact)
        {
            cout << csv_escape(r.category) << ","
                 << csv_escape(r.primitive) << ","
                 << fixed << setprecision(6) << r.mean_ms << "\n";
            return;
        }

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

static uint64_t message_limit_for_bits(unsigned int bits)
{
    if (bits == 0 || bits > 32)
    {
        throw runtime_error("DecPed microbench supports bounded 1..32-bit plaintexts");
    }
    return uint64_t(1) << bits;
}

static uint64_t ceil_sqrt_u64(uint64_t value)
{
    uint64_t root = static_cast<uint64_t>(sqrt(static_cast<long double>(value)));
    while (root * root < value)
    {
        ++root;
    }
    while (root > 0 && (root - 1) * (root - 1) >= value)
    {
        --root;
    }
    return root;
}

static void bn_set_u64(bn_t out, uint64_t value)
{
    ostringstream encoded;
    encoded << value;
    const string text = encoded.str();
    bn_read_str(out, text.c_str(), static_cast<int>(text.size()), 10);
}

static string g1_key(const g1_t point)
{
    if (g1_is_infty(point) == 1)
    {
        return string("INF");
    }

    g1_t normalized;
    g1_null(normalized);
    g1_new(normalized);
    g1_norm(normalized, point);

    const size_t len = g1_size_bin(normalized, 1);
    vector<uint8_t> bytes(len);
    g1_write_bin(bytes.data(), len, normalized, 1);
    g1_free(normalized);

    return string(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

class DecPedBsgsDecoder
{
public:
    DecPedBsgsDecoder(const bgn_t prv, unsigned int bits)
        : limit_(message_limit_for_bits(bits)),
          step_size_(ceil_sqrt_u64(limit_))
    {
        bn_null(order_);
        bn_null(secret_scalar_);
        bn_null(step_bn_);
        g1_null(base_);
        g1_null(giant_step_);

        bn_new(order_);
        bn_new(secret_scalar_);
        bn_new(step_bn_);
        g1_new(base_);
        g1_new(giant_step_);

        pc_get_ord(order_);
        bn_mul(secret_scalar_, prv->x, prv->y);
        bn_sub(secret_scalar_, secret_scalar_, prv->z);
        bn_mod(secret_scalar_, secret_scalar_, order_);
        g1_mul_gen(base_, secret_scalar_);
        g1_norm(base_, base_);

        build_table();
    }

    ~DecPedBsgsDecoder()
    {
        bn_free(order_);
        bn_free(secret_scalar_);
        bn_free(step_bn_);
        g1_free(base_);
        g1_free(giant_step_);
    }

    uint64_t max_message() const
    {
        return limit_ - 1;
    }

    int decode(uint64_t &out, const g1_t in[2], const bgn_t prv) const
    {
        g1_t target;
        g1_t current;
        g1_null(target);
        g1_null(current);
        g1_new(target);
        g1_new(current);

        int result = RLC_ERR;
        g1_mul(target, in[0], prv->x);
        g1_sub(target, target, in[1]);
        g1_norm(target, target);

        if (g1_is_infty(target) == 1)
        {
            out = 0;
            result = RLC_OK;
        }
        else
        {
            g1_copy(current, target);
            const uint64_t giant_steps = (limit_ + step_size_ - 1) / step_size_;
            for (uint64_t j = 0; j <= giant_steps; ++j)
            {
                const unordered_map<string, uint64_t>::const_iterator it = baby_steps_.find(g1_key(current));
                if (it != baby_steps_.end())
                {
                    const uint64_t candidate = j * step_size_ + it->second;
                    if (candidate < limit_)
                    {
                        out = candidate;
                        result = RLC_OK;
                        break;
                    }
                }

                g1_sub(current, current, giant_step_);
                g1_norm(current, current);
            }
        }

        g1_free(target);
        g1_free(current);
        return result;
    }

private:
    void build_table()
    {
        baby_steps_.reserve(static_cast<size_t>(step_size_ * 2));

        g1_t current;
        g1_null(current);
        g1_new(current);
        g1_set_infty(current);

        for (uint64_t i = 0; i < step_size_ && i < limit_; ++i)
        {
            baby_steps_[g1_key(current)] = i;
            g1_add(current, current, base_);
            g1_norm(current, current);
        }

        bn_set_u64(step_bn_, step_size_);
        g1_mul(giant_step_, base_, step_bn_);
        g1_norm(giant_step_, giant_step_);
        g1_free(current);
    }

    uint64_t limit_;
    uint64_t step_size_;
    unordered_map<string, uint64_t> baby_steps_;
    bn_t order_;
    bn_t secret_scalar_;
    bn_t step_bn_;
    g1_t base_;
    g1_t giant_step_;
};

static void bench_decped_decryption_bits(const BenchConfig &cfg, const bgn_t pub,
                                         const bgn_t prv, bn_t rho, unsigned int bits)
{
    DecPedBsgsDecoder decoder(prv, bits);

    g1_t ciphertext[2];
    bn_t message_bn;
    g1_new(ciphertext[0]);
    g1_new(ciphertext[1]);
    bn_new(message_bn);

    const uint64_t message = decoder.max_message();
    bn_set_u64(message_bn, message);
    cp_decped_enc3(ciphertext, rho, message_bn, pub);

    uint64_t decoded = 0;
    if (decoder.decode(decoded, ciphertext, prv) != RLC_OK || decoded != message)
    {
        throw runtime_error("DecPed bounded decryption self-check failed");
    }

    const string label = string("DecPed decryption ") + to_string(bits) + "-bit";
    print_result(run_bench("commitment", label, cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     decoder.decode(decoded, ciphertext, prv);
                 }),
                 cfg);

    bn_free(message_bn);
    g1_free(ciphertext[0]);
    g1_free(ciphertext[1]);
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
                 cfg);

    print_result(run_bench("group", "G1 exponentiation", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     g1_mul(g1_out, g1_base, scalar);
                 }),
                 cfg);

    print_result(run_bench("group", "G2 exponentiation", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     g2_mul(g2_out, g2_base, scalar);
                 }),
                 cfg);

    print_result(run_bench("group", "GT exponentiation", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     fp12_exp(gt_pow_out, gt_out, scalar);
                 }),
                 cfg);
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
                 cfg);

    bgn_t pub;
    bgn_t prv;
    g1_t decped_out[2];
    bgn_new(pub);
    bgn_new(prv);
    g1_new(decped_out[0]);
    g1_new(decped_out[1]);
    cp_decped_gen(pub, prv);

    print_result(run_bench("commitment", "DecPed commitment", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     cp_decped_enc3(decped_out, rho, x, pub);
                 }),
                 cfg);

    bench_decped_decryption_bits(cfg, pub, prv, rho, 8);
    bench_decped_decryption_bits(cfg, pub, prv, rho, 16);
    bench_decped_decryption_bits(cfg, pub, prv, rho, 32);
}

static void bench_prf(const BenchConfig &cfg)
{
    ZZ modulus;
    power(modulus, ZZ(2), 256);
    ZZ out;

    print_result(run_bench("prf", "PrfZZ", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     PrfZZ(out, 7, modulus);
                 }),
                 cfg);

    bn_t out_bn;
    bn_new(out_bn);
    print_result(run_bench("prf", "PrfBn", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     PrfBn(out_bn, 7, modulus);
                 }),
                 cfg);
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

    group_hss::HssGen(pk, ek0, ek1, 1024);
    group_hss::HssInput(ct, pk, ZZ(12345));
    group_hss::HssConvertInput(mx, 0, pk, ek0, ct, prf_key);
    group_hss::HssConvertInput(my, 0, pk, ek0, ct, prf_key);

    print_result(run_bench("hss-group", "HSS AddMemory", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     group_hss::HssAddMemory(mz, pk, mx, my);
                 }),
                 cfg);

    print_result(run_bench("hss-group", "HSS Multiply", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     group_hss::HssMul(mz, 0, pk, ct, mx, prf_key);
                 }),
                 cfg);
}

static void bench_group_vhss(const BenchConfig &cfg)
{
    VhssElgamalPk pk;
    VhssElgamalEk ek0, ek1;
    VhssElgamalVk vk;
    VhssElgamalCt ct;
    VhssElgamalMv mx;
    VhssElgamalMv my;
    VhssElgamalMv mz;
    int prf_key = 0;

    VhssElgamalGen(pk, vk, ek0, ek1, 1024, 256);
    VhssElgamalInput(ct, pk, ZZ(12345));
    VhssElgamalConvertInput(mx, 0, pk, ek0, ct, prf_key);
    VhssElgamalConvertInput(my, 0, pk, ek0, ct, prf_key);

    print_result(run_bench("vhss-group", "VHSS AddMemory", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     VhssElgamalAddMemory(mz, pk, mx, my);
                 }),
                 cfg);

    print_result(run_bench("vhss-group", "VHSS Multiply", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     VhssElgamalMul(mz, 0, pk, ct, mx, prf_key);
                 }),
                 cfg);
}

static void bench_rlwe_hss(const BenchConfig &cfg)
{
    rlwe_hss::PKE_Para pke_para;
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

    rlwe_hss::PKE_Gen(pke_para, pke_pk, pke_sk);
    ZZ_pXModulus modulus(pke_para.xN);
    rlwe_hss::HssGen(hss_ek0, hss_ek1, pke_para, pke_sk);

    vec_ZZ_pX ct;
    vec_ZZ_pX mx;
    vec_ZZ_pX my;
    vec_ZZ_pX mz;
    mx.SetLength(2);
    my.SetLength(2);
    mz.SetLength(2);

    rlwe_hss::HSS_Enc(ct, pke_para, modulus, pke_pk, ZZ(12345));
    rlwe_hss::HSS_Mult(mx, pke_para, modulus, hss_ek0, ct);
    rlwe_hss::HSS_Mult(my, pke_para, modulus, hss_ek0, ct);

    print_result(run_bench("hss-rlwe", "HSS AddMemory", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     rlwe_hss::HssAddMemory(mz, mx, my);
                 }),
                 cfg);

    print_result(run_bench("hss-rlwe", "HSS Enc/OKDM", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     rlwe_hss::HSS_Enc(ct, pke_para, modulus, pke_pk, ZZ(12345));
                 }),
                 cfg);

    print_result(run_bench("hss-rlwe", "HSS Multiply/DDec", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     rlwe_hss::HSS_Mult(mz, pke_para, modulus, hss_ek0, ct);
                 }),
                 cfg);
}

static void bench_rlwe_vhss(const BenchConfig &cfg)
{
    rlwe_vhss::PKE_Para pke_para;
    pke_para.msg_bit = 32;
    pke_para.num_data = 5;
    pke_para.d = 5;

    vec_ZZ_pX pke_pk;
    vec_ZZ_pX pke_sk;
    pke_pk.SetLength(2);
    pke_sk.SetLength(2);

    rlwe_vhss::PKE_Gen(pke_para, pke_pk, pke_sk);
    ZZ_pXModulus modulus(pke_para.xN);

    rlwe_vhss::VHSS_Para vhss_para;
    rlwe_vhss::VHSS_Gen(vhss_para, pke_para, modulus, pke_sk);

    vec_ZZ_pX ct;
    vec_ZZ_pX mx;
    vec_ZZ_pX my;
    vec_ZZ_pX mz;
    mx.SetLength(2);
    my.SetLength(2);
    mz.SetLength(2);

    rlwe_vhss::VHSS_Enc(ct, pke_para, modulus, pke_pk, ZZ(12345));
    rlwe_vhss::HssConvertInput(mx, pke_para, modulus, vhss_para.vhssEk_1, ct);
    rlwe_vhss::HssConvertInput(my, pke_para, modulus, vhss_para.vhssEk_1, ct);

    print_result(run_bench("vhss-rlwe", "VHSS AddMemory", cfg.cheap_samples, cfg.cheap_iters, true, cfg, [&]() {
                     rlwe_vhss::HssAddMemory(mz, mx, my);
                 }),
                 cfg);

    print_result(run_bench("vhss-rlwe", "VHSS Enc/OKDM", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     rlwe_vhss::VHSS_Enc(ct, pke_para, modulus, pke_pk, ZZ(12345));
                 }),
                 cfg);

    print_result(run_bench("vhss-rlwe", "VHSS Multiply/DDec", cfg.expensive_samples, cfg.expensive_iters, false, cfg, [&]() {
                     rlwe_vhss::VHSS_Mult(mz, pke_para, modulus, vhss_para.vhssEk_1, ct);
                 }),
                 cfg);
}

int main(int argc, char **argv)
{
    BenchConfig cfg = parse_config(argc, argv);
    init_relic();

    if (cfg.csv && cfg.header)
    {
        if (cfg.compact)
        {
            cout << "category,operation,mean_ms\n";
        }
        else
        {
            cout << "category,primitive,samples,iterations_per_sample,mode,mean_ms,median_ms,min_ms,rsd_percent\n";
        }
    }

    bench_pairing_primitives(cfg);
    bench_commitments(cfg);
    bench_prf(cfg);
    bench_group_hss(cfg);
    bench_group_vhss(cfg);
    bench_rlwe_hss(cfg);
    bench_rlwe_vhss(cfg);

    core_clean();
    return 0;
}
