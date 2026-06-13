#pragma once

#include "HSSElg.h"
#include "scheme_bench_runner.h"
#include "helper.h"

#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace scheme {

struct SchemeHss
{
    struct SetupOutput {
        pvhss::group::hss::HssPublicKey pk;
        pvhss::group::hss::HssEvalKey ek0;
        pvhss::group::hss::HssEvalKey ek1;
        int degree_f;
    };

    struct ProbGenOutput {
        std::vector<pvhss::group::hss::HssCiphertext> inputs;
    };

    struct ServerOutput {
        pvhss::group::hss::HssMemoryValue y_share;
    };

    struct VerifyOutput { bool accepted; };

    static bench::BenchCounters counters;

    static SetupOutput Setup(const bench::BenchConfig& cfg)
    {
        SetupOutput pp;
        pvhss::group::hss::HssGen(pp.pk, pp.ek0, pp.ek1, cfg.sk_len);
        pp.degree_f = cfg.degree_f;
        return pp;
    }

    static ProbGenOutput ProbGen(const SetupOutput& pp,
                                 const std::vector<NTL::ZZ>& x)
    {
        ProbGenOutput task;
        for (const auto& xi : x) {
            pvhss::group::hss::HssCiphertext ct;
            pvhss::group::hss::HssInput(ct, pp.pk, xi);
            task.inputs.push_back(ct);
        }
        return task;
    }

    static ServerOutput Compute(const SetupOutput& pp,
                                const ProbGenOutput& task, int server_id)
    {
        int msg_num  = static_cast<int>(task.inputs.size());
        int degree_f = pp.degree_f;
        const auto& ek = (server_id == 0) ? pp.ek0 : pp.ek1;
        int prf_key = 0;
        ServerOutput out;
        pvhss::group::hss::HssEvaluateMPE(out.y_share, server_id, task.inputs,
                                          pp.pk, ek, prf_key, degree_f);
        counters.witness_mul_count = msg_num * degree_f;
        counters.total_mul_count   = counters.witness_mul_count;
        return out;
    }

    static VerifyOutput Verify(const SetupOutput&, const ProbGenOutput&,
                               const ServerOutput&, const ServerOutput&)
    { return {true}; }

    static NTL::ZZ Decode(const SetupOutput&, const ServerOutput& out0,
                          const ServerOutput& out1)
    {
        NTL::ZZ r;
        pvhss::group::hss::HssDec(r, out0.y_share, out1.y_share);
        return r;
    }

    static bench::BenchCounters GetCounters() { return counters; }
};
inline bench::BenchCounters SchemeHss::counters;

}} // namespace pvhss::scheme
