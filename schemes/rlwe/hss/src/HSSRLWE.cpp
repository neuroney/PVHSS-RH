/**
 * HSSRLWE.cpp
 * 
 * lhss-based implementation of the HSSRLWE API.
 * Uses lhss RNS/NTT internally for ~2.2x speedup on N=32768.
 */

#include "HSSRLWE.h"

#include "lhss.hpp"
#include "lattice.hpp"
#include "poly.hpp"
#include "sampler.hpp"
#include "secretkey.hpp"
#include "ctxt.hpp"
#include "rlwe_ops.hpp"
#include "rlwe_ops_sym.hpp"
#include "evaluator.hpp"
#include "params.hpp"

#include <cstring>
#include <stdexcept>
#include <memory>
#include <vector>
#include <sstream>

using namespace NTL;
using namespace std;

using namespace lhss;

namespace pvhss { namespace rlwe { namespace hss {

// ============================================================
// Internal LHSS context
// ============================================================

struct LHSSContext {
    SecretKey sk;
    Ciphertext pk;
};

static LHSSContext* GetCtx(PKE_Para& p) {
    return static_cast<LHSSContext*>(p.lhss_client_);
}

// ============================================================
// NTL <-> lhss conversion helpers
// ============================================================

static inline ZZ mpzToZZ(const mpz_class& val) {
    return to_ZZ(val.get_str().c_str());
}

static void NTLToCRTPoly(const ZZ_pX& ntl_poly, CRTPoly& crt)
{
    for (size_t mod_i = 0; mod_i < Params::num_moduli; ++mod_i) {
        UIntType qi = Params::z_qi[mod_i].modulus;
        auto& sp = crt.small_polys[mod_i];
        for (size_t j = 0; j < Params::n; ++j) {
            ZZ c = conv<ZZ>(coeff(ntl_poly, static_cast<long>(j)));
            sp.coeffs[j] = static_cast<UIntType>(c % qi);
        }
    }
}

static void NTLToSecretKey(const vec_ZZ_pX& ntl_vec, SecretKey& sk)
{
    NTLToCRTPoly(ntl_vec[0], sk.GetB());
    NTLToCRTPoly(ntl_vec[1], sk.GetA());
    BigNTT::ApplyNTT(sk.GetB());
    BigNTT::ApplyNTT(sk.GetA());
}

static void SecretKeyToNTL(const SecretKey& sk, vec_ZZ_pX& ntl_vec)
{
    CRTPoly b = sk.GetConstB();
    CRTPoly a = sk.GetConstA();
    BigNTT::ApplyInvNTT(b);
    BigNTT::ApplyInvNTT(a);
    
    BigPoly bp_b, bp_a;
    PolyConv::CRTToBigPoly(b, bp_b);
    PolyConv::CRTToBigPoly(a, bp_a);
    
    ntl_vec.SetLength(2);
    ntl_vec[0].SetLength(Params::n);
    ntl_vec[1].SetLength(Params::n);
    
    for (size_t j = 0; j < Params::n; ++j) {
        ZZ v = mpzToZZ(bp_b.GetConstVal(j));
        SetCoeff(ntl_vec[0], static_cast<long>(j), conv<ZZ_p>(v));
        v = mpzToZZ(bp_a.GetConstVal(j));
        SetCoeff(ntl_vec[1], static_cast<long>(j), conv<ZZ_p>(v));
    }
}

static void NTLToHSSCtxt(const vec_ZZ_pX& ntl_ct, HSSCtxt& hss_ct)
{
    NTLToCRTPoly(ntl_ct[0], hss_ct.enc_m.GetB());
    NTLToCRTPoly(ntl_ct[1], hss_ct.enc_m.GetA());
    NTLToCRTPoly(ntl_ct[2], hss_ct.enc_m_times_s.GetB());
    NTLToCRTPoly(ntl_ct[3], hss_ct.enc_m_times_s.GetA());
    
    BigNTT::ApplyNTT(hss_ct.enc_m.GetB());
    BigNTT::ApplyNTT(hss_ct.enc_m.GetA());
    BigNTT::ApplyNTT(hss_ct.enc_m_times_s.GetB());
    BigNTT::ApplyNTT(hss_ct.enc_m_times_s.GetA());
}

static void CRTPolyToZZ_pX(const CRTPoly& crt, ZZ_pX& out)
{
    BigPoly bp;
    PolyConv::CRTToBigPoly(crt, bp);
    out.SetLength(Params::n);
    for (size_t j = 0; j < Params::n; ++j) {
        ZZ v = mpzToZZ(bp.GetConstVal(j));
        SetCoeff(out, static_cast<long>(j), conv<ZZ_p>(v));
    }
}

// ============================================================
// Parameter setup
// ============================================================

static bool s_lhss_global_init = false;

void SetParams(PKE_Para &pkePara)
{
    pkePara.N = 32768;
    
    if (!s_lhss_global_init) {
        std::vector<UIntType> plain_mods = {
            18014398506729473ULL, 18014398505943041ULL,
            9007199252840449ULL, 9007199252119553ULL,
            9007199251660801ULL, 9007199250874369ULL
        };
        std::vector<UIntType> ctxt_mods = {
            18014398506729473ULL, 18014398505943041ULL,
            9007199252840449ULL, 9007199252119553ULL,
            9007199251660801ULL, 9007199250874369ULL,
            144115188075593729ULL, 144115188075134977ULL,
            144115188070809601ULL, 144115188070023169ULL,
            144115188068319233ULL, 144115188068253697ULL
        };
        LHSSInit(plain_mods, ctxt_mods, 32768);
        s_lhss_global_init = true;
    }
    
    conv(pkePara.q, to_ZZ(Params::Q.get_str().c_str()));
    conv(pkePara.p, to_ZZ(Params::P.get_str().c_str()));
    ZZ_p::init(pkePara.q);
    
    SetCoeff(pkePara.xN, 0, 1);
    SetCoeff(pkePara.xN, pkePara.N, 1);
    
    pkePara.twice_p = 2 * pkePara.p;
    pkePara.twice_q = 2 * pkePara.q;
    pkePara.half_p = pkePara.p / 2;
    
    auto* ctx = new LHSSContext();
    pkePara.lhss_client_ = ctx;
    pkePara.lhss_initialized_ = true;
}

// ============================================================
// Key Generation
// ============================================================

void PKE_Gen(PKE_Para &pkePara, vec_ZZ_pX &pkePk, vec_ZZ_pX &pkeSk)
{
    SetParams(pkePara);
    auto* ctx = GetCtx(pkePara);
    
    CRTPoly::SetConstantNTT(1, ctx->sk.GetB());
    CRTSampler::SampleUniformTernaryWithHW(pkePara.hsk, ctx->sk.GetA());
    BigNTT::ApplyNTT(ctx->sk.GetA());
    
    EncryptZero(ctx->sk, ctx->pk);
    
    SecretKey pk_as_sk;
    pk_as_sk.GetB() = ctx->pk.GetConstB();
    pk_as_sk.GetA() = ctx->pk.GetConstA();
    SecretKeyToNTL(pk_as_sk, pkePk);
    SecretKeyToNTL(ctx->sk, pkeSk);
}

// ============================================================
// HSS Key Sharing
// ============================================================

void HssGen(vec_ZZ_pX &hssEk_1, vec_ZZ_pX &hssEk_2,
             const PKE_Para& pkePara, const vec_ZZ_pX& pkeSk)
{
    SecretKey sk_lhss;
    NTLToSecretKey(pkeSk, sk_lhss);
    
    SecretKey ek1_lhss;
    CRTSampler::SampleUniformModQ(ek1_lhss.GetB());
    CRTSampler::SampleUniformModQ(ek1_lhss.GetA());
    
    SecretKey ek2_lhss;
    SecretKey::SubMod(sk_lhss, ek1_lhss, ek2_lhss);
    
    SecretKeyToNTL(ek1_lhss, hssEk_1);
    SecretKeyToNTL(ek2_lhss, hssEk_2);
}

// ============================================================
// HSS Encryption
// ============================================================

void HSS_Enc(vec_ZZ_pX &C, const PKE_Para &pkePara, ZZ_pXModulus &modulus,
             const vec_ZZ_pX& pkePk, const ZZ &x)
{
    SecretKey pk_lhss;
    NTLToSecretKey(pkePk, pk_lhss);
    
    auto pk_ptr = std::make_shared<Ciphertext>();
    pk_ptr->GetB() = pk_lhss.GetB();
    pk_ptr->GetA() = pk_lhss.GetA();
    
    uint64_t m = static_cast<uint64_t>(to_long(x));
    HSSCtxt hss_ct;
    Poly plain;
    plain.SetCoeffs({m});
    HSSEncrypt(pk_ptr, plain, hss_ct);
    
    BigNTT::ApplyInvNTT(hss_ct.enc_m.GetB());
    BigNTT::ApplyInvNTT(hss_ct.enc_m.GetA());
    BigNTT::ApplyInvNTT(hss_ct.enc_m_times_s.GetB());
    BigNTT::ApplyInvNTT(hss_ct.enc_m_times_s.GetA());
    
    C.SetLength(4);
    CRTPolyToZZ_pX(hss_ct.enc_m.GetB(), C[0]);
    CRTPolyToZZ_pX(hss_ct.enc_m.GetA(), C[1]);
    CRTPolyToZZ_pX(hss_ct.enc_m_times_s.GetB(), C[2]);
    CRTPolyToZZ_pX(hss_ct.enc_m_times_s.GetA(), C[3]);
}

// ============================================================
// HSS Multiplication
// ============================================================

void HSS_Mult(vec_ZZ_pX &db, const PKE_Para& pkePara, const ZZ_pXModulus& modulus,
              const vec_ZZ_pX& pkeSk, const vec_ZZ_pX& C)
{
    SecretKey sk_lhss;
    NTLToSecretKey(pkeSk, sk_lhss);
    
    HSSCtxt hss_ct;
    NTLToHSSCtxt(C, hss_ct);
    
    auto dummy_pk = std::make_shared<Ciphertext>();
    Evaluator eval(dummy_pk, sk_lhss, "", 0);
    
    SecretKey result;
    eval.MultHSSCtxtAndShare(hss_ct, sk_lhss, result);
    
    SecretKeyToNTL(result, db);
}

// ============================================================
// HSS Memory Operations
// ============================================================

void HssAddMemory(vec_ZZ_pX &tb, const vec_ZZ_pX &C_X, const vec_ZZ_pX& C_Y)
{
    tb[0] = C_X[0] + C_Y[0];
    tb[1] = C_X[1] + C_Y[1];
}

void HssConvertInput(vec_ZZ_pX &tb_y, const PKE_Para& pkePara,
                      const ZZ_pXModulus& modulus, const vec_ZZ_pX& ek,
                      const vec_ZZ_pX& C_X)
{
    tb_y.SetLength(2);
    HSS_Mult(tb_y, pkePara, modulus, ek, C_X);
}

// ============================================================
// Optimized DP Evaluation
// ============================================================

void HssEvaluatePolyD2(vec_ZZ_pX &y_b_res, int b, const Vec<vec_ZZ_pX> &Ix,
                       const PKE_Para &pkePara, ZZ_pXModulus modulus,
                       const vec_ZZ_pX &pkeSk, int &prf_key, int degree_f,
                       const vec_ZZ_pX &M1)
{
    int k = Ix.length();
    
    SecretKey sk_lhss;
    NTLToSecretKey(pkeSk, sk_lhss);
    
    std::vector<HSSCtxt> ctxts(k);
    for (int i = 0; i < k; ++i) {
        NTLToHSSCtxt(Ix[i], ctxts[i]);
    }
    
    SecretKey M1_lhss;
    NTLToSecretKey(M1, M1_lhss);
    
    auto dummy_pk = std::make_shared<Ciphertext>();
    Evaluator eval(dummy_pk, sk_lhss, "", 0);
    
    std::vector<SecretKey> dp_prev(degree_f + 1);
    std::vector<SecretKey> dp_curr(degree_f + 1);
    
    dp_prev[0] = M1_lhss;
    
    for (int i = 0; i < k; ++i) {
        for (int s = 0; s <= degree_f; ++s) {
            dp_curr[s] = dp_prev[s];
        }
        
        for (int r = 0; r < degree_f; ++r) {
            SecretKey chain = dp_prev[r];
            
            for (int j = 1; r + j <= degree_f; ++j) {
                SecretKey next_chain;
                eval.MultHSSCtxtAndShare(ctxts[i], chain, next_chain);
                chain = next_chain;
                SecretKey::AddMod(chain, dp_curr[r + j]);
            }
        }
        
        dp_prev.swap(dp_curr);
    }
    
    SecretKey result;
    for (int s = 1; s <= degree_f; ++s) {
        SecretKey::AddMod(dp_prev[s], result);
    }
    
    SecretKeyToNTL(result, y_b_res);
}

// ============================================================
// Data generation (for testing/benchmarking)
// ============================================================

void GenerateData(Data &data, const PKE_Para& pkePara, const vec_ZZ_pX& pkePk)
{
    data.X.SetLength(pkePara.num_data);
    vec_ZZ_pX C_x, prf;
    C_x.SetLength(4);
    prf.SetLength(2);
    ZZ_pXModulus modulus(pkePara.xN);
    for (int i = 0; i < pkePara.num_data; i++)
    {
        NTL::RandomBits(data.X[i], pkePara.msg_bit);
        HSS_Enc(C_x, pkePara, modulus, pkePk, data.X[i]);
        data.C_X.append(C_x);
    }
    for (int i = 0; i < 10; i++)
    {
        RandomZZpx(prf[0], pkePara.N, pkePara.q_bit);
        RandomZZpx(prf[1], pkePara.N, pkePara.q_bit);
        data.PRF.append(prf);
    }
}

// ============================================================
// Cleanup
// ============================================================

void PKE_Cleanup(PKE_Para &pkePara)
{
    if (pkePara.lhss_client_) {
        delete GetCtx(pkePara);
        pkePara.lhss_client_ = nullptr;
    }
    pkePara.lhss_initialized_ = false;
}

}}} // namespace pvhss::rlwe::hss
