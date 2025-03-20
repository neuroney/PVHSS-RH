#pragma once
#include "helper.h"

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

void GenData(Data &data, PKE_Para pkePara, vec_ZZ_pX pkePk);
void SetPara(PKE_Para &pkePara);
void PKE_Gen(PKE_Para &pkePara, vec_ZZ_pX &pkePk, vec_ZZ_pX &pkeSk);
void PKE_Enc(vec_ZZ_pX &c, const PKE_Para pkePara, ZZ_pXModulus modulus, vec_ZZ_pX pkePk, const ZZ &x);
void PKE_OKDM(vec_ZZ_pX &C, const PKE_Para &pkePara, ZZ_pXModulus &modulus, vec_ZZ_pX &pkePk, const ZZ &x);
void PKE_DDec(vec_ZZ_pX &db, const PKE_Para pkePara, ZZ_pXModulus modulus, vec_ZZ_pX pkeSk, vec_ZZ_pX C);
void HSS_Gen(vec_ZZ_pX &hssEk_1, vec_ZZ_pX &hssEk_2, PKE_Para pkePara, vec_ZZ_pX pkeSk);
void HSS_Enc(vec_ZZ_pX &C, const PKE_Para &pkePara, ZZ_pXModulus &modulus, vec_ZZ_pX &pkePk, const ZZ &x);
void HSS_Mult(vec_ZZ_pX &db, const PKE_Para pkePara, ZZ_pXModulus modulus, vec_ZZ_pX pkeSk, vec_ZZ_pX C);
void HSS_ConvertInput(ZZ_pX &tb_y, const PKE_Para pkePara, ZZ_pXModulus modulus, vec_ZZ_pX ek, vec_ZZ_pX C_X);
void HSS_Eval(ZZ_pX &tb_y, int b, PKE_Para pkePara, ZZ_pXModulus modulus, vec_ZZ_pX ek, Vec<vec_ZZ_pX> C_X,Vec<vec_ZZ_pX> PRF, int &prfkey);
void f(vec_ZZ_pX &tb, int b, int d, int num_data, int loop, int beg_ind, int *ind_var,
       PKE_Para pkePara, ZZ_pXModulus modulus, vec_ZZ_pX ek, Vec<vec_ZZ_pX> C_X, Vec<vec_ZZ_pX> PRF, int &prfkey);


void HSS_Evaluate_P_d2(vec_ZZ_pX &y_b_res, int b, const vector<vec_ZZ_pX> &Ix, const PKE_Para &pkePara, ZZ_pXModulus modulus, const vec_ZZ_pX &pkeSk, int &prf_key, int degree_f);