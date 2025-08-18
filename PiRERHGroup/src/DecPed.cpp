#include "DecPed.h"

void DecPed_ComGen(CK &ck, bgn_t &sk)
{
    core_init();
    ep_curve_init();
    ep_param_set(B12_P381);
    ep2_curve_init();
    ep2_curve_set_twist(RLC_EP_MTYPE);


    bgn_null(ck.pub);
	bgn_null(sk);
	bgn_new(ck.pub);
	bgn_new(sk);
    cp_bgn_gen(ck.pub, sk);

    bn_null(ck.g1_order);
    bn_new(ck.g1_order);
    ep_curve_get_ord(ck.g1_order);
    bn2ZZ(ck.g1_order_ZZ, ck.g1_order);

}

void DecPed_Com(g1_t C[2], bn_t rho, const CK &ck, const ZZ &x_ZZ)
{
    bn_null(rho);
    bn_new(rho);

    bn_t x;
    bn_new(x);
    ZZ2bn(x, x_ZZ % ck.g1_order_ZZ);

    g1_null(C[0]);
	g1_null(C[1]);
    g1_new(C[0]);
    g1_new(C[1]);
    cp_bgn_enc3(C, rho, x, ck.pub);
}