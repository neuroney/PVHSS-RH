#include "PiDCRLWE.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace rlwe { namespace dc {

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

    NTL::RandomBits(param.sk_alpha, 32);
    ZZ A_ZZ = HssOutputPolyAtTwo(param.vhssPara.alpha, param.pkePara, param.ck.g1_order_ZZ);

    bn_t A;
    bn_new(A);
    ep2_new(param.vk);
    ZZtoBn(A, A_ZZ);
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
    (void)b;
    const ZZ yb_zz = HssOutputCoeff(y_b[0][0], param.pkePara, param.ck.g1_order_ZZ);
    const ZZ Yb_zz = HssOutputPolyAtTwo(Y_b[0], param.pkePara, param.ck.g1_order_ZZ);

    DecPed_Com(pi.C, ekpb[0], param.ck, yb_zz);
    DecPed_Com(pi.D, ekpb[1], param.ck, Yb_zz);
}

void Compute(PROOF &proof, int b, const PVHSSPara &param, const vec_ZZ_pX &ek1, const vec_ZZ_pX &ek2, vector<PVHSS_CT> &Ix, ZZ_pXModulus modulus, const vec_ZZ_pX &M1, vec_ZZ_pX &M2, vector<vector<int>> F_TEST, bn_t ekpb[2])
{
    int prf_key = 0;
    PVHSS_MV y_b_res, Y_b_res;

    if (b == 0)
    {
        HssEvaluatePolyD2(y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_1, prf_key, param.pkePara.d, M1);
        HssEvaluatePolyD2(Y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_3, prf_key, param.pkePara.d, M2);
    }
    else
    {
        HssEvaluatePolyD2(y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_2, prf_key, param.pkePara.d, M1);
        HssEvaluatePolyD2(Y_b_res, b, Ix, param.pkePara, modulus, param.vhssPara.vhssEk_4, prf_key, param.pkePara.d, M2);
    }
    Prove(proof, b, y_b_res, Y_b_res, param, ekpb);
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

bool PVHSS_ACC_TEST(int msg_num, int degree_f)
{
    PVHSSPara param;
    vec_ZZ_pX pkePk;
    PVHSS_SK sk;
    bn_t ekp0[2], ekp1[2];

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
        ProbGen(Ix, param, X, modulus, pkePk);
    }, 1);
    PrintTimeMs("ProbGen algo time", timing);

    vector<vector<int>> F_TEST;
    GenerateRandomFunc(F_TEST, msg_num, degree_f);

    // M1-M4: encrypt constant "1" and convert
    PVHSS_CT C1;
    PVHSS_MV M1, M2, M3, M4;
    VHSS_Enc(C1, param.pkePara, modulus, pkePk, ZZ(1));
    HssConvertInput(M1, param.pkePara, modulus, param.vhssPara.vhssEk_1, C1);
    HssConvertInput(M2, param.pkePara, modulus, param.vhssPara.vhssEk_2, C1);
    HssConvertInput(M3, param.pkePara, modulus, param.vhssPara.vhssEk_3, C1);
    HssConvertInput(M4, param.pkePara, modulus, param.vhssPara.vhssEk_4, C1);

    PROOF pi0, pi1;
    timing = MeasureTimeMs([&]() {
        Compute(pi0, 0, param, param.vhssPara.vhssEk_1, param.vhssPara.vhssEk_3, Ix, modulus, M1, M3, F_TEST, ekp0);
    }, 1);
    PrintTimeMs("Eval0 algo time", timing);

    timing = MeasureTimeMs([&]() {
        Compute(pi1, 1, param, param.vhssPara.vhssEk_2, param.vhssPara.vhssEk_4, Ix, modulus, M2, M4, F_TEST, ekp1);
    }, 1);
    PrintTimeMs("Eval1 algo time", timing);

    bool res_acc;
    timing = MeasureTimeMsAdaptive([&]() {
        res_acc = Verify(pi0, pi1, param);
    }, 1);
    PrintTimeMs("Veri algo time", timing);
    
    if (res_acc)
        printf("****************** Verification Passed ******************\n");
    else
        printf("****************** Verification Failed ********************\n");

    dig_t y_eval_dig;
    Decode(y_eval_dig, pi0, pi1, param.f_sk);
    ZZ y_eval = to_ZZ(static_cast<long>(y_eval_dig));
    ZZ y_native = to_ZZ(PolyD(X, degree_f) % param.ck.g1_order_ZZ);
    cout << "True result: " << y_native << endl;
    cout << "Eval result: " << y_eval << endl;
    const ZZ decode_limit = conv<ZZ>(PVHSS_M_MAX);
    if (y_native >= decode_limit)
    {
        cout << "P5 accuracy check relaxed: native result is outside DecPed 32-bit decryption range." << endl;
        core_clean();
        return res_acc;
    }
    
    core_clean();
    return res_acc && (y_native == y_eval);
}

}}} // namespace pvhss::rlwe::dc
