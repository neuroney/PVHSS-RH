#pragma once

#include <memory>
#include <vector>

// NTL & helper types
#include "helper.h"
#include <NTL/ZZ_p.h>

using NTL::ZZ;
using NTL::ZZ_pX;
using NTL::ZZ_pXModulus;
using NTL::ZZX;
using NTL::vec_ZZ;
using NTL::vec_ZZ_pX;
using NTL::Vec;

// Forward-declare lhss types (opaque pointers)
namespace lhss {
  class Client;
  struct Ciphertext;
  struct SecretKey;
  struct HSSCtxt;
  class Evaluator;
}

// --- Same structs as original, extended with internal lhss state ---

struct PKE_Para
{
    // Public fields (same as original)
    int N, msg_bit, p_bit, q_bit;
    ZZ msg, p, q, coeff, twice_p, twice_q, half_p;
    ZZ_pX xN;
    int hsk = 64;
    int d;
    int num_data;

    // Internal lhss state (opaque — do not access directly)
    void* lhss_client_ = nullptr;
    void* lhss_pk_ = nullptr;
    void* lhss_sk_ = nullptr;
    bool lhss_initialized_ = false;
};

struct Data {
    vec_ZZ X;
    Vec<vec_ZZ_pX> C_X;
    Vec<vec_ZZ_pX> PRF;
};

// Workspace for PKE_DDec (kept for API compat)
struct PKEWorkspace {
    ZZ coeff;
    ZZX temp;
    ZZ_pX temp1;
    ZZ_pX temp2;
};

// --- Same API as original HSSRLWE.h ---

void SetParams(PKE_Para &pkePara);
void PKE_Gen(PKE_Para &pkePara, vec_ZZ_pX &pkePk, vec_ZZ_pX &pkeSk);
void PKE_Enc(vec_ZZ_pX &c, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkePk, const ZZ &x);
void PKE_OKDM(vec_ZZ_pX &C, const PKE_Para &pkePara, ZZ_pXModulus &modulus, const vec_ZZ_pX& pkePk, const ZZ &x);
void PKE_DDec(vec_ZZ_pX &db, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkeSk, const vec_ZZ_pX& C);

void HssGen(vec_ZZ_pX &hssEk_1, vec_ZZ_pX &hssEk_2, const PKE_Para& pkePara, const vec_ZZ_pX& pkeSk);
void HSS_Enc(vec_ZZ_pX &C, const PKE_Para &pkePara, ZZ_pXModulus &modulus, const vec_ZZ_pX& pkePk, const ZZ &x);
void HSS_Mult(vec_ZZ_pX &db, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkeSk, const vec_ZZ_pX& C);

void HssAddMemory(vec_ZZ_pX &tb, const vec_ZZ_pX &C_X, const vec_ZZ_pX& C_Y);
inline void HssAddMemoryInPlace(vec_ZZ_pX& acc, const vec_ZZ_pX& x) {
    acc[0] += x[0];
    acc[1] += x[1];
}
void HssConvertInput(vec_ZZ_pX &tb_y, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& ek, const vec_ZZ_pX& C_X);

void HssEvaluatePolyD2(vec_ZZ_pX &y_b_res, int b, const Vec<vec_ZZ_pX> &Ix, const PKE_Para &pkePara, ZZ_pXModulus modulus, const vec_ZZ_pX &pkeSk, int &prf_key, int degree_f, const vec_ZZ_pX & M1);

// Data generation (for testing/benchmarking)
void GenerateData(Data &data, const PKE_Para& pkePara, const vec_ZZ_pX& pkePk);

// Recursive HSS evaluation (legacy)
void HSS_Eval(ZZ_pX &tb_y, int b, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& ek, const Vec<vec_ZZ_pX>& C_X, const Vec<vec_ZZ_pX>& PRF, int &prfkey);
void f(vec_ZZ_pX &tb, int b, int d, int num_data, int loop, int beg_ind, int *ind_var,
       const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& ek, const Vec<vec_ZZ_pX>& C_X, const Vec<vec_ZZ_pX>& PRF, int &prfkey);

// --- Cleanup ---
void PKE_Cleanup(PKE_Para &pkePara);
