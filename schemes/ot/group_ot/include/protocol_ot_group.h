#pragma once
#include "PiOTGroup.h"
#include "protocol_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace protocol {

struct ProtocolOtGroup
{
    struct SetupOutput {
        pvhss::group::ot::PVHSSElg1_Para param;
        VhssElgamalEk ek0, ek1;
        pvhss::group::ot::PVHSSElg1_SK sk;
        bn_t ekp0, ekp1;
    };
    struct ProbGenOutput { std::vector<VhssElgamalCt> Ix; };
    struct ServerOutput { pvhss::group::ot::PROOF proof; };
    struct VerifyOutput { bool accepted; };
    static bench::BenchCounters counters;

    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp;
        pp.param.skLen=cfg.sk_len; pp.param.vkLen=cfg.vk_len;
        pp.param.msg_bits=cfg.msg_bits; pp.param.degree_f=cfg.degree_f; pp.param.msg_num=cfg.msg_num;
        pvhss::group::ot::PVHSSElg1_Setup(pp.param, pp.ek0, pp.ek1);
        bn_new(pp.ekp0); bn_new(pp.ekp1);
        pvhss::group::ot::PVHSSElg1_KeyGen(pp.param, pp.sk, pp.ekp0, pp.ekp1);
        return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput task; NTL::vec_ZZ X; X.SetLength(x.size());
        for(size_t i=0;i<x.size();++i) X[i]=x[i];
        pvhss::group::ot::PVHSSElg1_ProbGen(task.Ix, pp.param, X);
        return task;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput out;
        bn_t ekpb; bn_new(ekpb);
        if(sid==0) bn_copy(ekpb, pp.ekp0); else bn_copy(ekpb, pp.ekp1);
        auto Ix_copy = task.Ix; std::vector<std::vector<int>> F_TEST;
        pvhss::group::ot::PVHSSElg1_Compute(out.proof, sid, pp.param, (sid==0)?pp.ek0:pp.ek1, Ix_copy, F_TEST, ekpb);
        counters.witness_mul_count=pp.param.msg_num*pp.param.degree_f;
        counters.proof_mul_count=2; counters.total_mul_count=counters.witness_mul_count+2;
        counters.msm_count=2; counters.pairing_count=2;
        return out;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        return {pvhss::group::ot::PVHSSElg1_Verify(o0.proof, o1.proof, pp.param.ck)};
    }
    static NTL::ZZ Decode(const SetupOutput& pp, const ServerOutput& o0, const ServerOutput& o1) {
        NTL::ZZ y; pvhss::group::ot::PVHSSElg1_Decode(y, o0.proof, o1.proof, pp.sk);
        return y % pp.param.ck.g1_order_ZZ;
    }
    static bench::BenchCounters GetCounters() { return counters; }
};
inline bench::BenchCounters ProtocolOtGroup::counters;
}} 
