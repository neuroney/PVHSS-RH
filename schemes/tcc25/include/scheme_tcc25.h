#pragma once
#include "Tcc25Group.h"
#include "scheme_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace scheme {

struct SchemeTcc25
{
    struct SetupOutput {
        pvhss::group::tcc25::Tcc25Param param;
        pvhss::group::tcc25::Tcc25Server s0, s1;
    };
    struct ProbGenOutput { std::vector<pvhss::group::tcc25::Tcc25Ciphertext> Ix; NTL::vec_ZZ x; };
    struct ServerOutput { pvhss::group::tcc25::Tcc25ProofShare ps; };
    struct VerifyOutput { bool accepted; };

    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp; pp.param.skLen=cfg.sk_len; pp.param.msg_bits=cfg.msg_bits;
        pp.param.degree_f=cfg.degree_f; pp.param.msg_num=cfg.msg_num;
        pvhss::group::tcc25::Tcc25_Setup(pp.param, pp.s0, pp.s1); return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& xv) {
        ProbGenOutput t; t.x.SetLength(xv.size());
        for(size_t i=0;i<xv.size();++i)t.x[i]=xv[i];
        pvhss::group::tcc25::Tcc25_ProbGen(t.Ix, pp.param, t.x); return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput out;
        pvhss::group::tcc25::Tcc25_Compute(out.ps, sid, pp.param, (sid==0)?pp.s0:pp.s1, task.Ix);
        return out;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput& task, const ServerOutput& o0, const ServerOutput& o1) {
        pvhss::group::tcc25::Tcc25Proof proof;
        return {pvhss::group::tcc25::Tcc25_Verify(proof, o0.ps, o1.ps, pp.param, task.x)};
    }
    static NTL::ZZ Decode(const SetupOutput& pp, const ServerOutput& o0, const ServerOutput& o1) {
        NTL::ZZ y; pvhss::group::tcc25::Tcc25_Decode(y, o0.ps, o1.ps, pp.param); return y;
    }
    static NTL::ZZ Modulus(const SetupOutput& pp) { return pp.param.order_ZZ; }
};
}}
