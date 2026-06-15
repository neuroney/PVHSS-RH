#pragma once
#include "PiDCRLWE.h"
#include "scheme_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <vector>

static inline void DecimalToBinary(NTL::ZZ_pX &out, NTL::ZZ in, int bits) {
    NTL::ZZ rem; NTL::ZZX out_zzx;
    for (int i = 0; i < bits; i++) {
        rem = in % 2; NTL::SetCoeff(out_zzx, i, rem); in = (in - rem) / 2;
    }
    NTL::conv(out, out_zzx);
}

namespace pvhss { namespace scheme {
struct SchemeRlweDc {
    struct SetupOutput {
        pvhss::rlwe::dc::PVHSSPara param; NTL::vec_ZZ_pX pkePk; NTL::ZZ_pXModulus modulus;
        NTL::vec_ZZ_pX M1_0,M1_1,M3_0,M3_1; pvhss::rlwe::dc::PVHSS_SK sk; bn_t ekp0_0,ekp0_1,ekp1_0,ekp1_1;
    };
    struct ProbGenOutput { std::vector<NTL::vec_ZZ_pX> Ix; };
    struct ServerOutput { pvhss::rlwe::dc::PROOF proof; };
    struct VerifyOutput { bool accepted; };

    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp; pp.param.pkePara.msg_bit=cfg.msg_bits;
        pp.param.pkePara.num_data=cfg.msg_num; pp.param.pkePara.d=cfg.degree_f;
        pp.pkePk.SetLength(2); bn_new(pp.ekp0_0);bn_new(pp.ekp0_1);bn_new(pp.ekp1_0);bn_new(pp.ekp1_1);
        pvhss::rlwe::dc::Setup(pp.param,pp.pkePk,cfg.msg_num,cfg.degree_f);
        pp.modulus=NTL::ZZ_pXModulus(pp.param.pkePara.xN);
        bn_t ekp0[2],ekp1[2]; bn_new(ekp0[0]);bn_new(ekp0[1]);bn_new(ekp1[0]);bn_new(ekp1[1]);
        pvhss::rlwe::dc::KeyGen(pp.param,pp.sk,pp.modulus,pp.pkePk,ekp0,ekp1);
        bn_copy(pp.ekp0_0,ekp0[0]);bn_copy(pp.ekp0_1,ekp0[1]);bn_copy(pp.ekp1_0,ekp1[0]);bn_copy(pp.ekp1_1,ekp1[1]);
        pp.M1_0.SetLength(2);pp.M1_1.SetLength(2);pp.M3_0.SetLength(2);pp.M3_1.SetLength(2);
        NTL::vec_ZZ_pX C1;C1.SetLength(4);
        NTL::ZZ_pX one; DecimalToBinary(one,NTL::ZZ(1),1);
        pvhss::rlwe::dc::PKE_OKDM(C1,pp.param.pkePara,pp.modulus,pp.pkePk,one);
        pvhss::rlwe::dc::HssConvertInput(pp.M1_0,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_1,C1);
        pvhss::rlwe::dc::HssConvertInput(pp.M1_1,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_2,C1);
        pvhss::rlwe::dc::HssConvertInput(pp.M3_0,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_3,C1);
        pvhss::rlwe::dc::HssConvertInput(pp.M3_1,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_4,C1);
        return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput t; NTL::vec_ZZ_pX Xp;Xp.SetLength(x.size());
        for(size_t i=0;i<x.size();++i)DecimalToBinary(Xp[i],x[i],pp.param.pkePara.msg_bit);
        pvhss::rlwe::dc::ProbGen(t.Ix,pp.param,Xp,pp.modulus,pp.pkePk); return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput o; bn_t ekpb[2];bn_new(ekpb[0]);bn_new(ekpb[1]);
        if(sid==0){bn_copy(ekpb[0],pp.ekp0_0);bn_copy(ekpb[1],pp.ekp0_1);}
        else{bn_copy(ekpb[0],pp.ekp1_0);bn_copy(ekpb[1],pp.ekp1_1);}
        auto Ix=task.Ix; auto M3=(sid==0)?pp.M3_0:pp.M3_1; std::vector<std::vector<int>> F;
        pvhss::rlwe::dc::Compute(o.proof,sid,pp.param,pp.param.vhssPara.vhssEk_1,pp.param.vhssPara.vhssEk_2,Ix,pp.modulus,(sid==0)?pp.M1_0:pp.M1_1,M3,F,ekpb);
        return o;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        return {pvhss::rlwe::dc::Verify(o0.proof,o1.proof,pp.param)};
    }
    static NTL::ZZ Decode(const SetupOutput& pp, const ServerOutput& o0, const ServerOutput& o1) {
        dig_t y; pvhss::rlwe::dc::Decode(y,o0.proof,o1.proof,pp.param.f_sk); return NTL::conv<NTL::ZZ>((long)y);
    }
    static bool CanDecodeReference(const SetupOutput&, const NTL::ZZ& reference) {
        return reference >= 0 && reference < NTL::conv<NTL::ZZ>((unsigned long)PVHSS_M_MAX);
    }
};
}}
