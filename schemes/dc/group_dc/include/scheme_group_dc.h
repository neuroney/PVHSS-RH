#pragma once
#include "PiDCGroup.h"
#include "scheme_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace scheme {

struct SchemeGroupDc
{
    struct SetupOutput {
        pvhss::group::dc::PVHSSElg2_Para param;
        VhssElgamalEk ek0, ek1;
        pvhss::group::dc::PVHSSElg2_SK sk;
        bn_t ekp0_0, ekp0_1, ekp1_0, ekp1_1;
        std::vector<bench::PhaseTiming> profile;
    };
    struct ProbGenOutput { std::vector<VhssElgamalCt> Ix; };
    struct ServerOutput {
        pvhss::group::dc::PROOF proof;
        std::vector<bench::PhaseTiming> profile;
    };
    struct VerifyOutput { bool accepted; };

    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp;
        pp.param.skLen=cfg.sk_len; pp.param.vkLen=cfg.vk_len; pp.param.msg_bits=cfg.msg_bits;
        pp.param.degree_f=cfg.degree_f; pp.param.msg_num=cfg.msg_num;
        auto vhss = bench::MeasureOnce([&]() {
            VhssElgamalGen(pp.param.pk, pp.ek0, pp.ek1, cfg.sk_len, cfg.vk_len);
        });
        auto extra = bench::MeasureOnce([&]() {
            pvhss::group::dc::DecPed_ComGen(pp.param.ck, pp.sk);
            bn_t A;
            bn_new(A);
            ep2_new(pp.param.vk);
            ZZtoBn(A, (pp.ek1[1] - pp.ek0[1]) % pp.param.pk.N);
            ep2_mul_gen(pp.param.vk, A);
        });
        pp.profile.push_back({"SetupVhss", vhss});
        pp.profile.push_back({"SetupExtra", extra});
        return pp;
    }
    static void KeyGen(SetupOutput& pp) {
        bn_t ekp0[2], ekp1[2];
        bn_new(ekp0[0]);bn_new(ekp0[1]);bn_new(ekp1[0]);bn_new(ekp1[1]);
        pvhss::group::dc::PVHSSElg2_KeyGen(pp.param, pp.sk, ekp0, ekp1);
        bn_new(pp.ekp0_0);bn_new(pp.ekp0_1);bn_new(pp.ekp1_0);bn_new(pp.ekp1_1);
        bn_copy(pp.ekp0_0,ekp0[0]);bn_copy(pp.ekp0_1,ekp0[1]);bn_copy(pp.ekp1_0,ekp1[0]);bn_copy(pp.ekp1_1,ekp1[1]);
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput t; NTL::vec_ZZ X; X.SetLength(x.size());
        for(size_t i=0;i<x.size();++i)X[i]=x[i];
        pvhss::group::dc::PVHSSElg2_ProbGen(t.Ix, pp.param, X); return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput out;
        bn_t ekpb[2];bn_new(ekpb[0]);bn_new(ekpb[1]);
        if(sid==0){bn_copy(ekpb[0],pp.ekp0_0);bn_copy(ekpb[1],pp.ekp0_1);}
        else{bn_copy(ekpb[0],pp.ekp1_0);bn_copy(ekpb[1],pp.ekp1_1);}
        auto Ix=task.Ix; std::vector<std::vector<int>> F;
        int prf_key = 0;
        VhssElgamalMv y_b_res;
        auto vhss = bench::MeasureOnce([&]() {
            VhssElgamalEvaluateMPE(y_b_res, sid, Ix, pp.param.pk,
                                   (sid==0)?pp.ek0:pp.ek1, prf_key,
                                   pp.param.degree_f);
        });
        auto extra = bench::MeasureOnce([&]() {
            pvhss::group::dc::PVHSSElg2_Prove(out.proof, sid, y_b_res[0],
                                              y_b_res[2], pp.param, ekpb);
        });
        out.profile.push_back({"Compute" + std::to_string(sid) + "Vhss", vhss});
        out.profile.push_back({"Compute" + std::to_string(sid) + "Extra", extra});
        return out;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        return {pvhss::group::dc::PVHSSElg2_Verify(o0.proof,o1.proof,pp.param)};
    }
    static NTL::ZZ Decode(const SetupOutput& pp, const ServerOutput& o0, const ServerOutput& o1) {
        dig_t y; pvhss::group::dc::PVHSSElg2_Decode(y,o0.proof,o1.proof,pp.sk);
        return NTL::conv<NTL::ZZ>((long)y);
    }
    static bool CanDecodeReference(const SetupOutput&, const NTL::ZZ& reference) {
        return reference >= 0 && reference < NTL::conv<NTL::ZZ>((unsigned long)PVHSS_M_MAX);
    }
};
}}
