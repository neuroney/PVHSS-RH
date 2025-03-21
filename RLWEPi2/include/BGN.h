#pragma once
#include "helper.h"
#include "relic_cp_bgn.h"

typedef struct
{
    ZZ g1_order_ZZ; // g2_order_ZZ;
    bn_t g1_order; // g2_order;
	bgn_t pub;
} CK;

typedef struct
{
    ep_t C[2];
    ep_t D[2];
    fp12_t e;  
} PROOF;

void BGN_ComGen(CK &ck, bgn_t &sk);
void BGN_Com(g1_t C[2], bn_t rho, const CK &ck, const ZZ &x_ZZ);

