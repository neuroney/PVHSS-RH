#pragma once
#include "VHSSRLWE.h"
#include "scheme_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <vector>

// Convert a decimal ZZ to a binary ZZ_pX polynomial (bit-length = bits).
static inline void DecimalToBinary(NTL::ZZ_pX &out, NTL::ZZ in, int bits) {
    NTL::ZZ rem; NTL::ZZX out_zzx;
    for (int i = 0; i < bits; i++) {
        rem = in % 2; NTL::SetCoeff(out_zzx, i, rem); in = (in - rem) / 2;
    }
    NTL::conv(out, out_zzx);
}
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
        pvhss::rlwe::vhss::PKE_Para pkePara; pvhss::rlwe::vhss::VHSS_Para vhssPara;
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
        pvhss::rlwe::vhss::SetParams(pp.pkePara);
        pvhss::rlwe::vhss::PKE_Gen(pp.pkePara,pp.pkePk,pp.pkeSk);
        pp.modulus=NTL::ZZ_pXModulus(pp.pkePara.xN);
        pvhss::rlwe::vhss::VHSS_Gen(pp.vhssPara,pp.pkePara,pp.modulus,pp.pkeSk);
        pp.M1_0.SetLength(2);pp.M1_1.SetLength(2);pp.M3_0.SetLength(2);pp.M3_1.SetLength(2);
        NTL::vec_ZZ_pX C1;C1.SetLength(4);
        NTL::ZZ_pX one; DecimalToBinary(one,NTL::ZZ(1),1);
        pvhss::rlwe::vhss::PKE_OKDM(C1,pp.pkePara,pp.modulus,pp.pkePk,one);
        pvhss::rlwe::vhss::HssConvertInput(pp.M1_0,pp.pkePara,pp.modulus,pp.vhssPara.vhssEk_1,C1);
        pvhss::rlwe::vhss::HssConvertInput(pp.M1_1,pp.pkePara,pp.modulus,pp.vhssPara.vhssEk_2,C1);
        pvhss::rlwe::vhss::HssConvertInput(pp.M3_0,pp.pkePara,pp.modulus,pp.vhssPara.vhssEk_3,C1);
        pvhss::rlwe::vhss::HssConvertInput(pp.M3_1,pp.pkePara,pp.modulus,pp.vhssPara.vhssEk_4,C1);
        return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput t; for(auto& xi:x){NTL::ZZ_pX xp;DecimalToBinary(xp,xi,pp.pkePara.msg_bit);
        NTL::vec_ZZ_pX ct;ct.SetLength(4);
        pvhss::rlwe::vhss::VHSS_Enc(ct,pp.pkePara,pp.modulus,pp.pkePk,xp);t.Ix.push_back(ct);} return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput o; int prf=0;
        pvhss::rlwe::vhss::HssEvaluateMPE(o.y_share,sid,task.Ix,pp.pkePara,pp.modulus,
            (sid==0)?pp.vhssPara.vhssEk_1:pp.vhssPara.vhssEk_2,prf,pp.degree_f,(sid==0)?pp.M1_0:pp.M1_1);
        prf=0;
        pvhss::rlwe::vhss::HssEvaluateMPE(o.Y_share,sid,task.Ix,pp.pkePara,pp.modulus,
            (sid==0)?pp.vhssPara.vhssEk_3:pp.vhssPara.vhssEk_4,prf,pp.degree_f,(sid==0)?pp.M3_0:pp.M3_1);
        return o;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        return {pvhss::rlwe::vhss::VHSS_Verify(o0.y_share,o1.y_share,o0.Y_share,o1.Y_share,pp.vhssPara)};
    }
    static NTL::ZZ Decode(const SetupOutput& pp, const ServerOutput& o0, const ServerOutput& o1) {
        (void)pp;
        NTL::ZZ_pX d=o0.y_share[0]+o1.y_share[0];
        NTL::ZZX dzx;NTL::conv(dzx,d);
        NTL::ZZ value(0);EvalZZX(value,dzx);
        return value;
    }
};
}}
