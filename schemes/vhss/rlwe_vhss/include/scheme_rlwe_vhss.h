#pragma once
#include "RLWEVHSS.h"
#include "scheme_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <vector>

static inline void EvalZZX(NTL::ZZ &y, NTL::ZZX f) {
    NTL::ZZ coeff, pow2; pow2 = 1;
    for (int i = 0; i < NTL::deg(f) + 1; i++) {
        NTL::GetCoeff(coeff, f, i); y = y + coeff * pow2; pow2 = pow2 * 2;
    }
}

namespace pvhss { namespace scheme {
struct SchemeRlweVhss {
    static constexpr bool ReportDecodeTime = false;

    struct SetupOutput {
        pvhss::rlwe::common::PKE_Para pkePara; pvhss::rlwe::common::VHSS_Para vhssPara;
        NTL::vec_ZZ_pX pkePk,pkeSk; NTL::ZZ_pXModulus modulus;
        NTL::vec_ZZ_pX M1_0,M1_1,M3_0,M3_1; int degree_f;
    };
    struct ProbGenOutput { std::vector<NTL::vec_ZZ_pX> Ix; };
    struct ServerOutput { NTL::vec_ZZ_pX y_share, Y_share; };
    struct VerifyOutput { bool accepted; };
    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp; pp.pkePara.msg_bit=cfg.msg_bits; pp.pkePara.num_data=cfg.msg_num;
        pp.pkePara.d=cfg.degree_f; pp.degree_f=cfg.degree_f;
        pp.pkePk.SetLength(2);pp.pkeSk.SetLength(2);
        pvhss::rlwe::common::SetParams(pp.pkePara);
        pvhss::rlwe::common::PKE_Gen(pp.pkePara,pp.pkePk,pp.pkeSk);
        pp.modulus=NTL::ZZ_pXModulus(pp.pkePara.xN);
        pvhss::rlwe::common::VHSS_Gen(pp.vhssPara,pp.pkePara,pp.modulus,pp.pkeSk);
        pp.M1_0 = pp.vhssPara.vhssEk_1;
        pp.M1_1 = pp.vhssPara.vhssEk_2;
        pp.M3_0 = pp.vhssPara.vhssEk_3;
        pp.M3_1 = pp.vhssPara.vhssEk_4;
        return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput t; for(auto& xi:x){NTL::ZZ_pX xp;pvhss::rlwe::common::EncodeBinaryPolynomial(xp,xi,pp.pkePara.msg_bit);
        NTL::vec_ZZ_pX ct;ct.SetLength(4);
        pvhss::rlwe::common::VHSS_Enc(ct,pp.pkePara,pp.modulus,pp.pkePk,xp);t.Ix.push_back(ct);} return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput o; int prf=0;
        pvhss::rlwe::common::HssEvaluateMPE(o.y_share,sid,task.Ix,pp.pkePara,pp.modulus,
            (sid==0)?pp.vhssPara.vhssEk_1:pp.vhssPara.vhssEk_2,prf,pp.degree_f,(sid==0)?pp.M1_0:pp.M1_1);
        prf=0;
        pvhss::rlwe::common::HssEvaluateMPE(o.Y_share,sid,task.Ix,pp.pkePara,pp.modulus,
            (sid==0)?pp.vhssPara.vhssEk_3:pp.vhssPara.vhssEk_4,prf,pp.degree_f,(sid==0)?pp.M3_0:pp.M3_1);
        return o;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        return {pvhss::rlwe::common::VHSS_Verify(o0.y_share,o1.y_share,o0.Y_share,o1.Y_share,pp.vhssPara)};
    }
    static NTL::ZZ Decode(const SetupOutput& pp, const ServerOutput& o0, const ServerOutput& o1) {
        (void)pp;
        NTL::ZZ_pX d=o1.y_share[0]-o0.y_share[0];
        NTL::ZZX dzx;NTL::conv(dzx,d);
        NTL::ZZ value(0);EvalZZX(value,dzx);
        return value;
    }
};
}}
