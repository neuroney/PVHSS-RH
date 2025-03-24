#include "RLWEPi1.h"

void Setup(PVHSSPara &param, vec_ZZ_pX &pkePk,
    int msg_num, int degree_f)
{
    double time = GetTime();
    param.pkePara.msg_bit = 32;
    param.pkePara.num_data = msg_num;
    param.pkePara.d = degree_f;
    vec_ZZ_pX pkeSk;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    PKE_Gen(param.pkePara, pkePk, pkeSk);
    ZZ_pXModulus modulus(param.pkePara.xN);
    VHSS_Gen(param.vhssPara, param.pkePara, modulus, pkeSk);
    cout << "VHSS Setup algorithm time: " << (GetTime() - time) * 1000 << " ms\n";
    Ped_ComGen(param.ck);
    RandomBits(param.sk_alpha, 32);
    ZZ_p A_ZZ;
    eval(A_ZZ, param.vhssPara.alpha, conv<ZZ_p>(param.sk_alpha));

    bn_t A;
    bn_new(A);
    ep2_new(param.ck.g2_A);
    ZZ2bn(A, conv<ZZ>(A_ZZ) % param.ck.g1_order_ZZ);
    ep2_mul_gen(param.ck.g2_A, A); // g_2^A
}

void KeyGen(PVHSSPara &param, PVHSS_SK &sk, ZZ_pXModulus modulus, vec_ZZ_pX pkePk)
{
    RandomBnd(sk, param.ck.g1_order_ZZ);
    VHSS_Enc(param.pk_f, param.pkePara, modulus, pkePk, sk);
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
    pi.y= conv<ZZ>(yb) % param.ck.g1_order_ZZ;

    bn_t delta;
    Ped_Com(pi.D, delta, param.ck, conv<ZZ>(Yb) % param.ck.g1_order_ZZ);

    ep_t h_delta;
    ep_new(h_delta);
    ep_mul(h_delta, param.ck.h, delta);

    fp12_new(pi.e);
    pp_map_oatep_k12(pi.e, h_delta, param.ck.g2);
}

void Compute(PROOF &proof, int b, const PVHSSPara &param, const vec_ZZ_pX &ek1, const vec_ZZ_pX &ek2, vector<PVHSS_CT> &Ix, ZZ_pXModulus modulus, const vec_ZZ_pX &M1, vec_ZZ_pX &M2, vector<vector<int>> F_TEST)
{
    int prf_key = 0;
    PVHSS_MV y_b_res, Y_b_res, sk_b, SK_b;

    // HSS_Evaluate(y_b_res, b, Ix, param.pk, ekb, prf_key, F_TEST);
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
    double time = time = GetTime();
    if (b==0)
    {
        HSS_ConvertInput(sk_b, param.pkePara, modulus, param.vhssPara.vhssEk_1, param.pk_f);
        HSS_ConvertInput(SK_b, param.pkePara, modulus, param.vhssPara.vhssEk_3, param.pk_f);
    } else {
        HSS_ConvertInput(sk_b, param.pkePara, modulus, param.vhssPara.vhssEk_2, param.pk_f);
        HSS_ConvertInput(SK_b, param.pkePara, modulus, param.vhssPara.vhssEk_4, param.pk_f);
    }
    HSS_AddMemory(y_b_res, y_b_res, sk_b);
    HSS_AddMemory(Y_b_res, Y_b_res, SK_b);
    Prove(proof, b, y_b_res, Y_b_res, param);
    cout << "Prove algorithm time: " << (GetTime() - time) * 1000 << " ms\n";
}

bool Verify(const PROOF &pi0, const PROOF &pi1, const CK &ck)
{
    fp12_t righthand, lefthand;
    fp12_new(righthand);
    fp12_new(lefthand);

    // Compute e(d_1/d_0, g)
    ep_t eptmp;
    ep_new(eptmp);
    ep_sub(eptmp, pi1.D, pi0.D);
    pp_map_oatep_k12(lefthand, eptmp, ck.g2);
    fp12_mul(lefthand, lefthand, pi0.e);

    // Compute e(g, g)^{Ay}
    bn_t y_bn;
    bn_new(y_bn);
    ZZ2bn(y_bn, (pi1.y-pi0.y) % ck.g1_order_ZZ);
    ep_mul(eptmp, ck.g1, y_bn);
    pp_map_oatep_k12(righthand, eptmp, ck.g2_A);
    fp12_mul(righthand, righthand, pi1.e);

    // Compare lefthand and righthand
    return (fp12_cmp(lefthand, righthand) == RLC_EQ);
}

void Decode(ZZ& y, const PROOF &pi0, const PROOF &pi1, const PVHSS_SK sk)
{
    add(y, pi0.y, pi1.y);
    sub(y, y, sk);
}