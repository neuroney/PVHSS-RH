#include "PiDCRLWE.h"

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
    
    DecPed_ComGen(param.ck, param.f_sk);

    RandomBits(param.sk_alpha, 32);
    ZZ_p A_ZZ;
    eval(A_ZZ, param.vhssPara.alpha, conv<ZZ_p>(param.sk_alpha));

    bn_t A;
    bn_new(A);
    ep2_new(param.ck.g2_A);
    ZZ2bn(A, conv<ZZ>(A_ZZ) % param.ck.g1_order_ZZ);
    ep2_mul_gen(param.vk, A); // g_2^A
    
}

void KeyGen(PVHSSPara &param, PVHSS_SK &sk, ZZ_pXModulus modulus, vec_ZZ_pX pkePk, bn_t ekp0[2], bn_t ekp1[2])
{
    bn_rand_mod(ekp0[0],param.ck.g1_order); //u_0
    bn_rand_mod(ekp0[1], param.ck.g1_order); //v_0
    bn_rand_mod(ekp1[0], param.ck.g1_order); //u_1
    bn_rand_mod(ekp1[1], param.ck.g1_order); //v_1

    bn_t u, v;
    bn_new(u);
    bn_new(v);
    bn_sub(u,ekp1[0],ekp0[0]);
    bn_sub(v,ekp1[1],ekp0[1]);
    
    g1_t hu;
    g1_new(hu);
    g1_mul_gen(hu, u);
    fp12_new(param.aux0);
    pp_map_oatep_k12(param.aux0, hu, param.vk); // e(hu, vk)

     g2_t g2;
    g2_new(g2);
    g2_get_gen(g2);

    g1_t hv;
    g1_new(hv);
    g1_mul_gen(hv, v);
    fp12_t tmp;
    fp12_new(tmp);
    pp_map_oatep_k12(tmp, hv, g2);
    fp12_inv(tmp,tmp);
    fp12_mul(param.aux0, tmp, param.aux0);

    g1_new(hu);
    g1_mul(hu, param.ck.pub->gx, u);
    fp12_new(param.aux1);
    pp_map_oatep_k12(param.aux1, hu, param.vk);

    g1_new(hv);
    g1_mul(hv, param.ck.pub->gx, v);
    fp12_new(tmp);
    pp_map_oatep_k12(tmp, hv, g2);
    fp12_inv(tmp,tmp);
    fp12_mul(param.aux1, tmp, param.aux1);
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

void Prove(PROOF &pi, int b, const PVHSS_MV &y_b, const PVHSS_MV &Y_b, const PVHSSPara &param, bn_t ekpb[2])
{
    ZZ_p yb, Yb;
    eval(yb, y_b[0], conv<ZZ_p>(param.sk_alpha));
    eval(Yb, Y_b[0], conv<ZZ_p>(param.sk_alpha));

    DecPed_Com(pi.C, ekpb[0], param.ck, conv<ZZ>(yb) % param.ck.g1_order_ZZ);
    DecPed_Com(pi.D, ekpb[1], param.ck, conv<ZZ>(Yb) % param.ck.g1_order_ZZ);
}

void Compute(PROOF &proof, int b, const PVHSSPara &param, const vec_ZZ_pX &ek1, const vec_ZZ_pX &ek2, vector<PVHSS_CT> &Ix, ZZ_pXModulus modulus, const vec_ZZ_pX &M1, vec_ZZ_pX &M2, vector<vector<int>> F_TEST, bn_t ekpb[2])
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
    //double time = time = GetTime();
    Prove(proof, b, y_b_res, Y_b_res, param, ekpb);
    //cout << "Prove algorithm time: " << (GetTime() - time) * 1000 << " ms\n";
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
    g1_t eptmp;
    g1_new(eptmp);
    g1_sub(eptmp, pi1.D[0], pi0.D[0]);
    pp_map_oatep_k12(lefthand, eptmp, g2);

    // Compute e(g, g)^{Ay}
    g1_new(eptmp);
    g1_sub(eptmp, pi1.C[0], pi0.C[0]);
    pp_map_oatep_k12(righthand, eptmp, param.vk);
    fp12_mul(lefthand, param.aux0, lefthand);

    if (fp12_cmp(lefthand, righthand) != RLC_EQ)
        return false;
    
    fp12_new(lefthand);
    fp12_new(righthand);
    g1_new(eptmp);
    g1_sub(eptmp, pi1.D[1], pi0.D[1]);
    pp_map_oatep_k12(lefthand, eptmp, g2);
    g1_new(eptmp);
    g1_sub(eptmp, pi1.C[1], pi0.C[1]);
    pp_map_oatep_k12(righthand, eptmp, param.vk);
    fp12_mul(lefthand, param.aux1, lefthand);
    return (fp12_cmp(lefthand, righthand) == RLC_EQ);
}

void Decode(dig_t &y, const PROOF &pi0, const PROOF &pi1, const PVHSS_SK sk)
{
    g1_t in[2];
    g1_new(in[0]);
    g1_new(in[1]);
    g1_sub(in[0], pi1.C[0], pi0.C[0]);
    g1_sub(in[1], pi1.C[1], pi0.C[1]);

    cp_decped_dec1(y, in, sk);
}