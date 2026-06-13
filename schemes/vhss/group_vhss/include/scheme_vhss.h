#pragma once

#include "VHSSElg.h"
#include "group_vhss_backend.h"
#include "rms_program.h"
#include "mpe_program.h"
#include "rms_evaluator.h"
#include "plaintext_mpe.h"
#include "scheme_bench_runner.h"
#include "helper.h"

#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace scheme {

struct SchemeVhss
{
    using Backend = pvhss::backend::GroupVhssBackend;

    struct SetupOutput {
        typename Backend::PublicKey pk;
        typename Backend::VerifKey  vk;
        typename Backend::EvalKey   ek0;
        typename Backend::EvalKey   ek1;
        int degree_f;
    };

    struct ProbGenOutput {
        std::vector<typename Backend::Ciphertext> inputs;
    };

    struct ServerOutput {
        typename Backend::Memory y_share;
    };

    struct VerifyOutput { bool accepted; };

    static bench::BenchCounters counters;

    static SetupOutput Setup(const bench::BenchConfig& cfg)
    {
        SetupOutput pp;
        Backend::GenerateKeys(pp.pk, pp.vk, pp.ek0, pp.ek1, cfg.sk_len, cfg.vk_len);
        pp.degree_f = cfg.degree_f;
        return pp;
    }

    static ProbGenOutput ProbGen(const SetupOutput& pp,
                                 const std::vector<NTL::ZZ>& x)
    {
        ProbGenOutput task;
        for (const auto& xi : x)
            task.inputs.push_back(Backend::EncodeInput(pp.pk, xi));
        return task;
    }

    static ServerOutput Compute(const SetupOutput& pp,
                                const ProbGenOutput& task, int server_id)
    {
        using namespace pvhss::programs;
        int msg_num  = static_cast<int>(task.inputs.size());
        int degree_f = pp.degree_f;
        auto program = BuildMpeRmsProgram(msg_num, degree_f);
        const auto& ek = (server_id == 0) ? pp.ek0 : pp.ek1;
        int prf_key = 0;
        ServerOutput out;
        out.y_share = EvalRmsProgram<Backend>(program, server_id, pp.pk, ek,
                                              task.inputs, prf_key);
        counters.witness_mul_count = msg_num * degree_f;
        counters.total_mul_count   = counters.witness_mul_count;
        return out;
    }

    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&,
                               const ServerOutput& out0, const ServerOutput& out1)
    { return { Backend::Verify(out0.y_share, out1.y_share, pp.vk) }; }

    static NTL::ZZ Decode(const SetupOutput&, const ServerOutput& out0,
                          const ServerOutput& out1)
    { NTL::ZZ r; sub(r, out1.y_share[0], out0.y_share[0]); return r; }

    static bench::BenchCounters GetCounters() { return counters; }
};
inline bench::BenchCounters SchemeVhss::counters;

}} // namespace pvhss::scheme
