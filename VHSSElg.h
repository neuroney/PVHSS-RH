#pragma once
#include "helper.h"

typedef struct
{
    ZZ f;
    ZZ g;
    ZZ h;
    ZZ N;
    ZZ N2;
} Elgamal_PK;

typedef Elgamal_PK HSS_PK;
typedef array<ZZ, 2> Elgamal_CT;
typedef ZZ Elgamal_SK;
typedef array<ZZ, 3> HSS_EK;
typedef array<Elgamal_CT, 2> HSS_CT;
typedef array<ZZ, 4> HSS_MV;

void Elgamal_Gen(Elgamal_PK &pk, Elgamal_SK &d, int skLen);
void Elgamal_Enc(Elgamal_CT &ct, const Elgamal_PK &pk, const ZZ &x);
void Elgamal_skEnc(Elgamal_CT &ct, const Elgamal_PK &pk, const ZZ &x);
void Elgamal_Dec(ZZ &x, const Elgamal_PK &pk, const Elgamal_SK &sk, const Elgamal_CT &ct);

void HSS_Gen(HSS_PK &pk, HSS_EK &ek0, HSS_EK &ek1, int skLen, int vkLen);
void HSS_Input(HSS_CT &I, const HSS_PK &pk, const ZZ &x);
void HSS_ConvertInput(HSS_MV &Mx, int idx, const HSS_PK &pk, const HSS_EK &ek, const HSS_CT &Ix, int &prf_key);
void HSS_Mul(HSS_MV &Mz, int idx, const HSS_PK &pk, const HSS_CT &Ix, const HSS_MV &My, int &prf_key);
void HSS_DDLog(ZZ &z, const HSS_PK &pk, const ZZ &g);
void HSS_AddMemory(HSS_MV &Mz, const HSS_PK &pk, const HSS_MV &Mx, const HSS_MV &My);
void HSS_Evaluate(HSS_MV &y_b_res, int b, const vector<HSS_CT> &Ix, const HSS_PK &pk, const HSS_EK &ekb, int &prf_key, vector<vector<int>> F_TEST);