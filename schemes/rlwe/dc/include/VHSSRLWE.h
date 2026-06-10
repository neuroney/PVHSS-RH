#pragma once
#include "helper.h"

namespace pvhss { namespace rlwe { namespace dc {

struct PKE_Para
{
    int N, msg_bit, p_bit, q_bit;
    ZZ msg, p, q, coeff, twice_p, twice_q, half_p;
    ZZ_pX xN;
    int hsk = 64;
    int d;
    int num_data;
};

struct Data {
    vec_ZZ X;
    Vec<vec_ZZ_pX> C_X;
    Vec<vec_ZZ_pX> PRF;
};

struct VHSS_Para
{
    vec_ZZ_pX vhssEk_1;
    vec_ZZ_pX vhssEk_2;
    vec_ZZ_pX vhssEk_3;
    vec_ZZ_pX vhssEk_4;
    ZZ_pX alpha;
};

// Workspace for PKE_DDec to avoid repeated temporary allocations
struct PKEWorkspace {
    ZZ coeff;
    ZZX temp;
    ZZ_pX temp1;
    ZZ_pX temp2;
};

void GenerateData(Data &data, const PKE_Para& pkePara, const vec_ZZ_pX& pkePk);
void SetParams(PKE_Para &pkePara);
void PKE_Gen(PKE_Para &pkePara, vec_ZZ_pX &pkePk, vec_ZZ_pX &pkeSk);
void PKE_Enc(vec_ZZ_pX &c, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkePk, const ZZ &x);
void PKE_OKDM(vec_ZZ_pX &C, const PKE_Para &pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkePk, const ZZ &x);
void PKE_DDec(vec_ZZ_pX &db, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkeSk, const vec_ZZ_pX& C);
void HssGen(vec_ZZ_pX &hssEk_1, vec_ZZ_pX &hssEk_2, const PKE_Para& pkePara, const vec_ZZ_pX& pkeSk);
void VHSS_Enc(vec_ZZ_pX &C, const PKE_Para &pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkePk, const ZZ &x);
void VHSS_Mult(vec_ZZ_pX &db, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkeSk, const vec_ZZ_pX& C);
void HssAddMemory(vec_ZZ_pX &tb, const vec_ZZ_pX &C_X, const vec_ZZ_pX& C_Y);
inline void HssAddMemoryInPlace(vec_ZZ_pX& acc, const vec_ZZ_pX& x) {
    acc[0] += x[0];
    acc[1] += x[1];
}
ZZ HssOutputCoeff(const ZZ_p& coeff, const PKE_Para& pkePara, const ZZ& output_mod);
ZZ HssOutputPolyAtTwo(const ZZ_pX& poly, const PKE_Para& pkePara, const ZZ& output_mod);
void HssConvertInput(vec_ZZ_pX &tb_y, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& ek, const vec_ZZ_pX& C_X);


void HssEvaluatePolyD2(vec_ZZ_pX &y_b_res, int b, const vector<vec_ZZ_pX> &Ix, const PKE_Para &pkePara, ZZ_pXModulus modulus, const vec_ZZ_pX &pkeSk, int &prf_key, int degree_f,
    const vec_ZZ_pX & M1);

    void VHSS_Gen(VHSS_Para &vhssPara, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkeSk);

}}} // namespace pvhss::rlwe::dc
