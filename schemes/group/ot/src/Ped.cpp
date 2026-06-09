#include "Ped.h"

void Ped_ComGen(CK &ck)
{
    core_init();
    ep_curve_init();
    ep_param_set(B12_P381);
    ep2_curve_init();
    ep2_curve_set_twist(RLC_EP_MTYPE);

    ep_new(ck.g1);
    ep2_new(ck.g2);
    ep_curve_get_gen(ck.g1);
    ep2_curve_get_gen(ck.g2);

    bn_new(ck.g1_order);
    ep_curve_get_ord(ck.g1_order);
    bn2ZZ(ck.g1_order_ZZ, ck.g1_order);

    ep_new(ck.h);

    bn_t z;
    bn_new(z);
    bn_rand_mod(z, ck.g1_order);
    ep_mul_gen(ck.h, z);
}

void Ped_Com(ep_t C, bn_t rho, const CK &ck, const ZZ &x_ZZ)
{
    bn_t x;
    bn_new(x);
    ZZ2bn(x, x_ZZ % ck.g1_order_ZZ);

    ep_null(C);
    ep_new(C);

    ep_t t1, t2;
    ep_new(t1);
    ep_new(t2);

    ep_mul(t1, ck.h, rho); // h^r
    ep_mul(t2, ck.g1, x);  // g1^x
    ep_add(C, t1, t2);   // c= h^r g1^x
}

bool Ped_OpenVer(const CK &ck, const ep_t C, const ZZ &x_ZZ, bn_t rho)
{
    bn_t x;
    bn_new(x);
    ZZ2bn(x, x_ZZ % ck.g1_order_ZZ);

    ep_t t1, t2, cc;
    ep_new(t1);
    ep_new(t2);
    ep_new(cc);

    ep_mul(t1, ck.h, rho);
    ep_mul(t2, ck.g1, x);
    ep_add(cc, t1, t2);

    return (ep_cmp(C, cc) == RLC_EQ); //&& Ped_ComVer(C, ck));
}

void Ped_Prove(PROOF &pi, int b, const ZZ &yb, const ZZ &Yb, const CK &ck, int &prf_key, bn_t rho)
{
    pi.y = yb % ck.g1_order_ZZ;
    Ped_Com(pi.D, rho, ck, Yb);
}