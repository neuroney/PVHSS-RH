/**
 * hss_lhss_bench.cpp
 * 
 * Comparative benchmark: HSS polynomial evaluation using lhss (RNS/NTT).
 * Compares against the original HSSRLWE (NTL-based) implementation.
 * 
 * Uses N=32768 parameters from lhss-rs param_est.
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <cstdlib>

// lhss includes
#include "lhss.hpp"
#include "lattice.hpp"

// Original HSSRLWE includes for comparison
#include "helper.h"
#include "HSSRLWE.h"

using namespace NTL;
using namespace std;

using namespace std;
using namespace lhss;

// Timing helper
struct ScopeTimer {
    chrono::steady_clock::time_point start;
    string name;
    ScopeTimer(const string& n) : name(n), start(chrono::steady_clock::now()) {}
    ~ScopeTimer() {
        auto end = chrono::steady_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        cout << "[lhss] " << name << ": " << ms << " ms" << endl;
    }
};

#define TIME_IT(name, block) \
 do { \
    auto _start = chrono::steady_clock::now(); \
    block \
    auto _end = chrono::steady_clock::now(); \
    auto _ms = chrono::duration_cast<chrono::milliseconds>(_end - _start).count(); \
    cout << "[lhss] " << name << ": " << _ms << " ms" << endl; \
 } while(0)

// ============================================================
// lhss-based HSS evaluation (re-implements HssEvaluatePolyD2)
// ============================================================

// Memory share = two CRTPoly elements (in mod P, non-NTT form)
struct MemShare {
    SkShare share;  // (m, m*s) in CRT form
};

// HSS ciphertext = encryption of (m, m*s)
struct HSSCtxtLhss {
    HSSCtxt ct;
};

// Initialize lhss with N=32768 parameters (Bmax=2^256, matching original p_bit=319, q_bit=662)
void InitLHSS_N32768()
{
    // Parameters from lhss-rs param_est/lhss_128.txt, Bmax=2^256, N=32768
    // logP = 320, logQ = 662, 6 plain mods, 12 ciphertext mods
    vector<UIntType> plain_mods = {
        18014398506729473ULL,
        18014398505943041ULL,
        9007199252840449ULL,
        9007199252119553ULL,
        9007199251660801ULL,
        9007199250874369ULL
    };
    vector<UIntType> ctxt_mods = {
        18014398506729473ULL,
        18014398505943041ULL,
        9007199252840449ULL,
        9007199252119553ULL,
        9007199251660801ULL,
        9007199250874369ULL,
        144115188075593729ULL,
        144115188075134977ULL,
        144115188070809601ULL,
        144115188070023169ULL,
        144115188068319233ULL,
        144115188068253697ULL
    };
    // N = 32768 = 2^15
    LHSSInit(plain_mods, ctxt_mods, 32768);
    cout << "[lhss] Params initialized: N=" << Params::GetN() 
         << ", logP=" << Params::GetLogP() 
         << ", logQ=" << Params::GetLogQ() << endl;
}

// Generate HSS key shares (ek0, ek1) from secret key
void HSS_Gen_lhss(Client& client, SkShare& ek0, SkShare& ek1)
{
    client.ShareSk(ek0, ek1);
}

// Generate HSS encryption of a message (unused, kept for reference)
void HSS_Enc_lhss_zz(const PkPtr& pk, const ZZ& x, HSSCtxt& ct)
{
    // Convert NTL ZZ to uint64_t (messages are 32-bit, safe to cast)
    uint64_t m = static_cast<uint64_t>(to_long(x));
    Poly plain;
    plain.SetCoeffs({m});
    HSSEncrypt(pk, plain, ct);
}

// Restricted multiplication: memory share × HSS ciphertext → new memory share
// This is equivalent to HSSRLWE's HSS_Mult (distributed decryption + rounding)
void HSS_Mult_lhss(Evaluator& eval, const SkShare& mem_share, const HSSCtxt& ct, SkShare& out)
{
    eval.MultHSSCtxtAndShare(ct, mem_share, out);
}

// In-place addition of memory shares
void HSS_AddMemory_lhss(SkShare& acc, const SkShare& x)
{
    SkShare::AddMod(x, acc);
}

// Copy memory share
void HSS_Copy_lhss(const SkShare& src, SkShare& dst)
{
    dst = src;
}

// DP-based polynomial evaluation (same algorithm as HssEvaluatePolyD2)
void HSS_Evaluate_P_d2_lhss(
    SkShare& y_b_res,
    const vector<HSSCtxt>& Ix,
    Evaluator& eval,
    const SkShare& M1,
    int degree_f)
{
    int k = static_cast<int>(Ix.size());
    
    // dp_prev[0..degree_f] and dp_curr[0..degree_f]
    vector<SkShare> dp_prev(degree_f + 1);
    vector<SkShare> dp_curr(degree_f + 1);
    
    // Base case: dp_prev[0] = M1
    HSS_Copy_lhss(M1, dp_prev[0]);
    // dp_prev[1..degree_f] = 0 (already default-constructed as zeros)
    
    for (int i = 0; i < k; i++)
    {
        // dp_curr = dp_prev (copy)
        for (int s = 0; s <= degree_f; s++)
        {
            HSS_Copy_lhss(dp_prev[s], dp_curr[s]);
        }
        
        // Chain reuse: chain = dp_prev[r] * x_i^j
        for (int r = 0; r < degree_f; r++)
        {
            SkShare chain;
            HSS_Copy_lhss(dp_prev[r], chain);
            
            for (int j = 1; r + j <= degree_f; j++)
            {
                SkShare next_chain;
                HSS_Mult_lhss(eval, chain, Ix[i], next_chain);
                HSS_Copy_lhss(next_chain, chain);
                HSS_AddMemory_lhss(dp_curr[r + j], chain);
            }
        }
        
        dp_prev.swap(dp_curr);
    }
    
    // Sum up dp_prev[1..degree_f]
    // y_b_res starts as zero (default)
    for (int s = 1; s <= degree_f; s++)
    {
        HSS_AddMemory_lhss(y_b_res, dp_prev[s]);
    }
}

// ============================================================
// Benchmark entry point
// ============================================================

int main(int argc, char** argv)
{
    int msg_num = 5;
    int degree_f = 5;
    
    if (argc > 1) msg_num = atoi(argv[1]);
    if (argc > 2) degree_f = atoi(argv[2]);
    
    cout << "=== LHSS-based HSS Benchmark ===" << endl;
    cout << "N=32768, msg_num=" << msg_num << ", degree_f=" << degree_f << endl;
    
    // ---- Initialize lhss ----
    InitLHSS_N32768();
    
    // ---- Key Generation ----
    Client client(64);  // hamming weight 64
    auto pk = client.GetPk();
    
    SkShare ek0, ek1;
    SkShare M1;  // M1 = ekb converted to share form
    
    {
        ScopeTimer t("KeyGen + ShareSk");
        HSS_Gen_lhss(client, ek0, ek1);
    }
    
    // ---- Generate messages and encrypt ----
    vector<unsigned long> msgs(msg_num);
    vector<HSSCtxt> ctxts(msg_num);
    
    for (int i = 0; i < msg_num; i++)
    {
        msgs[i] = (i + 1) * 100;  // dummy messages
    }
    
    {
        ScopeTimer t("HSS Encrypt (" + to_string(msg_num) + " msgs)");
        for (int i = 0; i < msg_num; i++)
        {
            Poly plain;
            plain.SetCoeffs({msgs[i]});
            HSSEncrypt(pk, plain, ctxts[i]);
        }
    }
    
    // ---- Convert M1 (ek0 in share form) ----
    // M1 is the secret key share converted to memory share form
    // This is equivalent to HssConvertInput
    {
        ScopeTimer t("ConvertInput (M1)");
        // M1 is just ek0 (the secret key share itself)
        HSS_Copy_lhss(ek0, M1);
    }
    
    // ---- HSS Evaluation (Party 0) ----
    SkShare y_res;
    {
        ScopeTimer t("Eval (degree=" + to_string(degree_f) + ")");
        Evaluator eval(pk, ek0);
        HSS_Evaluate_P_d2_lhss(y_res, ctxts, eval, M1, degree_f);
    }
    
    cout << "=== Done ===" << endl;
    
    // ---- Also run original HSSRLWE for comparison ----
    cout << "\n=== Original HSSRLWE Benchmark (for comparison) ===" << endl;
    
    PKE_Para pkePara;
    vec_ZZ_pX pkePk, pkeSk;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    pkePara.msg_bit = 32;
    pkePara.num_data = msg_num;
    pkePara.d = degree_f;
    
    {
        ScopeTimer t("Original: PKE_Gen");
        PKE_Gen(pkePara, pkePk, pkeSk);
    }
    
    ZZ_pXModulus modulus(pkePara.xN);
    vec_ZZ_pX ek_orig1, ek_orig2;
    ek_orig1.SetLength(2);
    ek_orig2.SetLength(2);
    
    {
        ScopeTimer t("Original: HssGen");
        HssGen(ek_orig1, ek_orig2, pkePara, pkeSk);
    }
    
    Vec<vec_ZZ_pX> Ix_orig;
    Ix_orig.SetLength(msg_num);
    {
        ScopeTimer t("Original: HSS_Enc (" + to_string(msg_num) + " msgs)");
        for (int i = 0; i < msg_num; i++)
        {
            vec_ZZ_pX ct;
            ZZ x;
            conv(x, static_cast<unsigned long>(msgs[i]));
            HSS_Enc(ct, pkePara, modulus, pkePk, x);
            Ix_orig[i] = ct;
        }
    }
    
    // Convert input for M1
    vec_ZZ_pX M1_orig;
    M1_orig.SetLength(2);
    {
        ScopeTimer t("Original: ConvertInput (M1)");
        HssConvertInput(M1_orig, pkePara, modulus, ek_orig1, pkePk);
    }
    
    vec_ZZ_pX y_res_orig;
    int prf_key = 0;
    {
        ScopeTimer t("Original: HssEvaluatePolyD2");
        HssEvaluatePolyD2(y_res_orig, 0, Ix_orig, pkePara, modulus, ek_orig1, prf_key, degree_f, M1_orig);
    }
    
    cout << "\n=== Benchmark complete ===" << endl;
    
    return 0;
}
