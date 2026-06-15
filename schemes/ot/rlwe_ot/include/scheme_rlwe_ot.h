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
    static std::vector<bench::PhaseTiming> SetupProfile(
        const bench::BenchConfig& cfg, int cyctimes) {
        auto vhss = bench::Measure("", [&]() {
            pvhss::rlwe::ot::PVHSSPara param;
            param.pkePara.msg_bit=32; param.pkePara.num_data=cfg.msg_num;
            param.pkePara.d=cfg.degree_f;
            NTL::vec_ZZ_pX pkePk, pkeSk;
            pkePk.SetLength(2); pkeSk.SetLength(2);
            pvhss::rlwe::ot::PKE_Gen(param.pkePara, pkePk, pkeSk);
            NTL::ZZ_pXModulus modulus(param.pkePara.xN);
            pvhss::rlwe::ot::VHSS_Gen(param.vhssPara, param.pkePara, modulus, pkeSk);

            NTL::vec_ZZ_pX M1_0, M1_1, M3_0, M3_1;
            M1_0.SetLength(2); M1_1.SetLength(2); M3_0.SetLength(2); M3_1.SetLength(2);
            NTL::vec_ZZ_pX C1; C1.SetLength(4);
            NTL::ZZ_pX one; DecimalToBinary(one, NTL::ZZ(1), 1);
            pvhss::rlwe::ot::PKE_OKDM(C1, param.pkePara, modulus, pkePk, one);
            pvhss::rlwe::ot::HssConvertInput(M1_0, param.pkePara, modulus, param.vhssPara.vhssEk_1, C1);
            pvhss::rlwe::ot::HssConvertInput(M1_1, param.pkePara, modulus, param.vhssPara.vhssEk_2, C1);
            pvhss::rlwe::ot::HssConvertInput(M3_0, param.pkePara, modulus, param.vhssPara.vhssEk_3, C1);
            pvhss::rlwe::ot::HssConvertInput(M3_1, param.pkePara, modulus, param.vhssPara.vhssEk_4, C1);
        }, cyctimes, "SetupVhss");

        SeedBenchmarkRandomness("SetupVhss");
        pvhss::rlwe::ot::PVHSSPara base_param;
        base_param.pkePara.msg_bit=32; base_param.pkePara.num_data=cfg.msg_num;
        base_param.pkePara.d=cfg.degree_f;
        NTL::vec_ZZ_pX base_pkePk, base_pkeSk;
        base_pkePk.SetLength(2); base_pkeSk.SetLength(2);
        pvhss::rlwe::ot::PKE_Gen(base_param.pkePara, base_pkePk, base_pkeSk);
        NTL::ZZ_pXModulus base_modulus(base_param.pkePara.xN);
        pvhss::rlwe::ot::VHSS_Gen(base_param.vhssPara, base_param.pkePara,
                                  base_modulus, base_pkeSk);

        auto extra = bench::Measure("", [&]() {
            pvhss::rlwe::ot::PVHSSPara param;
            param.pkePara = base_param.pkePara;
            param.vhssPara = base_param.vhssPara;

            pvhss::rlwe::ot::Ped_ComGen(param.ck);
            NTL::RandomBits(param.sk_alpha, 32);
            NTL::ZZ_p A_ZZ_p;
            eval(A_ZZ_p, param.vhssPara.alpha, NTL::conv<NTL::ZZ_p>(param.sk_alpha));
            NTL::ZZ A_ZZ = NTL::conv<NTL::ZZ>(A_ZZ_p) % param.ck.g1_order_ZZ;
            bn_t A;
            bn_new(A);
            ep2_new(param.ck.g2_A);
            ZZtoBn(A, A_ZZ);
            ep2_mul_gen(param.ck.g2_A, A);

            bn_t ekp0, ekp1;
            bn_new(ekp0); bn_new(ekp1);
            pvhss::rlwe::ot::PVHSS_SK sk;
            pvhss::rlwe::ot::KeyGen(param, sk, base_modulus, base_pkePk,
                                    ekp0, ekp1);
        }, cyctimes, "SetupExtra");

        return {{"Vhss", vhss}, {"Extra", extra}};
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
    static std::vector<bench::PhaseTiming> ComputeProfile(
        const SetupOutput& pp, const ProbGenOutput& task, int sid,
        int cyctimes) {
        bn_t ekpb; bn_new(ekpb);
        if (sid == 0) bn_copy(ekpb, pp.ekp0); else bn_copy(ekpb, pp.ekp1);

        auto vhss = bench::Measure("", [&]() {
            int prf_key = 0;
            NTL::vec_ZZ_pX y_b_res;
            NTL::vec_ZZ_pX Y_b_res;
            if (sid == 0) {
                pvhss::rlwe::ot::HssEvaluateMPE(y_b_res, sid, task.Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_1,
                    prf_key, pp.param.pkePara.d, pp.M1_0);
                pvhss::rlwe::ot::HssEvaluateMPE(Y_b_res, sid, task.Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_3,
                    prf_key, pp.param.pkePara.d, pp.M3_0);
            } else {
                pvhss::rlwe::ot::HssEvaluateMPE(y_b_res, sid, task.Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_2,
                    prf_key, pp.param.pkePara.d, pp.M1_1);
                pvhss::rlwe::ot::HssEvaluateMPE(Y_b_res, sid, task.Ix,
                    pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_4,
                    prf_key, pp.param.pkePara.d, pp.M3_1);
            }
        }, cyctimes, "Compute" + std::to_string(sid) + "Vhss");

        int base_prf_key = 0;
        NTL::vec_ZZ_pX base_y, base_Y;
        if (sid == 0) {
            pvhss::rlwe::ot::HssEvaluateMPE(base_y, sid, task.Ix,
                pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_1,
                base_prf_key, pp.param.pkePara.d, pp.M1_0);
            pvhss::rlwe::ot::HssEvaluateMPE(base_Y, sid, task.Ix,
                pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_3,
                base_prf_key, pp.param.pkePara.d, pp.M3_0);
        } else {
            pvhss::rlwe::ot::HssEvaluateMPE(base_y, sid, task.Ix,
                pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_2,
                base_prf_key, pp.param.pkePara.d, pp.M1_1);
            pvhss::rlwe::ot::HssEvaluateMPE(base_Y, sid, task.Ix,
                pp.param.pkePara, pp.modulus, pp.param.vhssPara.vhssEk_4,
                base_prf_key, pp.param.pkePara.d, pp.M3_1);
        }
        auto extra = bench::Measure("", [&]() {
            NTL::vec_ZZ_pX y_b_res = base_y;
            NTL::vec_ZZ_pX Y_b_res = base_Y;
            NTL::vec_ZZ_pX sk_b;
            NTL::vec_ZZ_pX SK_b;
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
            pvhss::rlwe::ot::PROOF proof;
            pvhss::rlwe::ot::Prove(proof, sid, y_b_res, Y_b_res,
                                   pp.param, ekpb);
        }, cyctimes, "Compute" + std::to_string(sid) + "Extra");

        return {{"Vhss", vhss}, {"Extra", extra}};
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
