#include "PiOTRLWE.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace rlwe { namespace ot {

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
    Ped_ComGen(param.ck);
    NTL::RandomBits(param.sk_alpha, 32);
    ZZ_p A_ZZ_p;
    eval(A_ZZ_p, param.vhssPara.alpha, conv<ZZ_p>(param.sk_alpha));
    ZZ A_ZZ = conv<ZZ>(A_ZZ_p) % param.ck.g1_order_ZZ;

    bn_t A;
    bn_new(A);
    ep2_new(param.ck.g2_A);
    ZZtoBn(A, A_ZZ);
    ep2_mul_gen(param.ck.g2_A, A); // g_2^A
}

void KeyGen(PVHSSPara &param, PVHSS_SK &sk, ZZ_pXModulus modulus, vec_ZZ_pX pkePk,  bn_t ekp0, bn_t ekp1)
{
    NTL::RandomBnd(sk, param.ck.g1_order_ZZ);
    ZZ_pX sk_pX;
    EncodeBinaryPolynomial(sk_pX, sk, param.pkePara.msg_bit);
    VHSS_Enc(param.pk_f, param.pkePara, modulus, pkePk, sk_pX);

     bn_rand_mod(ekp0, param.ck.g1_order); // u_0
    bn_rand_mod(ekp1, param.ck.g1_order); // u_1

    bn_t u;
    bn_new(u);
    bn_sub(u, ekp0, ekp1);
    g1_t hu;
    g1_new(hu);
    g1_mul(hu, param.ck.h, u);
    fp12_new(param.ck.aux);

    g2_t g2;
    g2_new(g2);
    g2_get_gen(g2);
    pp_map_oatep_k12(param.ck.aux, hu, g2); // e(hu, vk)
}

void ProbGen(vector<PVHSS_CT> &Ix, const PVHSSPara &param, const vec_ZZ_pX &x, ZZ_pXModulus modulus, vec_ZZ_pX pkePk)
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

void Prove(PROOF &pi, int b, const PVHSS_MV &y_b, const PVHSS_MV &Y_b, const PVHSSPara &param, bn_t ekpb)
{
    (void)b;
    ZZ_p yb, Yb;
    eval(yb, y_b[0], conv<ZZ_p>(param.sk_alpha));
    eval(Yb, Y_b[0], conv<ZZ_p>(param.sk_alpha));
    pi.y = conv<ZZ>(yb) % param.ck.g1_order_ZZ;
    Ped_Com(pi.D, ekpb, param.ck, conv<ZZ>(Yb) % param.ck.g1_order_ZZ);
}

void Compute(PROOF &proof, int b, const PVHSSPara &param, const vec_ZZ_pX &ek1, const vec_ZZ_pX &ek2, vector<PVHSS_CT> &Ix, ZZ_pXModulus modulus, const vec_ZZ_pX &M1, vec_ZZ_pX &M2, vector<vector<int>> F_TEST, bn_t ekpb)
{
    int prf_key = 0;
    PVHSS_MV y_b_res, Y_b_res, sk_b, SK_b;

    // HssEvaluate(y_b_res, b, Ix, param.pk, ekb, prf_key, F_TEST);
    if (b == 0)
    {
        HssEvaluateMPE(y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_1, prf_key, param.pkePara.d, M1);
        HssEvaluateMPE(Y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_3, prf_key, param.pkePara.d, M2);

    }
    else
    {
        HssEvaluateMPE(y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_2, prf_key, param.pkePara.d, M1);
        HssEvaluateMPE(Y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_4, prf_key, param.pkePara.d, M2);

    }
    if (b == 0)
    {
        HssConvertInput(sk_b, param.pkePara, modulus, param.vhssPara.vhssEk_1, param.pk_f);
        HssConvertInput(SK_b, param.pkePara, modulus, param.vhssPara.vhssEk_3, param.pk_f);
    }
    else
    {
        HssConvertInput(sk_b, param.pkePara, modulus, param.vhssPara.vhssEk_2, param.pk_f);
        HssConvertInput(SK_b, param.pkePara, modulus, param.vhssPara.vhssEk_4, param.pk_f);
    }
    HssAddMemory(y_b_res, y_b_res, sk_b);
    HssAddMemory(Y_b_res, Y_b_res, SK_b);
    Prove(proof, b, y_b_res, Y_b_res, param, ekpb);
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
    fp12_mul(lefthand, lefthand, ck.aux);

    // Compute e(g, g)^{Ay}
    bn_t y_bn;
    bn_new(y_bn);
    ZZtoBn(y_bn, (pi1.y-pi0.y) % ck.g1_order_ZZ);
    ep_mul(eptmp, ck.g1, y_bn);
    pp_map_oatep_k12(righthand, eptmp, ck.g2_A);

    // Compare lefthand and righthand
    return (fp12_cmp(lefthand, righthand) == RLC_EQ);
}

void Decode(ZZ& y, const PROOF &pi0, const PROOF &pi1, const PVHSS_SK sk)
{
    add(y, pi0.y, pi1.y);
    sub(y, y, sk);
}

bool PVHSS_ACC_TEST(int msg_num, int degree_f)
{
    PVHSSPara param;
    vec_ZZ_pX pkePk;
    PVHSS_SK sk;
    bn_t ekp0, ekp1;

    TimingResult timing = MeasureTimeMs([&]() {
        Setup(param, pkePk, msg_num, degree_f);
    }, 1);
    PrintTimeMs("Setup algo time", timing);

    ZZ_pXModulus modulus(param.pkePara.xN);
    timing = MeasureTimeMs([&]() {
        KeyGen(param, sk, modulus, pkePk, ekp0, ekp1);
    }, 1);
    PrintTimeMs("KeyGen algo time", timing);

    vec_ZZ X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        NTL::RandomBits(X[i], param.pkePara.msg_bit);
    }

    vector<PVHSS_CT> Ix;
    timing = MeasureTimeMs([&]() {
        vec_ZZ_pX Xp; Xp.SetLength(msg_num);
        for (int i = 0; i < msg_num; ++i) EncodeBinaryPolynomial(Xp[i], X[i], param.pkePara.msg_bit);
        ProbGen(Ix, param, Xp, modulus, pkePk);
    }, 1);
    PrintTimeMs("ProbGen algo time", timing);

    vector<vector<int>> F_TEST;
    GenerateRandomFunc(F_TEST, msg_num, degree_f);

    PVHSS_CT C1;
    vec_ZZ_pX M1_0, M1_1, M3_0, M3_1;
    ZZ_pX one; EncodeBinaryPolynomial(one, ZZ(1), 1);
    VHSS_Enc(C1, param.pkePara, modulus, pkePk, one);
    HssConvertInput(M1_0, param.pkePara, modulus, param.vhssPara.vhssEk_1, C1);
    HssConvertInput(M1_1, param.pkePara, modulus, param.vhssPara.vhssEk_2, C1);
    HssConvertInput(M3_0, param.pkePara, modulus, param.vhssPara.vhssEk_3, C1);
    HssConvertInput(M3_1, param.pkePara, modulus, param.vhssPara.vhssEk_4, C1);

    PROOF pi0, pi1;
    timing = MeasureTimeMs([&]() {
        Compute(pi0, 0, param, param.vhssPara.vhssEk_1, param.vhssPara.vhssEk_2, Ix, modulus, M1_0, M3_0, F_TEST, ekp0);
    }, 1);
    PrintTimeMs("Eval0 algo time", timing);

    timing = MeasureTimeMs([&]() {
        Compute(pi1, 1, param, param.vhssPara.vhssEk_1, param.vhssPara.vhssEk_2, Ix, modulus, M1_1, M3_1, F_TEST, ekp1);
    }, 1);
    PrintTimeMs("Eval1 algo time", timing);

    bool res_acc;
    timing = MeasureTimeMsAdaptive([&]() {
        res_acc = Verify(pi0, pi1, param.ck);
    }, 1);
    PrintTimeMs("Veri algo time", timing);
    
    if (res_acc)
        printf("****************** Verification Passed ******************\n");
    else
        printf("****************** Verification Failed ********************\n");

    ZZ y_native, y_eval;
    Decode(y_eval, pi0, pi1, sk);

    y_eval = y_eval % param.ck.g1_order_ZZ;
    if (y_eval < 0)
        y_eval += param.ck.g1_order_ZZ;
    y_native = MPE(X, degree_f) % param.ck.g1_order_ZZ;
    cout << "True result: " << y_native << endl;
    cout << "Eval result: " << y_eval << endl;
    const bool result_matches = (y_native == y_eval);
    if (!result_matches)
        cout << "P5 accuracy check failed: decoded result differs from native computation." << endl;
    
    core_clean();
    return res_acc && result_matches;
}

}}} // namespace pvhss::rlwe::ot
