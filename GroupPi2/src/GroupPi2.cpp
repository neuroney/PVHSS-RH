#include "GroupPi2.h"

void Setup(PVHSSPara &param, PVHSS_EK &ek0, PVHSS_EK &ek1, PVHSS_SK &sk)
{
    double time =GetTime();
    HSS_Gen(param.pk, ek0, ek1, param.skLen, param.vkLen);
    cout << "VHSS Setup algorithm time: " << (GetTime() - time) * 1000 << " ms\n";
    BGN_ComGen(param.ck, sk);

    bn_t A;
    bn_new(A);
    ep2_new(param.vk);
    ZZ2bn(A, (ek1[1] - ek0[1]) % param.pk.N);
    ep2_mul_gen(param.vk, A); // g_2^A
    
}

void KeyGen(PVHSSPara &param, PVHSS_SK &sk)
{
    return;
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

void Compute(PROOF &proof, int b, const PVHSSPara &param, const PVHSS_EK &ekb, vector<PVHSS_CT> &Ix, vector<vector<int>> F_TEST)
{
    int prf_key = 0;
    HSS_MV y_b_res;
    //HSS_Evaluate(y_b_res, b, Ix, param.pk, ekb, prf_key, F_TEST);
    HSS_Evaluate_P_d2(y_b_res, b, Ix, param.pk, ekb, prf_key, param.degree_f);
    double time =GetTime();
    Prove(proof, b, y_b_res[0], y_b_res[2], param);
    cout << "Prove algorithm time: " << (GetTime() - time) * 1000 << " ms\n";
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

void Prove(PROOF &pi, int b, const ZZ &yb, const ZZ &Yb, const PVHSSPara &param)
{
    bn_t u, v;
    fp12_t mu, delta;

    bn_new(u);
    bn_new(v);
    fp12_new(mu);
    fp12_new(delta);

    BGN_Com(pi.C, u, param.ck, yb);
    BGN_Com(pi.D, v, param.ck, Yb);

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
    if (b == 0)
    {
        fp12_inv(pi.e, delta);
        fp12_mul(pi.e, mu, pi.e);
    }
    else
    {
        fp12_inv(pi.e, mu);
        fp12_mul(pi.e, delta, pi.e);
    }
}

void PVHSS_ACC_TEST(int msg_num, int degree_f)
{
    PVHSSPara param;
    PVHSS_EK ek0, ek1;
    PVHSS_SK sk;
    param.skLen = 1024;
    param.vkLen = 256;
    param.msg_bits = 1;
    param.degree_f = degree_f;
    param.msg_num = msg_num;
    double time = GetTime();
    Setup(param, ek0, ek1, sk);
    KeyGen(param, sk);
    time = GetTime() - time;
    cout << "KeyGen algo time: " << time * 1000 << " ms\n";

    vec_ZZ X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.msg_bits);
    }

    vector<PVHSS_CT> Ix;
    time = GetTime();
    ProbGen(Ix, param, X);
    time = GetTime() - time;
    cout << "ProbGen algo time: " << time * 1000 << " ms\n";

    vector<vector<int>> F_TEST;
    Random_Func(F_TEST, msg_num, degree_f);

    PROOF pi0, pi1;
    time = GetTime();
    Compute(pi0, 0, param, ek0, Ix, F_TEST);
    time = GetTime() - time;
    cout << "Eval0 algo time: " << time * 1000 << " ms\n";

    time = GetTime();
    Compute(pi1, 1, param, ek1, Ix, F_TEST);
    time = GetTime() - time;
    cout << "Eval1 algo time: " << time * 1000 << " ms\n";

    bool res_acc;
    time = GetTime();
    res_acc = Verify(pi0, pi1, param);
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
    dig_t y_dig;
    Decode(y_dig, pi0, pi1, sk);
    
    
    // NativeEval(y_native, param.degree_f, msg_num, X, param.ck.g1_order_ZZ, F_TEST);
    y_native = P_d2(X,degree_f) % param.ck.g1_order_ZZ;
    cout << "True result: " << y_native << endl;
    cout << "Eval result: " << y_dig << endl;
    core_clean();
}