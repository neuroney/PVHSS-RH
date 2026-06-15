#include "helper.h"
#include <chrono>
#include <cmath>
#include <cstring>

using namespace NTL;
using namespace std;

namespace
{
RandomStream &PRFStream()
{
    // Existing callers pass only a counter, so keep a fixed domain key and avoid global RNG reseeding.
    static const unsigned char key[32] = {
        0x50, 0x56, 0x48, 0x53, 0x53, 0x2d, 0x52, 0x48,
        0x20, 0x50, 0x52, 0x46, 0x20, 0x76, 0x31, 0x00,
        0x83, 0x9b, 0x61, 0x4f, 0xd7, 0x2c, 0x0e, 0x5a,
        0xa1, 0x37, 0xc4, 0x98, 0x6e, 0x10, 0xf2, 0xbb};
    thread_local RandomStream stream(key);
    return stream;
}

void PRFBoundedZZ(NTL::ZZ &res, int prfkey, const NTL::ZZ &mmod)
{
    if (mmod <= 1)
    {
        clear(res);
        return;
    }

    const long nbits = NumBits(mmod - 1);
    const long nbytes = (nbits + 7) / 8;
    const long excess_bits = nbytes * 8 - nbits;
    const unsigned char top_mask =
        (excess_bits == 0) ? 0xff : static_cast<unsigned char>((1u << (8 - excess_bits)) - 1u);

    thread_local vector<unsigned char> buf;
    buf.resize(nbytes);

    RandomStream &stream = PRFStream();
    stream.set_nonce(static_cast<unsigned long>(static_cast<uint32_t>(prfkey)));

    do
    {
        stream.get(buf.data(), nbytes);
        if (excess_bits != 0)
        {
            buf[nbytes - 1] &= top_mask;
        }
        ZZFromBytes(res, buf.data(), nbytes);
    } while (res >= mmod);
}

bool g_bench_seed_configured = false;
std::string g_bench_seed;
std::array<uint8_t, RLC_RAND_SEED> g_relic_seed = {};

uint64_t Fnv1a64(const std::string &text)
{
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : text)
    {
        h ^= static_cast<uint64_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

uint64_t SplitMix64(uint64_t &x)
{
    uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void FillSeedBytes(unsigned char *out, size_t size, const std::string &seed,
                   const std::string &domain)
{
    uint64_t state = Fnv1a64(seed);
    state ^= Fnv1a64(domain) + 0x9e3779b97f4a7c15ULL + (state << 6) + (state >> 2);

    size_t offset = 0;
    while (offset < size)
    {
        const uint64_t word = SplitMix64(state);
        const size_t chunk = std::min<size_t>(sizeof(word), size - offset);
        std::memcpy(out + offset, &word, chunk);
        offset += chunk;
    }
}
}

void ZZtoBn(bn_t out, const NTL::ZZ &in)
{
    bn_null(out);
    bn_new(out);
    if (in == 0)
    {
        bn_read_str(out, "0", 1, 10);
    }
    else
    {
        std::stringstream buffer;
        buffer << in;
        const std::string ss(buffer.str());
        bn_read_str(out, ss.c_str(), static_cast<int>(ss.length()), 10);
    }
}

void BnToZZ(NTL::ZZ &out, const bn_t in)
{
    int size = bn_size_str(in, 10);
    char *in_char = new char[size];
    bn_write_str(in_char, size, in, 10);
    out = NTL::conv<NTL::ZZ>(in_char);
    delete[] in_char;
}

double SteadyTimeSeconds()
{
    using Clock = std::chrono::steady_clock;
    static const Clock::time_point base = Clock::now();
    const Clock::time_point now = Clock::now();
    return std::chrono::duration<double>(now - base).count();
}

TimingResult MeasureTimeMs(const std::function<void()> &fn, int samples,
                           int iterations_per_sample,
                           bool adaptive,
                           double min_sample_ms,
                           int max_adaptive_iters,
                           const std::function<void(int)> &before_sample)
{
    using Clock = std::chrono::steady_clock;

    if (samples < 1)
    {
        samples = 1;
    }
    if (iterations_per_sample < 1)
    {
        iterations_per_sample = 1;
    }
    if (max_adaptive_iters < iterations_per_sample)
    {
        max_adaptive_iters = iterations_per_sample;
    }

    vector<double> sample_ms(samples);
    int measured_iters = iterations_per_sample;

    for (int i = 0; i < samples; ++i)
    {
        int iters = iterations_per_sample;
        double elapsed_ms = 0.0;

        if (before_sample)
        {
            before_sample(i);
        }

        while (true)
        {
            const Clock::time_point start = Clock::now();
            for (int j = 0; j < iters; ++j)
            {
                fn();
            }
            const Clock::time_point end = Clock::now();
            elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();

            if (!adaptive || elapsed_ms >= min_sample_ms || iters >= max_adaptive_iters)
            {
                break;
            }

            double scale = min_sample_ms / (elapsed_ms > 0.0 ? elapsed_ms : 0.001);
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
            if (next_iters > max_adaptive_iters)
            {
                next_iters = max_adaptive_iters;
            }
            iters = static_cast<int>(next_iters);
        }

        measured_iters = iters;
        sample_ms[i] = elapsed_ms / iters;
    }

    vector<double> sorted = sample_ms;
    sort(sorted.begin(), sorted.end());

    // Trim one min and one max when we have enough samples.
    int start = 0;
    int end   = samples;
    if (samples >= 3)
    {
        ++start;
        --end;
    }
    const int trimmed_count = end - start;

    double mean = 0.0;
    for (int i = start; i < end; ++i)
    {
        mean += sorted[i];
    }
    mean /= trimmed_count;

    double variance = 0.0;
    for (int i = start; i < end; ++i)
    {
        const double diff = sorted[i] - mean;
        variance += diff * diff;
    }
    variance /= trimmed_count;

    double median = sorted[(samples - 1) / 2];
    if (samples % 2 == 0)
    {
        median = (sorted[samples / 2 - 1] + sorted[samples / 2]) / 2.0;
    }

    TimingResult result;
    result.samples = trimmed_count;
    result.iterations_per_sample = measured_iters;
    result.adaptive = adaptive;
    result.mean_ms = mean;
    result.median_ms = median;
    result.min_ms = sorted.front();
    result.rsd = (mean == 0.0) ? 0.0 : sqrt(variance) / mean;
    return result;
}

TimingResult MeasureTimeMsAdaptive(const std::function<void()> &fn, int samples,
                                   double min_sample_ms,
                                   int max_adaptive_iters)
{
    return MeasureTimeMs(fn, samples, 1, true, min_sample_ms, max_adaptive_iters);
}

void PrintTimeMs(const std::string &label, const TimingResult &result)
{
    std::cout << label << ": " << result.mean_ms << " ms  RSD: " << result.rsd * 100 << "%";
    if (result.adaptive)
    {
        std::cout << "  iters/sample: " << result.iterations_per_sample;
    }
    std::cout << "\n";
}

void ConfigureBenchmarkRandomness(const std::string &seed)
{
    g_bench_seed = seed;
    g_bench_seed_configured = !seed.empty();
}

bool BenchmarkRandomnessConfigured()
{
    return g_bench_seed_configured;
}

void SeedBenchmarkRandomness(const std::string &domain)
{
    if (!g_bench_seed_configured)
    {
        return;
    }

    std::array<unsigned char, 64> seed_bytes = {};
    FillSeedBytes(seed_bytes.data(), seed_bytes.size(), g_bench_seed, domain);

    NTL::SetSeed(seed_bytes.data(), static_cast<long>(seed_bytes.size()));

    unsigned int c_seed = 0;
    std::memcpy(&c_seed, seed_bytes.data(), sizeof(c_seed));
    std::srand(c_seed);

    for (size_t i = 0; i < g_relic_seed.size(); ++i)
    {
        g_relic_seed[i] = seed_bytes[i % seed_bytes.size()];
    }
}

void ReseedRelicBenchmarkRandomness()
{
    if (!g_bench_seed_configured)
    {
        return;
    }

    rand_seed(g_relic_seed.data(), g_relic_seed.size());
}


NTL::ZZ PrfZZ(int prf_key, const NTL::ZZ &mmod)
{
    NTL::ZZ res;
    PRFBoundedZZ(res, prf_key, mmod);
    return res;
}

void PrfZZ(NTL::ZZ& res, int prf_key, const NTL::ZZ& mmod)
{
    PRFBoundedZZ(res, prf_key, mmod);
}

void PrfBn(bn_t res, int prf_key, const NTL::ZZ &mmod)
{
    NTL::ZZ tmp;
    tmp = PrfZZ(prf_key, mmod);
    bn_null(res);
    bn_new(res);
    ZZtoBn(res, tmp);
}

void GenerateRandomFunc(std::vector<std::vector<int>> &F_TEST, int msg_num, int degree_f)
{
    std::vector<std::vector<int>> Combinations;
    std::vector<int> tmp;

    for (int i = 1; i <= degree_f; ++i)
    {
        Combinations.clear();
        tmp.clear();
        int len = 0;
        GeneratePartitions(len, Combinations, i, msg_num, tmp, 0);
        if (!BenchmarkRandomnessConfigured())
        {
            srand(static_cast<unsigned>(time(0)));
        }
        tmp = Combinations[rand() % len];
        F_TEST.push_back(tmp);
    }
}

void GeneratePartitions(int &cnt, std::vector<std::vector<int>> &Res, int sum, int k,
                        std::vector<int> &lst, int minn)
{
    if (k == 0)
    {
        if (sum == 0)
        {
            Res.push_back(lst);
            ++cnt;
        }
        return;
    }
    for (int i = minn; i < sum + 1; ++i)
    {
        lst.push_back(i);
        GeneratePartitions(cnt, Res, sum - i, k - 1, lst, i);
        lst.pop_back();
    }
}

void RandomZZpx(ZZ_pX &a, int N, int q_bit) {
    ZZ_p coeff;
    for (int i = 0; i < N; i++) {
        conv(coeff, NTL::RandomBits_ZZ(q_bit));
        SetCoeff(a, i, coeff);
    }
}

void RlweSecretKey(ZZ_pX &sk, int N, int hsk) {
    int interval = 0;
    interval = N / hsk;
    int index = rand() % interval;
    for (int i = 0; i < hsk; i++) {
        SetCoeff(sk, index, 1);
        index = index + rand() % interval;
    }

}

void GaussRandom(ZZ_pX &e, int N) {
    double res_standard;
    int deviation = 8;
    int res;
    for (int i = 0; i < N; i++) {
        res_standard = sqrt(-2.0 * log(rand() / (RAND_MAX + 1.0))) * sin(2.0 * PVHSS_PI * rand() / (RAND_MAX + 1.0));
        res = res_standard * deviation;
        SetCoeff(e, i, res);
    }
}

void ZZpxScaleMul(ZZ_pX &out, const ZZ_pX &in1, const ZZ &in2)
{
    ZZ_pX a;
    conv(a,in2);
    out = in1 * a;
}

ZZ MPE(const vector<ZZ> &x, int degree_f, const ZZ &modulus) {
    vector<ZZ> h(degree_f + 1, ZZ(0));
    h[0] = ZZ(1);

    const bool has_modulus = (modulus > 0);
    for (const ZZ &xi : x) {
        for (int s = 1; s <= degree_f; ++s) {
            ZZ prod;
            if (has_modulus) {
                MulMod(prod, xi, h[s - 1], modulus);
                AddMod(h[s], h[s], prod, modulus);
            } else {
                mul(prod, xi, h[s - 1]);
                add(h[s], h[s], prod);
            }
        }
    }

    ZZ result(0);
    for (int s = 1; s <= degree_f; ++s) {
        if (has_modulus) {
            AddMod(result, result, h[s], modulus);
        } else {
            add(result, result, h[s]);
        }
    }
    return result;
}

ZZ MPE(const vec_ZZ &x, int degree_f) {
    vector<ZZ> values;
    values.reserve(x.length());
    for (long i = 0; i < x.length(); ++i) {
        values.push_back(x[i]);
    }
    return MPE(values, degree_f, ZZ(0));
}
