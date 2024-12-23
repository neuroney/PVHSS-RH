#include "PVHSS.h"

void KeyGen(PVHSSPara &param, PVHSS_EK &ek0, PVHSS_EK &ek1)
{
    HSS_Gen(param.pk, ek0, ek1, param.skLen, param.vkLen);
    Biv_ComGen(param.ck);

    bn_t A;
    bn_new(A);
    ep2_new(param.ck.g2_A);
    ZZ2bn(A, (ek1[1]-ek0[1]) % param.pk.N);
    ep2_mul_gen(param.ck.g2_A, A); // g_2^A
}

void ProbGen(vector<PVHSS_CT> &Ix, const PVHSSPara &param, const vec_ZZ &x)
{
    Ix.clear();
    int i;
    HSS_CT CTtmp;
    for (i = 0; i < x.length(); ++i)
    {
        HSS_Input(CTtmp, param.pk, x[i]);
        Ix.push_back(CTtmp);
    }
}

void evaluate(bn_t y, PROOF &proof, int b, const PVHSSPara &param, const PVHSS_EK &ekb, vector<PVHSS_CT> &Ix, vector<vector<int>> F_TEST)
{
    int prf_key = 0;
    HSS_MV y_b_res; 
    HSS_Evaluate(y_b_res, b, Ix, param.pk, ekb, prf_key, F_TEST);
   
    bn_null(y);
    bn_new(y);
    ZZ2bn(y, y_b_res[0] % param.ck.g1_order_ZZ);

    BivPE_Prove(proof, b, y_b_res[0], y_b_res[2], param.ck, prf_key);
}

bool verify(const PROOF &pi0, const PROOF &pi1, const CK &ck)
{
    fp12_t fp12tmp1, fp12tmp2, lefthand;
    fp12_new(fp12tmp1);
    fp12_new(fp12tmp2);
    fp12_new(lefthand);

    // Compute e(d_1/d_0, g)
    ep_t eptmp;
    ep_new(eptmp);
    ep_sub(eptmp, pi1.D, pi0.D);
    pp_map_oatep_k12(fp12tmp1, eptmp, ck.g2);

    // Compute e(h^upsilon_0, g)
    ep_mul(eptmp, ck.h, pi0.upsilon);
    pp_map_oatep_k12(fp12tmp2, eptmp, ck.g2);

    // Compute lefthand
    fp12_mul(lefthand, fp12tmp1, fp12tmp2);
    fp12_mul(lefthand, lefthand, pi1.e);

    // Compute e(g, g)^{Ay}
    fp12_t righthand;
    fp12_new(righthand);
    ep_sub(eptmp, pi1.C, pi0.C);
    pp_map_oatep_k12(fp12tmp1, eptmp, ck.g2_A);

    // Compute e(h^upsilon_1, g)
    ep_mul(eptmp, ck.h, pi1.upsilon);
    pp_map_oatep_k12(fp12tmp2, eptmp, ck.g2);

    // Compute righthand
    fp12_mul(righthand, fp12tmp1, fp12tmp2);
    fp12_mul(righthand, righthand, pi0.e);

    // Compare lefthand and righthand
    return (fp12_cmp(lefthand, righthand) == RLC_EQ);
}

bool verify(bn_t y0, bn_t y1, const PROOF &pi0, const PROOF &pi1, const CK &ck)
{
    // y = y_1 - y_0
    bn_t y;
    bn_new(y);
    bn_sub(y, y1, y0);

    // t11 = d_1 / d_0
    ep_t t11;
    ep_new(t11);
    ep_sub(t11, pi1.D, pi0.D);

    // t2 = e(d_1/d_0,g)
    fp12_t t1, t2, t3;
    fp12_new(t1);
    fp12_new(t2);
    fp12_new(t3);
    pp_map_oatep_k12(t2, t11, ck.g2);

    ep_t t22;
    ep_new(t22);
    ep_mul(t22, ck.h, pi0.upsilon);
    pp_map_oatep_k12(t1, t22, ck.g2);

    fp12_mul(t3, t1, t2);
    fp12_mul(t3, t3, pi1.e);

    // t3 = e(g,g)^{-Ay}
    ep_t c11;
    ep_new(c11);
    ep_mul(c11, ck.g1, y);
    pp_map_oatep_k12(t1, c11, ck.g2_A);

    ep_t c22;
    ep_new(c22);
    ep_mul(c22, ck.h, pi1.upsilon);
    pp_map_oatep_k12(t2, t22, ck.g2);

    fp12_mul(t2, t1, t2);
    fp12_mul(t2, t2, pi0.e);

    return (fp12_cmp(t2, t3) == RLC_EQ);
}

void dec(bn_t y, bn_t y0, bn_t y1)
{
    bn_new(y);
    bn_sub(y, y1, y0);
}
void PVHSS_ACC_TEST(int msg_num, int degree_f)
{
    PVHSSPara param;
    PVHSS_EK ek0, ek1;
    param.skLen = 1024;
    param.vkLen = 256;
    param.msg_bits = 32;
    param.degree_f = degree_f;
    param.msg_num = msg_num;

    double time = GetTime();
    KeyGen(param, ek0, ek1);
    time = GetTime() - time;
    cout << "KeyGen algo time: " << time * 1000 << " ms\n";

    vec_ZZ X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.msg_bits);
    }

    vector<PVHSS_CT>  Ix;
    time = GetTime();
    ProbGen(Ix, param, X);
    time = GetTime() - time;
    cout << "ProbGen algo time: " << time * 1000 << " ms\n";

    vector<vector<int>> F_TEST;
    Random_Func(F_TEST, msg_num, degree_f);

    bn_t y0, y1;
    ep_t g1_y0, g1_y1;
    PROOF pi0, pi1;
    time = GetTime();
    evaluate(y0, pi0, 0, param, ek0, Ix, F_TEST);
    time = GetTime() - time;
    cout << "Eval0 algo time: " << time * 1000 << " ms\n";

    time = GetTime();
    evaluate(y1, pi1, 1, param, ek1, Ix, F_TEST);
    time = GetTime() - time;
    cout << "Eval1 algo time: " << time * 1000 << " ms\n";

    bool res_acc;
    time = GetTime();
    res_acc = verify(pi0, pi1, param.ck);
    time = GetTime() - time;
    cout << "Veri algo time: " << time * 1000 << " ms\n";
    if (res_acc)
    {
        printf("****************** Verification Passed ******************\n");
    }
    else
    {
        printf("****************** Verification Failed ********************\n");
    }

    ZZ y_native, y_eval;
    bn_t y_eval_bn;
    bn_new(y_eval_bn);
    bn_sub(y_eval_bn, y1, y0);
    bn2ZZ(y_eval, y_eval_bn);
    y_eval = y_eval % param.ck.g1_order_ZZ;

    NativeEval(y_native, param.degree_f, msg_num, X, param.ck.g1_order_ZZ, F_TEST);
    cout << "True result: " << y_native << endl;
    cout << "Eval result: " << y_eval << endl;
    core_clean();
}