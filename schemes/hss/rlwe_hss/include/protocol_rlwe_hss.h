#pragma once
#include "HSSRLWE.h"
#include "protocol_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace protocol {

struct ProtocolRlweHss
{
    struct SetupOutput {
        pvhss::rlwe::hss::PKE_Para pkePara; NTL::vec_ZZ_pX pkePk, pkeSk, hssEk1, hssEk2;
        NTL::ZZ_pXModulus modulus; NTL::vec_ZZ_pX M1; int degree_f;
    };
    struct ProbGenOutput { std::vector<NTL::vec_ZZ_pX> Ix; };
    struct ServerOutput { NTL::vec_ZZ_pX y_share; };
    struct VerifyOutput { bool accepted; };
    static bench::BenchCounters counters;

    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp; pp.pkePara.msg_bit=cfg.msg_bits; pp.pkePara.num_data=cfg.msg_num;
        pp.pkePara.d=cfg.degree_f; pp.degree_f=cfg.degree_f;
        pp.pkePk.SetLength(2);pp.pkeSk.SetLength(2);pp.hssEk1.SetLength(2);pp.hssEk2.SetLength(2);
        pvhss::rlwe::hss::SetParams(pp.pkePara);
        pvhss::rlwe::hss::PKE_Gen(pp.pkePara,pp.pkePk,pp.pkeSk);
        pp.modulus=NTL::ZZ_pXModulus(pp.pkePara.xN);
        pvhss::rlwe::hss::HssGen(pp.hssEk1,pp.hssEk2,pp.pkePara,pp.pkeSk);
        pp.M1.SetLength(2); NTL::vec_ZZ_pX C1;C1.SetLength(4);
        pvhss::rlwe::hss::PKE_OKDM(C1,pp.pkePara,pp.modulus,pp.pkePk,NTL::ZZ(1));
        pvhss::rlwe::hss::HssConvertInput(pp.M1,pp.pkePara,pp.modulus,pp.hssEk1,C1);
        return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput t; for(auto& xi:x){NTL::vec_ZZ_pX ct;ct.SetLength(4);
        pvhss::rlwe::hss::HSS_Enc(ct,pp.pkePara,pp.modulus,pp.pkePk,xi);t.Ix.push_back(ct);} return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput o; int prf=0;
        pvhss::rlwe::hss::HssEvaluatePolyD2(o.y_share,sid,task.Ix,pp.pkePara,pp.modulus,
            (sid==0)?pp.hssEk1:pp.hssEk2,prf,pp.degree_f,pp.M1);
        counters.witness_mul_count=pp.pkePara.num_data*pp.degree_f;
        counters.total_mul_count=counters.witness_mul_count; return o;
    }
    static VerifyOutput Verify(const SetupOutput&,const ProbGenOutput&,const ServerOutput&,const ServerOutput&){return{true};}
    static NTL::ZZ Decode(const SetupOutput& pp, const ServerOutput& o0, const ServerOutput& o1) {
        NTL::ZZ_pX d=o1.y_share[0]-o0.y_share[0]; return NTL::conv<NTL::ZZ>(NTL::coeff(d,0))%pp.pkePara.q;
    }
    static bench::BenchCounters GetCounters(){return counters;}
};
inline bench::BenchCounters ProtocolRlweHss::counters;
}}
