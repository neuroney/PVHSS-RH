#pragma once
#include "helper.h"

typedef struct
{
    ep2_t g2_A;
    ZZ g1_order_ZZ; // g2_order_ZZ;
    bn_t g1_order; // g2_order;

    ep_t h; 
    ep_t g1;
    ep2_t g2;
} CK;

typedef struct
{
    ZZ y;
    ep_t D;
    fp12_t e;  
} PROOF;

void Ped_ComGen(CK &ck);
void Ped_Com(ep_t C, bn_t rho, const CK &ck, const ZZ &x_ZZ);
bool Ped_OpenVer(const CK &ck, const ep_t C, const ZZ &x_ZZ, bn_t rho);
void Ped_Prove(PROOF &pi, int b, const ZZ &yb, const ZZ &Yb, const CK &ck, int &prf_key);

