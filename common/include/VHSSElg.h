#pragma once
#include "elgamal.h"


typedef Elgamal_PK VHSSElg_PK;
typedef array<ZZ, 3> VHSSElg_EK;
typedef array<Elgamal_CT, 2> VHSSElg_CT;
typedef array<ZZ, 4> VHSSElg_MV;


void VHSSElg_Gen(VHSSElg_PK &pk, VHSSElg_EK &ek0, VHSSElg_EK &ek1, int skLen, int vkLen);
void VHSSElg_Input(VHSSElg_CT &I, const VHSSElg_PK &pk, const ZZ &x);
void VHSSElg_ConvertInput(VHSSElg_MV &Mx, int idx, const VHSSElg_PK &pk, const VHSSElg_EK &ek, const VHSSElg_CT &Ix, int &prf_key);
void VHSSElg_Mul(VHSSElg_MV &Mz, int idx, const VHSSElg_PK &pk, const VHSSElg_CT &Ix, const VHSSElg_MV &My, int &prf_key);
void VHSSElg_DDLog(ZZ &z, const VHSSElg_PK &pk, const ZZ &g);
void VHSSElg_AddMemory(VHSSElg_MV &Mz, const VHSSElg_PK &pk, const VHSSElg_MV &Mx, const VHSSElg_MV &My);
void VHSSElg_Evaluate(VHSSElg_MV &y_b_res, int b, const vector<VHSSElg_CT> &Ix, const VHSSElg_PK &pk, const VHSSElg_EK &ekb, int &prf_key, vector<vector<int>> F_TEST);
void VHSSElg_Evaluate_P_d(VHSSElg_MV &y_b_res, int b, const vector<VHSSElg_CT> &Ix, const VHSSElg_PK &pk, const VHSSElg_EK &ekb, int &prf_key, int degree_f);
void VHSSElg_Evaluate_P_d2(VHSSElg_MV &y_b_res, int b, const vector<VHSSElg_CT> &Ix, const VHSSElg_PK &pk, const VHSSElg_EK &ekb, int &prf_key, int degree_f);