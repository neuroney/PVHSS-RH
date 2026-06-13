#pragma once
#include "VHSSRLWE.h"
#include "scheme_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace scheme {
struct SchemeRlweVhss {
    struct SetupOutput {
        pvhss::rlwe::vhss::PKE_Para pkePara; pvhss::rlwe::vhss::VHSS_Para vhssPara;
        NTL::vec_ZZ_pX pkePk,pkeSk; NTL::ZZ_pXModulus modulus;
        NTL::vec_ZZ_pX M1_0,M1_1,M3_0,M3_1; int degree_f;
    };
    struct ProbGenOutput { std::vector<NTL::vec_ZZ_pX> Ix; };
    struct ServerOutput { NTL::vec_ZZ_pX y_share, Y_share; };
    struct VerifyOutput { bool accepted; };
    static bench::BenchCounters counters;

    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp; pp.pkePara.msg_bit=cfg.msg_bits; pp.pkePara.num_data=cfg.msg_num;
        pp.pkePara.d=cfg.degree_f; pp.degree_f=cfg.degree_f;
        pp.pkePk.SetLength(2);pp.pkeSk.SetLength(2);
        pvhss::rlwe::vhss::SetParams(pp.pkePara);
        pvhss::rlwe::vhss::PKE_Gen(pp.pkePara,pp.pkePk,pp.pkeSk);
        pp.modulus=NTL::ZZ_pXModulus(pp.pkePara.xN);
        pvhss::rlwe::vhss::VHSS_Gen(pp.vhssPara,pp.pkePara,pp.modulus,pp.pkeSk);
        pp.M1_0.SetLength(2);pp.M1_1.SetLength(2);pp.M3_0.SetLength(2);pp.M3_1.SetLength(2);
        NTL::vec_ZZ_pX C1;C1.SetLength(4);
        pvhss::rlwe::vhss::PKE_OKDM(C1,pp.pkePara,pp.modulus,pp.pkePk,NTL::ZZ(1));
        pvhss::rlwe::vhss::HssConvertInput(pp.M1_0,pp.pkePara,pp.modulus,pp.vhssPara.vhssEk_1,C1);
        pvhss::rlwe::vhss::HssConvertInput(pp.M1_1,pp.pkePara,pp.modulus,pp.vhssPara.vhssEk_2,C1);
        pvhss::rlwe::vhss::HssConvertInput(pp.M3_0,pp.pkePara,pp.modulus,pp.vhssPara.vhssEk_3,C1);
        pvhss::rlwe::vhss::HssConvertInput(pp.M3_1,pp.pkePara,pp.modulus,pp.vhssPara.vhssEk_4,C1);
        return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput t; for(auto& xi:x){NTL::vec_ZZ_pX ct;ct.SetLength(4);
        pvhss::rlwe::vhss::VHSS_Enc(ct,pp.pkePara,pp.modulus,pp.pkePk,xi);t.Ix.push_back(ct);} return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput o; int prf=0;
        pvhss::rlwe::vhss::HssEvaluatePolyD2(o.y_share,sid,task.Ix,pp.pkePara,pp.modulus,
            (sid==0)?pp.vhssPara.vhssEk_1:pp.vhssPara.vhssEk_2,prf,pp.degree_f,(sid==0)?pp.M1_0:pp.M1_1);
        prf=0;
        pvhss::rlwe::vhss::HssEvaluatePolyD2(o.Y_share,sid,task.Ix,pp.pkePara,pp.modulus,
            (sid==0)?pp.vhssPara.vhssEk_3:pp.vhssPara.vhssEk_4,prf,pp.degree_f,(sid==0)?pp.M3_0:pp.M3_1);
        counters.witness_mul_count=pp.pkePara.num_data*pp.degree_f;
        counters.total_mul_count=counters.witness_mul_count*2; return o;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        return {pvhss::rlwe::vhss::VHSS_Verify(o0.y_share,o1.y_share,o0.Y_share,o1.Y_share,pp.vhssPara)};
    }
    static NTL::ZZ Decode(const SetupOutput& pp, const ServerOutput& o0, const ServerOutput& o1) {
        NTL::ZZ_pX d=o0.y_share[0]+o1.y_share[0];
        NTL::ZZ value=NTL::conv<NTL::ZZ>(NTL::coeff(d,0));
        if(value>pp.pkePara.q/2)value-=pp.pkePara.q;
        return value;
    }
    static bench::BenchCounters GetCounters(){return counters;}
};
inline bench::BenchCounters SchemeRlweVhss::counters;
}}
