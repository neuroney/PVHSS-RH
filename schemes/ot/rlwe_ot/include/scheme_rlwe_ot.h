#pragma once
#include "PiOTRLWE.h"
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
struct SchemeRlweOt {
    struct SetupOutput {
        pvhss::rlwe::ot::PVHSSPara param; NTL::vec_ZZ_pX pkePk; NTL::ZZ_pXModulus modulus;
        NTL::vec_ZZ_pX M1_0,M1_1,M3_0,M3_1; pvhss::rlwe::ot::PVHSS_SK sk; bn_t ekp0,ekp1;
        std::vector<bench::PhaseTiming> profile;
    };
    struct ProbGenOutput { std::vector<NTL::vec_ZZ_pX> Ix; };
    struct ServerOutput {
        pvhss::rlwe::ot::PROOF proof;
        std::vector<bench::PhaseTiming> profile;
    };
    struct VerifyOutput { bool accepted; };

    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp;
        auto vhss = bench::MeasureOnce([&]() {
            pp.param.pkePara.msg_bit=cfg.msg_bits;
            pp.param.pkePara.num_data=cfg.msg_num; pp.param.pkePara.d=cfg.degree_f;
            NTL::vec_ZZ_pX pkeSk;
            pp.pkePk.SetLength(2); pkeSk.SetLength(2);
            pvhss::rlwe::ot::PKE_Gen(pp.param.pkePara, pp.pkePk, pkeSk);
            pp.modulus=NTL::ZZ_pXModulus(pp.param.pkePara.xN);
            pvhss::rlwe::ot::VHSS_Gen(pp.param.vhssPara, pp.param.pkePara,
                                      pp.modulus, pkeSk);
            pp.M1_0.SetLength(2);pp.M1_1.SetLength(2);pp.M3_0.SetLength(2);pp.M3_1.SetLength(2);
            NTL::vec_ZZ_pX C1;C1.SetLength(4);
            NTL::ZZ_pX one; DecimalToBinary(one,NTL::ZZ(1),1);
            pvhss::rlwe::ot::PKE_OKDM(C1,pp.param.pkePara,pp.modulus,pp.pkePk,one);
            pvhss::rlwe::ot::HssConvertInput(pp.M1_0,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_1,C1);
            pvhss::rlwe::ot::HssConvertInput(pp.M1_1,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_2,C1);
            pvhss::rlwe::ot::HssConvertInput(pp.M3_0,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_3,C1);
            pvhss::rlwe::ot::HssConvertInput(pp.M3_1,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_4,C1);
        });
        auto extra = bench::MeasureOnce([&]() {
            pvhss::rlwe::ot::Ped_ComGen(pp.param.ck);
            const NTL::ZZ A_ZZ = pvhss::rlwe::ot::HssOutputPolyAtTwo(
                pp.param.vhssPara.alpha, pp.param.pkePara, pp.param.ck.g1_order_ZZ);
            bn_t A;
            bn_new(A);
            ep2_new(pp.param.ck.g2_A);
            ZZtoBn(A, A_ZZ);
            ep2_mul_gen(pp.param.ck.g2_A, A);

            bn_new(pp.ekp0);bn_new(pp.ekp1);
            pvhss::rlwe::ot::KeyGen(pp.param,pp.sk,pp.modulus,pp.pkePk,pp.ekp0,pp.ekp1);
        });
        pp.profile.push_back({"SetupVhss", vhss});
        pp.profile.push_back({"SetupExtra", extra});
        return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput t; NTL::vec_ZZ X;X.SetLength(x.size());
        for(size_t i=0;i<x.size();++i)X[i]=x[i];
        NTL::vec_ZZ_pX Xp;Xp.SetLength(x.size());
        for(size_t i=0;i<x.size();++i)DecimalToBinary(Xp[i],x[i],pp.param.pkePara.msg_bit);
        pvhss::rlwe::ot::ProbGen(t.Ix,pp.param,Xp,pp.modulus,pp.pkePk); return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput o; bn_t ekpb;bn_new(ekpb); if(sid==0)bn_copy(ekpb,pp.ekp0);else bn_copy(ekpb,pp.ekp1);
        auto Ix=task.Ix;
        pvhss::rlwe::ot::PVHSS_MV y_b_res, Y_b_res, sk_b, SK_b;
        int prf_key = 0;
        auto vhss = bench::MeasureOnce([&]() {
            if (sid == 0) {
                pvhss::rlwe::ot::HssEvaluateMPE(y_b_res, sid, Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_1,
                    prf_key, pp.param.pkePara.d, pp.M1_0);
                pvhss::rlwe::ot::HssEvaluateMPE(Y_b_res, sid, Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_3,
                    prf_key, pp.param.pkePara.d, pp.M3_0);
            } else {
                pvhss::rlwe::ot::HssEvaluateMPE(y_b_res, sid, Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_2,
                    prf_key, pp.param.pkePara.d, pp.M1_1);
                pvhss::rlwe::ot::HssEvaluateMPE(Y_b_res, sid, Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_4,
                    prf_key, pp.param.pkePara.d, pp.M3_1);
            }
        });
        auto extra = bench::MeasureOnce([&]() {
            if (sid == 0) {
                pvhss::rlwe::ot::HssConvertInput(sk_b, pp.param.pkePara,
                    pp.modulus, pp.param.vhssPara.vhssEk_1, pp.param.pk_f);
                pvhss::rlwe::ot::HssConvertInput(SK_b, pp.param.pkePara,
                    pp.modulus, pp.param.vhssPara.vhssEk_3, pp.param.pk_f);
            } else {
                pvhss::rlwe::ot::HssConvertInput(sk_b, pp.param.pkePara,
                    pp.modulus, pp.param.vhssPara.vhssEk_2, pp.param.pk_f);
                pvhss::rlwe::ot::HssConvertInput(SK_b, pp.param.pkePara,
                    pp.modulus, pp.param.vhssPara.vhssEk_4, pp.param.pk_f);
            }
            pvhss::rlwe::ot::HssAddMemory(y_b_res, y_b_res, sk_b);
            pvhss::rlwe::ot::HssAddMemory(Y_b_res, Y_b_res, SK_b);
            pvhss::rlwe::ot::Prove(o.proof, sid, y_b_res, Y_b_res,
                                   pp.param, ekpb);
        });
        o.profile.push_back({"Compute" + std::to_string(sid) + "Vhss", vhss});
        o.profile.push_back({"Compute" + std::to_string(sid) + "Extra", extra});
        return o;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        return {pvhss::rlwe::ot::Verify(o0.proof,o1.proof,pp.param.ck)};
    }
    static NTL::ZZ Decode(const SetupOutput& pp, const ServerOutput& o0, const ServerOutput& o1) {
        NTL::ZZ y; pvhss::rlwe::ot::Decode(y,o0.proof,o1.proof,pp.sk);
        y%=pp.param.ck.g1_order_ZZ; if(y<0)y+=pp.param.ck.g1_order_ZZ; return y;
    }
    static NTL::ZZ Modulus(const SetupOutput& pp) { return pp.param.ck.g1_order_ZZ; }
};
}}
