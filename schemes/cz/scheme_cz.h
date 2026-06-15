#pragma once
#include "PVHSS.h"
#include "PKE.h"
#include "GenData.h"
#include "tool.h"
#include "scheme_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <vector>
namespace pvhss { namespace scheme {
struct SchemeCz {
    static constexpr bool ReportDecodeTime = false;

    struct SetupOutput { PkeParams params; PvhssParams pvhss; NTL::vec_ZZ_pX pk,sk,ek1,ek2,C_alpha; NTL::ZZ_pXModulus mod; NTL::vec_ZZ_pX M1_0,M1_1; int df; };
    struct ProbGenOutput { NTL::Vec<NTL::vec_ZZ_pX> CX; };
    struct ServerOutput { NTL::ZZ_pX y; ep_t g1; ep2_t g2; };
    struct VerifyOutput { bool accepted; };

    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp; pp.pk.SetLength(2);pp.sk.SetLength(2);pp.df=cfg.degree_f;
        PkeGen(pp.params,pp.pk,pp.sk,cfg.degree_f); pp.params.msg_bit=cfg.msg_bits;
        pp.params.d=cfg.degree_f; pp.params.num_data=cfg.msg_num;
        NTL::build(pp.mod,pp.params.xN); pp.ek1.SetLength(2);pp.ek2.SetLength(2);pp.C_alpha.SetLength(4);
        PvhssGen(pp.ek1,pp.ek2,pp.C_alpha,pp.pvhss,pp.params,pp.mod,pp.pk,pp.sk);
        NTL::ZZ_pX one; DecimalToBinary(one,NTL::ZZ(1),1); NTL::vec_ZZ_pX C1;C1.SetLength(4);
        PvhssEnc(C1,pp.params,pp.mod,pp.pk,one); pp.M1_0.SetLength(2); pp.M1_1.SetLength(2);
        PvhssMult(pp.M1_0,pp.params,pp.mod,pp.ek1,C1);
        PvhssMult(pp.M1_1,pp.params,pp.mod,pp.ek2,C1); return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput t; for(auto& xi:x){NTL::ZZ_pX xp;DecimalToBinary(xp,xi,pp.params.msg_bit);
        NTL::vec_ZZ_pX C;C.SetLength(4);PvhssEnc(C,pp.params,pp.mod,pp.pk,xp);t.CX.append(C);} return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput o; ep_new(o.g1);ep2_new(o.g2); int k=task.CX.length();
        const int eval_id=(sid==0)?1:2;
        PvhssEval(o.y,o.g1,o.g2,eval_id,pp.pvhss,pp.params,pp.mod,
                  (sid==0)?pp.ek1:pp.ek2,pp.C_alpha,task.CX,pp.df,
                  (sid==0)?pp.M1_0:pp.M1_1);
        return o;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        ep_t g1;ep2_t g2;ep_new(g1);ep2_new(g2);ep_copy(g1,o0.g1);ep2_copy(g2,o1.g2);
        NTL::ZZ saved_mod=NTL::ZZ_p::modulus();
        bool ok=PvhssVerify(o0.y,o1.y,g1,g2,pp.pvhss);
        NTL::ZZ_p::init(saved_mod);
        return {ok};
    }
    static NTL::ZZ Decode(const SetupOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        NTL::ZZ_pX d=o1.y-o0.y;NTL::ZZX dzx;NTL::conv(dzx,d);NTL::ZZ r(0);EvalZZX(r,dzx);return r;
    }
};
}}
