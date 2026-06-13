#pragma once

#include "helper.h"
#include "plaintext_mpe.h"
#include "rms_evaluator.h"

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

template <class Scheme, class = void>
struct HasCanDecodeReference : std::false_type {};

template <class Scheme>
struct HasCanDecodeReference<Scheme, std::void_t<decltype(
    Scheme::CanDecodeReference(
        std::declval<const typename Scheme::SetupOutput&>(),
        std::declval<const NTL::ZZ&>()))>> : std::true_type {};

template <class Scheme>
bool CanDecodeReference(const typename Scheme::SetupOutput& pp,
                        const NTL::ZZ& reference)
{
    if constexpr (HasCanDecodeReference<Scheme>::value)
    {
        return Scheme::CanDecodeReference(pp, reference);
    }
    else
    {
        (void)pp;
        (void)reference;
        return true;
    }
}

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

/// Counter set for detailed operation counting.
struct BenchCounters {
    int witness_mul_count        = 0;
    int proof_mul_count          = 0;
    int total_mul_count          = 0;
    int memory_add_count         = 0;
    int memory_scalar_mul_count  = 0;
    int input_lincomb_count      = 0;
    int group_scalar_mul_count   = 0;
    int msm_count                = 0;
    int pairing_count            = 0;

    void print() const
    {
        std::cout << "  witness_mul_count         = " << witness_mul_count        << "\n";
        std::cout << "  proof_mul_count           = " << proof_mul_count          << "\n";
        std::cout << "  total_mul_count           = " << total_mul_count          << "\n";
        std::cout << "  memory_add_count          = " << memory_add_count         << "\n";
        std::cout << "  memory_scalar_mul_count   = " << memory_scalar_mul_count  << "\n";
        std::cout << "  input_lincomb_count       = " << input_lincomb_count      << "\n";
        std::cout << "  group_scalar_mul_count    = " << group_scalar_mul_count   << "\n";
        std::cout << "  msm_count                 = " << msm_count                << "\n";
        std::cout << "  pairing_count             = " << pairing_count            << "\n";
    }
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

/// Generic scheme benchmark runner.
///
/// Scheme must expose:
///   struct SetupOutput;
///   struct ProbGenOutput;
///   struct ServerOutput;
///   struct VerifyOutput;
///   static SetupOutput   Setup(const BenchConfig& cfg);
///   static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x);
///   static ServerOutput  Compute(const SetupOutput& pp, const ProbGenOutput& task, int server_id);
///   static VerifyOutput  Verify(const SetupOutput& pp, const ProbGenOutput& task,
///                               const ServerOutput& out0, const ServerOutput& out1);
///   static NTL::ZZ       Decode(const SetupOutput& pp,
///                               const ServerOutput& out0, const ServerOutput& out1);
///   static BenchCounters GetCounters();
template <class Scheme>
void RunSchemeBench(const BenchConfig& cfg)
{
    using namespace pvhss::programs;

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

    // --- Verify ---
    typename Scheme::VerifyOutput verify_val;
    Measure("Verify", [&]() {
        verify_val = Scheme::Verify(setup_val, task_val, out0_val, out1_val);
    }, cfg.cyctimes);

    // --- Correctness reference ---
    NTL::ZZ reference = PlaintextMpe(x, cfg.degree_f, cfg.modulus);
    const bool can_decode_reference = CanDecodeReference<Scheme>(setup_val, reference);

    // --- Decode ---
    NTL::ZZ decoded;
    Measure("Decode", [&]() {
        decoded = Scheme::Decode(setup_val, out0_val, out1_val);
    }, cfg.cyctimes);

    // --- Correctness check ---
    bool correct = verify_val.accepted && (!can_decode_reference || decoded == reference);
    std::cout << "\n  Correctness: " << (correct ? "PASS" : "FAIL");
    if (correct && !can_decode_reference)
    {
        std::cout << " (relaxed)";
    }
    std::cout << "\n";
    if (!verify_val.accepted)
    {
        std::cout << "  verification rejected\n";
    }
    if (!correct && can_decode_reference)
    {
        std::cout << "  decoded   = " << decoded   << "\n";
        std::cout << "  reference = " << reference << "\n";
    }
    else if (!can_decode_reference)
    {
        std::cout << "  reference = " << reference << "\n";
    }

    // --- Counters ---
    std::cout << "\n  --- Operation Counters ---\n";
    BenchCounters counters = Scheme::GetCounters();
    counters.print();

    std::cout << "=======================================================\n";
    std::cout.flush();
}

}} // namespace pvhss::bench
