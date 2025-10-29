#pragma once
#include "helper.h"
#include "relic_cp_decped.h"

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
} PROOF;

void DecPed_ComGen(CK &ck, bgn_t &sk);
void DecPed_Com(g1_t C[2], bn_t rho, const CK &ck, const ZZ &x_ZZ);

