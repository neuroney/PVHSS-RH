#pragma once
#include "PiOTGroup.h"
#include "scheme_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace scheme {

struct SchemeGroupOt
{
    struct SetupOutput {
        pvhss::group::ot::PVHSSElg1_Para param;
        VhssElgamalEk ek0, ek1;
        pvhss::group::ot::PVHSSElg1_SK sk;
        bn_t ekp0, ekp1;
        std::vector<bench::PhaseTiming> profile;
    };
    struct ProbGenOutput { std::vector<VhssElgamalCt> Ix; };
    struct ServerOutput {
        pvhss::group::ot::PROOF proof;
        std::vector<bench::PhaseTiming> profile;
    };
    struct VerifyOutput { bool accepted; };

    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp;
        pp.param.skLen=cfg.sk_len; pp.param.vkLen=cfg.vk_len;
        pp.param.msg_bits=cfg.msg_bits; pp.param.degree_f=cfg.degree_f; pp.param.msg_num=cfg.msg_num;
        auto vhss = bench::MeasureOnce([&]() {
            VhssElgamalGen(pp.param.pk, pp.ek0, pp.ek1, cfg.sk_len, cfg.vk_len);
        });
        auto extra = bench::MeasureOnce([&]() {
            pvhss::group::ot::Ped_ComGen(pp.param.ck);
            bn_t A;
            bn_new(A);
            ep2_new(pp.param.ck.g2_A);
            ZZtoBn(A, (pp.ek1[1] - pp.ek0[1]) % pp.param.pk.N);
            ep2_mul_gen(pp.param.ck.g2_A, A);
        });
        auto gen = bench::MeasureOnce([&]() {
            bn_new(pp.ekp0); bn_new(pp.ekp1);
            pvhss::group::ot::PVHSSElg1_KeyGen(pp.param, pp.sk, pp.ekp0, pp.ekp1);
        });
        pp.profile.push_back({"SetupVhss", vhss});
        pp.profile.push_back({"SetupExtra", extra});
        pp.profile.push_back({"Gen", gen});
        return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput task; NTL::vec_ZZ X; X.SetLength(x.size());
        for(size_t i=0;i<x.size();++i) X[i]=x[i];
        pvhss::group::ot::PVHSSElg1_ProbGen(task.Ix, pp.param, X);
        return task;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput out;
        bn_t ekpb; bn_new(ekpb);
        if(sid==0) bn_copy(ekpb, pp.ekp0); else bn_copy(ekpb, pp.ekp1);
        auto Ix_copy = task.Ix; std::vector<std::vector<int>> F_TEST;
        int prf_key = 0;
        VhssElgamalMv y_b_res;
        auto vhss = bench::MeasureOnce([&]() {
            VhssElgamalEvaluateMPE(y_b_res, sid, Ix_copy, pp.param.pk,
                                   (sid==0)?pp.ek0:pp.ek1, prf_key,
                                   pp.param.degree_f);
        });
        auto extra = bench::MeasureOnce([&]() {
            VhssElgamalMv sk_b;
            VhssElgamalConvertInput(sk_b, sid, pp.param.pk,
                                    (sid==0)?pp.ek0:pp.ek1, pp.param.pk_f,
                                    prf_key);
            y_b_res[0] = y_b_res[0] + sk_b[0];
            y_b_res[2] = y_b_res[2] + sk_b[2];
            pvhss::group::ot::Ped_Prove(out.proof, sid, y_b_res[0], y_b_res[2],
                                        pp.param.ck, prf_key, ekpb);
        });
        out.profile.push_back({"Compute" + std::to_string(sid) + "Vhss", vhss});
        out.profile.push_back({"Compute" + std::to_string(sid) + "Extra", extra});
        return out;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        return {pvhss::group::ot::PVHSSElg1_Verify(o0.proof, o1.proof, pp.param.ck)};
    }
    static NTL::ZZ Decode(const SetupOutput& pp, const ServerOutput& o0, const ServerOutput& o1) {
        NTL::ZZ y; pvhss::group::ot::PVHSSElg1_Decode(y, o0.proof, o1.proof, pp.sk);
        return y % pp.param.ck.g1_order_ZZ;
    }
    static NTL::ZZ Modulus(const SetupOutput& pp) { return pp.param.ck.g1_order_ZZ; }
};
}} 
