#pragma once

#include "VHSSElg.h"
#include "scheme_bench_runner.h"
#include "helper.h"

#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace scheme {

struct SchemeVhss
{
    struct SetupOutput {
        VhssElgamalPk pk;
        VhssElgamalVk vk;
        VhssElgamalEk ek0;
        VhssElgamalEk ek1;
        int degree_f;
    };

    struct ProbGenOutput {
        std::vector<VhssElgamalCt> inputs;
    };

    struct ServerOutput {
        VhssElgamalMv y_share;
    };

    struct VerifyOutput { bool accepted; };

    static SetupOutput Setup(const bench::BenchConfig& cfg)
    {
        SetupOutput pp;
        VhssElgamalGen(pp.pk, pp.vk, pp.ek0, pp.ek1, cfg.sk_len, cfg.vk_len);
        pp.degree_f = cfg.degree_f;
        return pp;
    }

    static ProbGenOutput ProbGen(const SetupOutput& pp,
                                 const std::vector<NTL::ZZ>& x)
    {
        ProbGenOutput task;
        for (const auto& xi : x) {
            VhssElgamalCt ct;
            VhssElgamalInput(ct, pp.pk, xi);
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
        VhssElgamalEvaluateMPE(out.y_share, server_id, task.inputs, pp.pk, ek,
                               prf_key, degree_f);
        return out;
    }

    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&,
                               const ServerOutput& out0, const ServerOutput& out1)
    { return { VhssElgamalVerify(out0.y_share, out1.y_share, pp.vk) }; }

    static NTL::ZZ Decode(const SetupOutput&, const ServerOutput& out0,
                          const ServerOutput& out1)
    { NTL::ZZ r; sub(r, out1.y_share[0], out0.y_share[0]); return r; }

};

}} // namespace pvhss::scheme
