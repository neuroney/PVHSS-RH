#pragma once
#include "PVHSS.h"
#include "PKE.h"
#include "GenData.h"
#include "tool.h"
#include "protocol_bench_runner.h"
#include "helper.h"
#include <NTL/ZZ.h>
#include <vector>
namespace pvhss { namespace protocol {
struct ProtocolCz {
    struct SetupOutput { PkeParams params; PvhssParams pvhss; NTL::vec_ZZ_pX pk,sk,ek1,ek2,C_alpha; NTL::ZZ_pXModulus mod; NTL::vec_ZZ_pX M_base; int df; };
    struct ProbGenOutput { NTL::Vec<NTL::vec_ZZ_pX> CX; };
    struct ServerOutput { NTL::ZZ_pX y; ep_t g1; ep2_t g2; };
    struct VerifyOutput { bool accepted; };
    static bench::BenchCounters counters;
    static SetupOutput Setup(const bench::BenchConfig& cfg) {
        SetupOutput pp; pp.pk.SetLength(2);pp.sk.SetLength(2);pp.df=cfg.degree_f;
        PkeGen(pp.params,pp.pk,pp.sk,cfg.degree_f); pp.params.msg_bit=cfg.msg_bits;
        pp.params.d=cfg.degree_f; pp.params.num_data=cfg.msg_num;
        NTL::build(pp.mod,pp.params.xN); pp.ek1.SetLength(2);pp.ek2.SetLength(2);pp.C_alpha.SetLength(4);
        PvhssGen(pp.ek1,pp.ek2,pp.C_alpha,pp.pvhss,pp.params,pp.mod,pp.pk,pp.sk);
        NTL::ZZ_pX one; DecimalToBinary(one,NTL::ZZ(1),1); NTL::vec_ZZ_pX C1;C1.SetLength(4);
        PvhssEnc(C1,pp.params,pp.mod,pp.pk,one); pp.M_base.SetLength(2);
        PvhssMult(pp.M_base,pp.params,pp.mod,pp.ek1,C1); return pp;
    }
    static ProbGenOutput ProbGen(const SetupOutput& pp, const std::vector<NTL::ZZ>& x) {
        ProbGenOutput t; for(auto& xi:x){NTL::ZZ_pX xp;DecimalToBinary(xp,xi,pp.params.msg_bit);
        NTL::vec_ZZ_pX C;C.SetLength(4);PvhssEnc(C,pp.params,pp.mod,pp.pk,xp);t.CX.append(C);} return t;
    }
    static ServerOutput Compute(const SetupOutput& pp, const ProbGenOutput& task, int sid) {
        ServerOutput o; ep_new(o.g1);ep2_new(o.g2); int k=task.CX.length(); PvhssEvalWorkspace ws; InitPvhssEvalWorkspace(ws,pp.df);
        ws.dp_prev[0]=pp.M_base; for(int s=1;s<=pp.df;s++){ws.dp_prev[s].SetLength(2);ws.dp_prev[s][0]=0;ws.dp_prev[s][1]=0;}
        for(int i=1;i<=k;i++){ws.dp_curr[0]=ws.dp_prev[0];
            for(int s=1;s<=pp.df;s++){ws.dp_curr[s].SetLength(2);ws.dp_curr[s][0]=0;ws.dp_curr[s][1]=0;
                PvhssAddInPlace(ws.dp_curr[s],ws.dp_prev[s]); PvhssMult(ws.next,pp.params,pp.mod,ws.dp_curr[s-1],task.CX[i-1]);
                PvhssAddInPlace(ws.dp_curr[s],ws.next);} ws.dp_prev.swap(ws.dp_curr);}
        NTL::vec_ZZ_pX yb;yb.SetLength(2);yb[0]=0;yb[1]=0; for(int s=1;s<=pp.df;s++)PvhssAddInPlace(yb,ws.dp_prev[s]); o.y=yb[0];
        NTL::vec_ZZ_pX Y;Y.SetLength(2);PvhssMult(Y,pp.params,pp.mod,yb,pp.C_alpha);NTL::ZZX tb0;NTL::conv(tb0,Y[0]);
        NTL::ZZ Tb;EvalZZX(Tb,tb0);bn_t T;bn_new(T);
        if(sid==1){Tb%=pp.pvhss.g1_order_ZZ;ZZtoBn(T,Tb);ep_mul_gen(o.g1,T);}
        else{Tb%=pp.pvhss.g2_order_ZZ;ZZtoBn(T,Tb);ep2_mul_gen(o.g2,T);}
        counters.witness_mul_count=k*pp.df;counters.total_mul_count=counters.witness_mul_count;counters.pairing_count=2; return o;
    }
    static VerifyOutput Verify(const SetupOutput& pp, const ProbGenOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        ep_t g1;ep2_t g2;ep_new(g1);ep2_new(g2);ep_copy(g1,o0.g1);ep2_copy(g2,o1.g2);
        return {PvhssVerify(o0.y,o1.y,g1,g2,pp.pvhss)};
    }
    static NTL::ZZ Decode(const SetupOutput&, const ServerOutput& o0, const ServerOutput& o1) {
        NTL::ZZ_pX d=o1.y-o0.y;NTL::ZZX dzx;NTL::conv(dzx,d);NTL::ZZ r(0);EvalZZX(r,dzx);return r;
    }
    static bench::BenchCounters GetCounters(){return counters;}
};
inline bench::BenchCounters ProtocolCz::counters;
}}
