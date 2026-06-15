#pragma once
#include "HSSRLWE.h"
#include "scheme_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <vector>

// Convert a decimal ZZ to a binary ZZ_pX polynomial (bit-length = bits).
static inline void DecimalToBinary(NTL::ZZ_pX &out, NTL::ZZ in, int bits) {
    NTL::ZZ rem;
    NTL::ZZX out_zzx;
    for (int i = 0; i < bits; i++) {
        rem = in % 2;
        NTL::SetCoeff(out_zzx, i, rem);
        in = (in - rem) / 2;
    }
    NTL::conv(out, out_zzx);
}

// Evaluate a ZZX polynomial at x = 2 (Horner-style).
static inline void EvalZZX(NTL::ZZ &y, NTL::ZZX f) {
    NTL::ZZ coeff, pow2;
    pow2 = 1;
    for (int i = 0; i < NTL::deg(f) + 1; i++) {
        NTL::GetCoeff(coeff, f, i);
        y = y + coeff * pow2;
        pow2 = pow2 * 2;
    }
}

namespace pvhss { namespace scheme {

struct SchemeRlweHss
{
    static constexpr bool ReportDecodeTime = false;

    struct SetupOutput {
        pvhss::rlwe::hss::PKE_Para pkePara; NTL::vec_ZZ_pX pkePk, pkeSk, hssEk1, hssEk2;
        NTL::ZZ_pXModulus modulus; NTL::vec_ZZ_pX M1_0, M1_1; int degree_f;
    };
    struct ProbGenOutput { std::vector<NTL::vec_ZZ_pX> Ix; };
    struct ServerOutput { NTL::vec_ZZ_pX y_share; };

    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp; pp.pkePara.msg_bit=cfg.msg_bits; pp.pkePara.num_data=cfg.msg_num;
        pp.pkePara.d=cfg.degree_f; pp.degree_f=cfg.degree_f;
        pp.pkePk.SetLength(2);pp.pkeSk.SetLength(2);pp.hssEk1.SetLength(2);pp.hssEk2.SetLength(2);
        pvhss::rlwe::hss::SetParams(pp.pkePara);
        pvhss::rlwe::hss::PKE_Gen(pp.pkePara,pp.pkePk,pp.pkeSk);
        pp.modulus=NTL::ZZ_pXModulus(pp.pkePara.xN);
        pvhss::rlwe::hss::HssGen(pp.hssEk1,pp.hssEk2,pp.pkePara,pp.pkeSk);
        pp.M1_0.SetLength(2); pp.M1_1.SetLength(2); NTL::vec_ZZ_pX C1;C1.SetLength(4);
        NTL::ZZ_pX one; DecimalToBinary(one,NTL::ZZ(1),1);
        pvhss::rlwe::hss::PKE_OKDM(C1,pp.pkePara,pp.modulus,pp.pkePk,one);
        pvhss::rlwe::hss::HssConvertInput(pp.M1_0,pp.pkePara,pp.modulus,pp.hssEk1,C1);
        pvhss::rlwe::hss::HssConvertInput(pp.M1_1,pp.pkePara,pp.modulus,pp.hssEk2,C1);
        return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput t; for(auto& xi:x){NTL::ZZ_pX xp;DecimalToBinary(xp,xi,pp.pkePara.msg_bit);
        NTL::vec_ZZ_pX ct;ct.SetLength(4);
        pvhss::rlwe::hss::HSS_Enc(ct,pp.pkePara,pp.modulus,pp.pkePk,xp);t.Ix.push_back(ct);} return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput o; int prf=0;
        pvhss::rlwe::hss::HssEvaluateMPE(o.y_share,sid,task.Ix,pp.pkePara,pp.modulus,
            (sid==0)?pp.hssEk1:pp.hssEk2,prf,pp.degree_f,(sid==0)?pp.M1_0:pp.M1_1);
        return o;
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
