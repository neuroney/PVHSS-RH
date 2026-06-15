#pragma once

#include "helper.h"

#include <NTL/ZZ.h>
#include <vector>
#include <string>
#include <functional>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <type_traits>
#include <utility>

namespace pvhss { namespace bench {

/// Benchmark configuration.
struct BenchConfig {
    int msg_num   = 5;
    int degree_f  = 5;
    int msg_bits  = 32;
    int sk_len    = 1024;
    int vk_len    = 256;
    int cyctimes  = 3;     // timing samples
    NTL::ZZ modulus;       // modulus for plaintext reference
    bool verbose  = false;
    std::string random_seed;
};

struct PhaseTiming {
    std::string label;
    TimingResult timing;
};

struct PhaseTimingSamples {
    std::string label;
    std::vector<double> sample_ms;
};

inline TimingResult TimingFromElapsedMs(double elapsed_ms)
{
    TimingResult result;
    result.samples = 1;
    result.iterations_per_sample = 1;
    result.adaptive = false;
    result.mean_ms = elapsed_ms;
    result.median_ms = elapsed_ms;
    result.min_ms = elapsed_ms;
    result.rsd = 0.0;
    return result;
}

inline TimingResult TimingFromSampleMs(std::vector<double> sample_ms)
{
    if (sample_ms.empty())
    {
        sample_ms.push_back(0.0);
    }

    const int samples = static_cast<int>(sample_ms.size());
    std::sort(sample_ms.begin(), sample_ms.end());

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

    double median = sample_ms[(samples - 1) / 2];
    if (samples % 2 == 0)
    {
        median = (sample_ms[samples / 2 - 1] + sample_ms[samples / 2]) / 2.0;
    }

    TimingResult result;
    result.samples = samples;
    result.iterations_per_sample = 1;
    result.adaptive = false;
    result.mean_ms = mean;
    result.median_ms = median;
    result.min_ms = sample_ms.front();
    result.rsd = (mean == 0.0) ? 0.0 : std::sqrt(variance) / mean;
    return result;
}

template <class F>
TimingResult MeasureOnce(F&& fn)
{
    const double start = SteadyTimeSeconds();
    std::forward<F>(fn)();
    const double end = SteadyTimeSeconds();
    return TimingFromElapsedMs((end - start) * 1000.0);
}

/// Timing helper: measure a callable and return the TimingResult.
template <class F>
TimingResult Measure(const std::string& label, F&& fn, int cyctimes,
                     const std::string& random_domain = "")
{
    const std::string domain = random_domain.empty() ? label : random_domain;
    std::function<void(int)> before_sample;
    if (BenchmarkRandomnessConfigured() && !domain.empty())
    {
        before_sample = [domain](int) {
            SeedBenchmarkRandomness(domain);
        };
    }

    auto result = MeasureTimeMs(std::forward<F>(fn), cyctimes, 1, false,
                                25.0, 10000000, before_sample);
    if (!label.empty())
    {
        PrintTimeMs(label, result);
    }
    return result;
}

template <class Scheme, class = void>
struct HasDecode : std::false_type {};

template <class Scheme>
struct HasDecode<Scheme, std::void_t<decltype(
    Scheme::Decode(
        std::declval<const typename Scheme::SetupOutput&>(),
        std::declval<const typename Scheme::ServerOutput&>(),
        std::declval<const typename Scheme::ServerOutput&>()))>> : std::true_type {};

template <class Scheme, class = void>
struct HasVerify : std::false_type {};

template <class Scheme>
struct HasVerify<Scheme, std::void_t<decltype(
    Scheme::Verify(
        std::declval<const typename Scheme::SetupOutput&>(),
        std::declval<const typename Scheme::ProbGenOutput&>(),
        std::declval<const typename Scheme::ServerOutput&>(),
        std::declval<const typename Scheme::ServerOutput&>()))>> : std::true_type {};

template <class Scheme, class = void>
struct HasCanDecodeReference : std::false_type {};

template <class Scheme>
struct HasCanDecodeReference<Scheme, std::void_t<decltype(
    Scheme::CanDecodeReference(
        std::declval<const typename Scheme::SetupOutput&>(),
        std::declval<const NTL::ZZ&>()))>> : std::true_type {};

template <class Scheme, class = void>
struct HasModulus : std::false_type {};

template <class Scheme>
struct HasModulus<Scheme, std::void_t<decltype(
    Scheme::Modulus(
        std::declval<const typename Scheme::SetupOutput&>()))>> : std::true_type {};

template <class Scheme, class = void>
struct ReportsDecodeTime : std::true_type {};

template <class Scheme>
struct ReportsDecodeTime<Scheme, std::void_t<decltype(Scheme::ReportDecodeTime)>>
    : std::bool_constant<Scheme::ReportDecodeTime> {};

template <class Scheme>
NTL::ZZ GetModulus(const typename Scheme::SetupOutput& pp, const NTL::ZZ& fallback)
{
    if constexpr (HasModulus<Scheme>::value)
        return Scheme::Modulus(pp);
    else
        return fallback;
}

template <class Output, class = void>
struct HasRecordedProfile : std::false_type {};

template <class Output>
struct HasRecordedProfile<Output, std::void_t<decltype(
    std::declval<const Output&>().profile)>> : std::true_type {};

inline void AddProfileSample(std::vector<PhaseTimingSamples>& samples,
                             const PhaseTiming& phase)
{
    for (auto& existing : samples)
    {
        if (existing.label == phase.label)
        {
            existing.sample_ms.push_back(phase.timing.mean_ms);
            return;
        }
    }

    PhaseTimingSamples created;
    created.label = phase.label;
    created.sample_ms.push_back(phase.timing.mean_ms);
    samples.push_back(std::move(created));
}

template <class Output>
void AccumulateRecordedProfileSamples(std::vector<PhaseTimingSamples>& samples,
                                      const Output& output)
{
    if constexpr (HasRecordedProfile<Output>::value)
    {
        for (const auto& phase : output.profile)
        {
            AddProfileSample(samples, phase);
        }
    }
}

inline void PrintProfileSampleMeans(const std::vector<PhaseTimingSamples>& samples)
{
    for (const auto& phase : samples)
    {
        PrintTimeMs(phase.label, TimingFromSampleMs(phase.sample_ms));
    }
}

template <class Output, class F>
TimingResult MeasureWithRecordedProfile(const std::string& label, Output& output,
                                        F&& fn, int cyctimes,
                                        const std::string& random_domain = "")
{
    const std::string domain = random_domain.empty() ? label : random_domain;
    if (cyctimes < 1)
    {
        cyctimes = 1;
    }

    std::vector<double> sample_ms(cyctimes);
    std::vector<PhaseTimingSamples> profile_samples;

    for (int i = 0; i < cyctimes; ++i)
    {
        if (BenchmarkRandomnessConfigured() && !domain.empty())
        {
            SeedBenchmarkRandomness(domain);
        }

        const double start = SteadyTimeSeconds();
        std::forward<F>(fn)();
        const double end = SteadyTimeSeconds();

        sample_ms[i] = (end - start) * 1000.0;
        AccumulateRecordedProfileSamples(profile_samples, output);
    }

    auto result = TimingFromSampleMs(sample_ms);
    if (!label.empty())
    {
        PrintTimeMs(label, result);
    }
    PrintProfileSampleMeans(profile_samples);
    return result;
}

/// Generic scheme benchmark runner.
///
/// Scheme must expose:
///   struct SetupOutput;
///   struct ProbGenOutput;
///   struct ServerOutput;
///   static SetupOutput   Setup(const BenchConfig& cfg);
///   static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x);
///   static ServerOutput  Compute(const SetupOutput& pp, const ProbGenOutput& task, int server_id);
/// Optionally:
///   static NTL::ZZ       Decode(const SetupOutput& pp,
///                               const ServerOutput& out0, const ServerOutput& out1);
template <class Scheme>
void RunSchemeBench(const BenchConfig& cfg)
{
    const char *env_seed = std::getenv("PVHSS_BENCH_SEED");
    const std::string configured_seed =
        !cfg.random_seed.empty() ? cfg.random_seed : (env_seed ? std::string(env_seed) : std::string());
    ConfigureBenchmarkRandomness(configured_seed);

    std::cout << "=======================================================\n";
    std::cout << "  Scheme Benchmark\n";
    std::cout << "  msg_num = " << cfg.msg_num << "  degree_f = " << cfg.degree_f
              << "  cyctimes = " << cfg.cyctimes << "\n";
    if (BenchmarkRandomnessConfigured())
    {
        std::cout << "  random_seed = " << configured_seed << "\n";
    }
    std::cout << "-------------------------------------------------------\n";

    // --- Setup ---
    typename Scheme::SetupOutput setup_val;
    MeasureWithRecordedProfile("Setup", setup_val, [&]() {
        setup_val = Scheme::Setup(cfg);
    }, cfg.cyctimes, "Setup");

    // --- Sample inputs ---
    SeedBenchmarkRandomness("Inputs");
    std::vector<NTL::ZZ> x(cfg.msg_num);
    for (int i = 0; i < cfg.msg_num; ++i)
    {
        NTL::RandomBits(x[i], cfg.msg_bits);
    }

    // --- ProbGen ---
    typename Scheme::ProbGenOutput task_val;
    Measure("ProbGen", [&]() {
        task_val = Scheme::ProbGen(setup_val, x);
    }, cfg.cyctimes, "ProbGen");

    // --- Compute server 0 ---
    typename Scheme::ServerOutput out0_val;
    MeasureWithRecordedProfile("Compute0", out0_val, [&]() {
        out0_val = Scheme::Compute(setup_val, task_val, 0);
    }, cfg.cyctimes, "Compute0");

    // --- Compute server 1 ---
    typename Scheme::ServerOutput out1_val;
    MeasureWithRecordedProfile("Compute1", out1_val, [&]() {
        out1_val = Scheme::Compute(setup_val, task_val, 1);
    }, cfg.cyctimes, "Compute1");

    // --- Verify (if scheme provides) ---
    if constexpr (HasVerify<Scheme>::value)
    {
        typename Scheme::VerifyOutput verify_val;
        Measure("Verify", [&]() {
            verify_val = Scheme::Verify(setup_val, task_val, out0_val, out1_val);
        }, cfg.cyctimes, "Verify");
        std::cout << "\n  Verification: " << (verify_val.accepted ? "PASS" : "FAIL") << "\n";
    }

    // --- Correctness check (if scheme provides Decode and can compare) ---
    if constexpr (HasDecode<Scheme>::value)
    {
        NTL::ZZ mod = GetModulus<Scheme>(setup_val, cfg.modulus);
        NTL::ZZ reference = MPE(x, cfg.degree_f, mod);

        bool can_compare = true;
        if constexpr (HasCanDecodeReference<Scheme>::value)
        {
            can_compare = Scheme::CanDecodeReference(setup_val, reference);
        }

        NTL::ZZ decoded;
        if constexpr (ReportsDecodeTime<Scheme>::value)
        {
            Measure("Decode", [&]() {
                decoded = Scheme::Decode(setup_val, out0_val, out1_val);
            }, cfg.cyctimes, "Decode");
        }
        else
        {
            decoded = Scheme::Decode(setup_val, out0_val, out1_val);
        }

        // Normalise both sides modulo the scheme's modulus
        if (mod > 0)
        {
            decoded %= mod;
            if (decoded < 0) decoded += mod;
        }

        if (can_compare)
        {
            bool correct = (decoded == reference);
            std::cout << "  Correctness: " << (correct ? "PASS" : "FAIL") << "\n";
            if (!correct)
            {
                std::cout << "  decoded   = " << decoded   << "\n";
                std::cout << "  reference = " << reference << "\n";
            }
        }
    }

    std::cout << "=======================================================\n";
    std::cout.flush();
}

}} // namespace pvhss::bench
