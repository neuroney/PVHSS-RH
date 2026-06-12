#pragma once
#include "PiOTRLWE.h"
#include "protocol_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <vector>
namespace pvhss { namespace protocol {
struct ProtocolRlweOt {
    struct SetupOutput {
        pvhss::rlwe::ot::PVHSSPara param; NTL::vec_ZZ_pX pkePk; NTL::ZZ_pXModulus modulus;
        NTL::vec_ZZ_pX M1,M2; pvhss::rlwe::ot::PVHSS_SK sk; bn_t ekp0,ekp1;
    };
    struct ProbGenOutput { std::vector<NTL::vec_ZZ_pX> Ix; };
    struct ServerOutput { pvhss::rlwe::ot::PROOF proof; };
    struct VerifyOutput { bool accepted; };
    static bench::BenchCounters counters;

    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp; pp.param.pkePara.msg_bit=cfg.msg_bits;
        pp.param.pkePara.num_data=cfg.msg_num; pp.param.pkePara.d=cfg.degree_f;
        pp.pkePk.SetLength(2); bn_new(pp.ekp0);bn_new(pp.ekp1);
        pvhss::rlwe::ot::Setup(pp.param,pp.pkePk,cfg.msg_num,cfg.degree_f);
        pp.modulus=NTL::ZZ_pXModulus(pp.param.pkePara.xN);
        pvhss::rlwe::ot::KeyGen(pp.param,pp.sk,pp.modulus,pp.pkePk,pp.ekp0,pp.ekp1);
        pp.M1.SetLength(2);pp.M2.SetLength(2);NTL::vec_ZZ_pX C1;C1.SetLength(4);
        pvhss::rlwe::ot::PKE_OKDM(C1,pp.param.pkePara,pp.modulus,pp.pkePk,NTL::ZZ(1));
        pvhss::rlwe::ot::HssConvertInput(pp.M1,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_1,C1);
        pvhss::rlwe::ot::HssConvertInput(pp.M2,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_3,C1);
        return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput t; NTL::vec_ZZ X;X.SetLength(x.size());
        for(size_t i=0;i<x.size();++i)X[i]=x[i];
        pvhss::rlwe::ot::ProbGen(t.Ix,pp.param,X,pp.modulus,pp.pkePk); return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput o; bn_t ekpb;bn_new(ekpb); if(sid==0)bn_copy(ekpb,pp.ekp0);else bn_copy(ekpb,pp.ekp1);
        auto Ix=task.Ix; auto M2=pp.M2; std::vector<std::vector<int>> F;
        pvhss::rlwe::ot::Compute(o.proof,sid,pp.param,pp.param.vhssPara.vhssEk_1,pp.param.vhssPara.vhssEk_2,Ix,pp.modulus,pp.M1,M2,F,ekpb);
        counters.witness_mul_count=pp.param.pkePara.num_data*pp.param.pkePara.d;
        counters.total_mul_count=counters.witness_mul_count+2; counters.pairing_count=2; return o;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        return {pvhss::rlwe::ot::Verify(o0.proof,o1.proof,pp.param.ck)};
    }
    static NTL::ZZ Decode(const SetupOutput& pp, const ServerOutput& o0, const ServerOutput& o1) {
        NTL::ZZ y; pvhss::rlwe::ot::Decode(y,o0.proof,o1.proof,pp.sk); return y;
    }
    static bench::BenchCounters GetCounters(){return counters;}
};
inline bench::BenchCounters ProtocolRlweOt::counters;
}}
