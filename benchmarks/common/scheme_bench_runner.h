#pragma once

#include "helper.h"

#include <NTL/ZZ.h>
#include <vector>
#include <string>
#include <functional>
#include <cassert>
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
};

/// Timing helper: measure a callable and return the TimingResult.
template <class F>
TimingResult Measure(const std::string& label, F&& fn, int cyctimes)
{
    auto result = MeasureTimeMs(std::forward<F>(fn), cyctimes);
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

template <class Scheme>
NTL::ZZ GetModulus(const typename Scheme::SetupOutput& pp, const NTL::ZZ& fallback)
{
    if constexpr (HasModulus<Scheme>::value)
        return Scheme::Modulus(pp);
    else
        return fallback;
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
    std::cout << "=======================================================\n";
    std::cout << "  Scheme Benchmark\n";
    std::cout << "  msg_num = " << cfg.msg_num << "  degree_f = " << cfg.degree_f
              << "  cyctimes = " << cfg.cyctimes << "\n";
    std::cout << "-------------------------------------------------------\n";

    // --- Setup ---
    typename Scheme::SetupOutput setup_val;
    Measure("Setup", [&]() {
        setup_val = Scheme::Setup(cfg);
    }, cfg.cyctimes);

    // --- Sample inputs ---
    std::vector<NTL::ZZ> x(cfg.msg_num);
    for (int i = 0; i < cfg.msg_num; ++i)
    {
        NTL::RandomBits(x[i], cfg.msg_bits);
    }

    // --- ProbGen ---
    typename Scheme::ProbGenOutput task_val;
    Measure("ProbGen", [&]() {
        task_val = Scheme::ProbGen(setup_val, x);
    }, cfg.cyctimes);

    // --- Compute server 0 ---
    typename Scheme::ServerOutput out0_val;
    Measure("Compute0", [&]() {
        out0_val = Scheme::Compute(setup_val, task_val, 0);
    }, cfg.cyctimes);

    // --- Compute server 1 ---
    typename Scheme::ServerOutput out1_val;
    Measure("Compute1", [&]() {
        out1_val = Scheme::Compute(setup_val, task_val, 1);
    }, cfg.cyctimes);

    // --- Verify (if scheme provides) ---
    if constexpr (HasVerify<Scheme>::value)
    {
        typename Scheme::VerifyOutput verify_val;
        Measure("Verify", [&]() {
            verify_val = Scheme::Verify(setup_val, task_val, out0_val, out1_val);
        }, cfg.cyctimes);
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
        Measure("Decode", [&]() {
            decoded = Scheme::Decode(setup_val, out0_val, out1_val);
        }, cfg.cyctimes);

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
