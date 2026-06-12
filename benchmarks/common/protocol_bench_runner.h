#pragma once

#include "helper.h"
#include "plaintext_pd2.h"
#include "rms_evaluator.h"

#include <NTL/ZZ.h>
#include <vector>
#include <string>
#include <functional>
#include <cassert>
#include <iostream>
#include <iomanip>

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

/// Generic protocol benchmark runner.
///
/// Protocol must expose:
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
template <class Protocol>
void RunProtocolBench(const BenchConfig& cfg)
{
    using namespace pvhss::programs;

    std::cout << "=======================================================\n";
    std::cout << "  Protocol Benchmark\n";
    std::cout << "  msg_num = " << cfg.msg_num << "  degree_f = " << cfg.degree_f
              << "  cyctimes = " << cfg.cyctimes << "\n";
    std::cout << "-------------------------------------------------------\n";

    // --- Setup ---
    typename Protocol::SetupOutput setup_val;
    Measure("Setup", [&]() {
        setup_val = Protocol::Setup(cfg);
    }, cfg.cyctimes);

    // --- Sample inputs ---
    std::vector<NTL::ZZ> x(cfg.msg_num);
    for (int i = 0; i < cfg.msg_num; ++i)
    {
        NTL::RandomBits(x[i], cfg.msg_bits);
    }

    // --- ProbGen ---
    typename Protocol::ProbGenOutput task_val;
    Measure("ProbGen", [&]() {
        task_val = Protocol::ProbGen(setup_val, x);
    }, cfg.cyctimes);

    // --- Compute server 0 ---
    typename Protocol::ServerOutput out0_val;
    Measure("Compute0", [&]() {
        out0_val = Protocol::Compute(setup_val, task_val, 0);
    }, cfg.cyctimes);

    // --- Compute server 1 ---
    typename Protocol::ServerOutput out1_val;
    Measure("Compute1", [&]() {
        out1_val = Protocol::Compute(setup_val, task_val, 1);
    }, cfg.cyctimes);

    // --- Verify ---
    typename Protocol::VerifyOutput verify_val;
    Measure("Verify", [&]() {
        verify_val = Protocol::Verify(setup_val, task_val, out0_val, out1_val);
    }, std::max(1, cfg.cyctimes / 2));

    // --- Decode ---
    NTL::ZZ decoded;
    Measure("Decode", [&]() {
        decoded = Protocol::Decode(setup_val, out0_val, out1_val);
    }, cfg.cyctimes);

    // --- Correctness check ---
    NTL::ZZ reference = PlaintextPd2(x, cfg.degree_f, cfg.modulus);
    bool correct = (decoded == reference);
    std::cout << "\n  Correctness: " << (correct ? "PASS" : "FAIL") << "\n";
    if (!correct)
    {
        std::cout << "  decoded   = " << decoded   << "\n";
        std::cout << "  reference = " << reference << "\n";
    }

    // --- Counters ---
    std::cout << "\n  --- Operation Counters ---\n";
    BenchCounters counters = Protocol::GetCounters();
    counters.print();

    std::cout << "=======================================================\n";
    std::cout.flush();
}

}} // namespace pvhss::bench
