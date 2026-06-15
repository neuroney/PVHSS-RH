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
        std::vector<bench::PhaseTiming> profile;
    };
    struct ProbGenOutput { std::vector<NTL::vec_ZZ_pX> Ix; };
    struct ServerOutput {
        pvhss::rlwe::dc::PROOF proof;
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
            pvhss::rlwe::dc::PKE_Gen(pp.param.pkePara, pp.pkePk, pkeSk);
            pp.modulus=NTL::ZZ_pXModulus(pp.param.pkePara.xN);
            pvhss::rlwe::dc::VHSS_Gen(pp.param.vhssPara, pp.param.pkePara,
                                      pp.modulus, pkeSk);
            pp.M1_0.SetLength(2);pp.M1_1.SetLength(2);pp.M3_0.SetLength(2);pp.M3_1.SetLength(2);
            NTL::vec_ZZ_pX C1;C1.SetLength(4);
            NTL::ZZ_pX one; DecimalToBinary(one,NTL::ZZ(1),1);
            pvhss::rlwe::dc::PKE_OKDM(C1,pp.param.pkePara,pp.modulus,pp.pkePk,one);
            pvhss::rlwe::dc::HssConvertInput(pp.M1_0,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_1,C1);
            pvhss::rlwe::dc::HssConvertInput(pp.M1_1,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_2,C1);
            pvhss::rlwe::dc::HssConvertInput(pp.M3_0,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_3,C1);
            pvhss::rlwe::dc::HssConvertInput(pp.M3_1,pp.param.pkePara,pp.modulus,pp.param.vhssPara.vhssEk_4,C1);
        });
        auto extra = bench::MeasureOnce([&]() {
            pvhss::rlwe::dc::DecPed_ComGen(pp.param.ck, pp.param.f_sk);
            NTL::RandomBits(pp.param.sk_alpha, 32);
            const NTL::ZZ A_ZZ = pvhss::rlwe::dc::HssOutputPolyAtTwo(
                pp.param.vhssPara.alpha, pp.param.pkePara, pp.param.ck.g1_order_ZZ);
            bn_t A;
            bn_new(A);
            ep2_new(pp.param.vk);
            ZZtoBn(A, A_ZZ);
            ep2_mul_gen(pp.param.vk, A);

            bn_t ekp0[2],ekp1[2];
            bn_new(ekp0[0]);bn_new(ekp0[1]);bn_new(ekp1[0]);bn_new(ekp1[1]);
            pvhss::rlwe::dc::KeyGen(pp.param,pp.sk,pp.modulus,pp.pkePk,ekp0,ekp1);
            bn_new(pp.ekp0_0);bn_new(pp.ekp0_1);bn_new(pp.ekp1_0);bn_new(pp.ekp1_1);
            bn_copy(pp.ekp0_0,ekp0[0]);bn_copy(pp.ekp0_1,ekp0[1]);bn_copy(pp.ekp1_0,ekp1[0]);bn_copy(pp.ekp1_1,ekp1[1]);
        });
        pp.profile.push_back({"SetupVhss", vhss});
        pp.profile.push_back({"SetupExtra", extra});
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
        auto Ix=task.Ix;
        pvhss::rlwe::dc::PVHSS_MV y_b_res, Y_b_res;
        int prf_key = 0;
        auto vhss = bench::MeasureOnce([&]() {
            if (sid == 0) {
                pvhss::rlwe::dc::HssEvaluateMPE(y_b_res, sid, Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_1,
                    prf_key, pp.param.pkePara.d, pp.M1_0);
                pvhss::rlwe::dc::HssEvaluateMPE(Y_b_res, sid, Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_3,
                    prf_key, pp.param.pkePara.d, pp.M3_0);
            } else {
                pvhss::rlwe::dc::HssEvaluateMPE(y_b_res, sid, Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_2,
                    prf_key, pp.param.pkePara.d, pp.M1_1);
                pvhss::rlwe::dc::HssEvaluateMPE(Y_b_res, sid, Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_4,
                    prf_key, pp.param.pkePara.d, pp.M3_1);
            }
        });
        auto extra = bench::MeasureOnce([&]() {
            pvhss::rlwe::dc::Prove(o.proof, sid, y_b_res, Y_b_res,
                                   pp.param, ekpb);
        });
        o.profile.push_back({"Compute" + std::to_string(sid) + "Vhss", vhss});
        o.profile.push_back({"Compute" + std::to_string(sid) + "Extra", extra});
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
