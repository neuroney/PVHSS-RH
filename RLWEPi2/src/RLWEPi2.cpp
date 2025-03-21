#include "RLWEPi2.h"

void Setup(PVHSSPara &param, vec_ZZ_pX &pkePk,
           int msg_num, int degree_f)
{
    param.pkePara.msg_bit = 32;
    param.pkePara.num_data = msg_num;
    param.pkePara.d = degree_f;
    vec_ZZ_pX pkeSk;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    PKE_Gen(param.pkePara, pkePk, pkeSk);
    ZZ_pXModulus modulus(param.pkePara.xN);
    VHSS_Gen(param.vhssPara, param.pkePara, modulus, pkeSk);
    BGN_ComGen(param.ck, param.f_sk);

    RandomBits(param.sk_alpha, 32);
    ZZ_p A_ZZ;
    eval(A_ZZ, param.vhssPara.alpha, conv<ZZ_p>(param.sk_alpha));

    bn_t A;
    bn_new(A);
    ep2_new(param.ck.g2_A);
    ZZ2bn(A, conv<ZZ>(A_ZZ) % param.ck.g1_order_ZZ);
    ep2_mul_gen(param.vk, A); // g_2^A
}

void KeyGen(PVHSSPara &param, PVHSS_SK &sk, ZZ_pXModulus modulus, vec_ZZ_pX pkePk)
{
    return;
}

void ProbGen(vector<PVHSS_CT> &Ix, const PVHSSPara &param, const vec_ZZ &x, ZZ_pXModulus modulus, vec_ZZ_pX pkePk)
{
    Ix.clear();
    int i;
    PVHSS_CT CTtmp;
    for (i = 0; i < x.length(); ++i)
    {
        VHSS_Enc(CTtmp, param.pkePara, modulus, pkePk, x[i]);
        Ix.push_back(CTtmp);
    }
}

void Prove(PROOF &pi, int b, const PVHSS_MV &y_b, const PVHSS_MV &Y_b, const PVHSSPara &param)
{
    ZZ_p yb, Yb;
    eval(yb, y_b[0], conv<ZZ_p>(param.sk_alpha));
    eval(Yb, Y_b[0], conv<ZZ_p>(param.sk_alpha));

    bn_t u, v;
    fp12_t mu, delta;

    bn_new(u);
    bn_new(v);
    fp12_new(mu);
    fp12_new(delta);

    BGN_Com(pi.C, u, param.ck, conv<ZZ>(yb) % param.ck.g1_order_ZZ);
    BGN_Com(pi.D, v, param.ck, conv<ZZ>(Yb) % param.ck.g1_order_ZZ);

    ep_t h;
    ep_new(h);
    ep_mul_gen(h, u);
    pp_map_oatep_k12(mu, h, param.vk);

    ep_new(h);
    ep_mul_gen(h, v);
    g2_t g2;
    g2_new(g2);
    g2_get_gen(g2);
    pp_map_oatep_k12(delta, h, g2);

    fp12_new(pi.e);

    fp12_inv(pi.e, mu);
    fp12_mul(pi.e, delta, pi.e);
}

void Compute(PROOF &proof, int b, const PVHSSPara &param, const vec_ZZ_pX &ek1, const vec_ZZ_pX &ek2, vector<PVHSS_CT> &Ix, ZZ_pXModulus modulus, const vec_ZZ_pX &M1, vec_ZZ_pX &M2, vector<vector<int>> F_TEST)
{
    int prf_key = 0;
    PVHSS_MV y_b_res, Y_b_res;

    if (b == 0)
    {
        HSS_Evaluate_P_d2(y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_1, prf_key, param.pkePara.d, M1);
        HSS_Evaluate_P_d2(Y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_3, prf_key, param.pkePara.d, M2);
    }
    else
    {
        HSS_Evaluate_P_d2(y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_2, prf_key, param.pkePara.d, M1);
        HSS_Evaluate_P_d2(Y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_4, prf_key, param.pkePara.d, M2);
    }

    Prove(proof, b, y_b_res, Y_b_res, param);
}

bool Verify(const PROOF &pi0, const PROOF &pi1, const PVHSSPara &param)
{
    fp12_t lefthand,righthand;
    fp12_new(lefthand);
    fp12_new(righthand);

    g2_t g2;
    g2_new(g2);
    g2_get_gen(g2);
    // Compute e(d_1/d_0, g)
    ep_t eptmp;
    ep_new(eptmp);
    ep_sub(eptmp, pi1.D[0], pi0.D[0]);
    pp_map_oatep_k12(lefthand, eptmp, g2);

    // Compute e(g, g)^{Ay}
    ep_new(eptmp);
    ep_sub(eptmp, pi1.C[0], pi0.C[0]);
    pp_map_oatep_k12(righthand, eptmp, param.vk);
    fp12_mul(righthand, pi0.e, righthand);
    fp12_mul(righthand, pi1.e, righthand);

    // Compare lefthand and righthand
    return (fp12_cmp(lefthand, righthand) == RLC_EQ);
}

void Decode(dig_t &y, const PROOF &pi0, const PROOF &pi1, const PVHSS_SK sk)
{
    g1_t in[2];
    g1_new(in[0]);
    g1_new(in[1]);
    g1_sub(in[0], pi1.C[0], pi0.C[0]);
    g1_sub(in[1], pi1.C[1], pi0.C[1]);

    cp_bgn_dec1(y, in, sk);
}