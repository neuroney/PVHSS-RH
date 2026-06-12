#pragma once

#include "rms_program.h"
#include "VHSSRLWE.h"

#include <NTL/ZZ.h>
#include <NTL/ZZ_pX.h>
#include <vector>

namespace pvhss { namespace backend {

/// RLWE VHSS backend adapter.
struct RlweVhssBackend
{
    using PkeParams   = pvhss::rlwe::vhss::PKE_Para;
    using VhssParams  = pvhss::rlwe::vhss::VHSS_Para;
    using PublicKey   = NTL::vec_ZZ_pX;
    using EvalKey     = NTL::vec_ZZ_pX;
    using Ciphertext  = NTL::vec_ZZ_pX;
    using Memory      = NTL::vec_ZZ_pX;

    static void GenerateKeys(PkeParams& params, PublicKey& pk,
                             VhssParams& vhss, EvalKey& ek0, EvalKey& ek1)
    {
        NTL::vec_ZZ_pX sk;
        pk.SetLength(2);
        sk.SetLength(2);
        pvhss::rlwe::vhss::SetParams(params);
        pvhss::rlwe::vhss::PKE_Gen(params, pk, sk);
        NTL::ZZ_pXModulus modulus(params.xN);
        pvhss::rlwe::vhss::VHSS_Gen(vhss, params, modulus, sk);
        ek0 = vhss.vhssEk_1;  // simplified: use VHSS eval keys
        ek1 = vhss.vhssEk_2;
    }

    static NTL::ZZ_pXModulus MakeModulus(const PkeParams& params)
    {
        return NTL::ZZ_pXModulus(params.xN);
    }

    static Ciphertext EncodeInput(const PkeParams& params,
                                  const NTL::ZZ_pXModulus& modulus,
                                  const PublicKey& pk, const NTL::ZZ& x)
    {
        Ciphertext ct;
        ct.SetLength(4);
        pvhss::rlwe::vhss::VHSS_Enc(ct, params, modulus, pk, x);
        return ct;
    }

    static Memory ConstantMemory(int /*sid*/, const PkeParams& params,
                                 const NTL::ZZ_pXModulus& modulus,
                                 const EvalKey& ek, const NTL::ZZ& c)
    {
        Memory mem;
        mem.SetLength(2);
        if (IsZero(c)) { mem[0]=0; mem[1]=0; return mem; }
        NTL::vec_ZZ_pX ct;
        ct.SetLength(4);
        NTL::ZZ_pX c_px; NTL::conv(c_px, c);
        pvhss::rlwe::vhss::PKE_OKDM(ct, params, modulus, pk, c_px);
        pvhss::rlwe::vhss::HssConvertInput(mem, params, modulus, ek, ct);
        return mem;
    }

    static Ciphertext EvalInputLinComb(
        const PkeParams& /*params*/,
        const std::vector<Ciphertext>& inputs,
        const programs::InputLinComb& lc)
    {
        if (lc.terms.size()==1 && lc.terms[0].coeff==1 && lc.terms[0].index>=1)
            return inputs[lc.terms[0].index-1];
        Ciphertext acc; acc.SetLength(4);
        bool first=true;
        for (const auto& t : lc.terms) {
            if (t.index==0) continue;
            Ciphertext ct=inputs[t.index-1];
            if (t.coeff!=1) {
                NTL::ZZ_pX sc; NTL::conv(sc,NTL::conv<NTL::ZZ>(t.coeff));
                for(long j=0;j<ct.length();++j) ct[j]*=sc;
            }
            if(first){acc=ct;first=false;}
            else{for(long j=0;j<acc.length();++j)acc[j]+=ct[j];}
        }
        return acc;
    }

    static Memory EvalMemLinComb(
        const std::vector<Memory>& memory, const programs::MemLinComb& lc)
    {
        Memory acc; acc.SetLength(2); acc[0]=0;acc[1]=0;
        for(const auto& t:lc.terms){
            Memory s=memory[t.index];
            if(t.coeff!=1){NTL::ZZ_pX sc;NTL::conv(sc,NTL::conv<NTL::ZZ>(t.coeff));s[0]*=sc;s[1]*=sc;}
            pvhss::rlwe::vhss::HssAddMemoryInPlace(acc,s);
        }
        return acc;
    }

    static Memory MulInputMemory(int /*sid*/, const PkeParams& params,
                                 const NTL::ZZ_pXModulus& modulus,
                                 const EvalKey& ek, const Ciphertext& a,
                                 const Memory& b, int& /*prf*/)
    {
        Memory r; r.SetLength(2);
        NTL::vec_ZZ_pX tmp; tmp.SetLength(2);
        pvhss::rlwe::vhss::HssConvertInput(tmp,params,modulus,ek,a);
        r[0]=tmp[0]+b[0]; r[1]=tmp[1]+b[1];
        return r;
    }

    static void AddInPlace(Memory& acc, const Memory& rhs)
    { pvhss::rlwe::vhss::HssAddMemoryInPlace(acc,rhs); }

    static void ScalarMulInPlace(Memory& mem, const NTL::ZZ& c)
    { NTL::ZZ_pX sc;NTL::conv(sc,NTL::conv<NTL::ZZ>(c));mem[0]*=sc;mem[1]*=sc; }

    static NTL::ZZ Decode(const PkeParams& params, const Memory& m0, const Memory& m1)
    { NTL::ZZ_pX d=m1[0]-m0[0]; NTL::ZZ r; NTL::conv(r,NTL::coeff(d,0)); return r; }

    static bool Verify(const Memory& y0, const Memory& y1,
                       const Memory& Y0, const Memory& Y1,
                       const VhssParams& vhss)
    {
        return pvhss::rlwe::vhss::VHSS_Verify(y0,y1,Y0,Y1,vhss);
    }
};

}} // namespace pvhss::backend
