#include "PiRERHGroup.h"

void PVHSSElg2_Setup(PVHSSElg2_Para &param, PVHSSElg2_EK &ek0, PVHSSElg2_EK &ek1, PVHSSElg2_SK &sk)
{
    VHSSElg_Gen(param.pk, ek0, ek1, param.skLen, param.vkLen);
    DecPed_ComGen(param.ck, sk);

    bn_t A;
    bn_new(A);
    ep2_new(param.vk);
    ZZ2bn(A, (ek1[1] - ek0[1]) % param.pk.N);
    ep2_mul_gen(param.vk, A); // g_2^A
    
}

void PVHSSElg2_KeyGen(PVHSSElg2_Para &param, PVHSSElg2_SK &sk)
{
    return;
}

void PVHSSElg2_ProbGen(vector<PVHSSElg2_CT> &Ix, const PVHSSElg2_Para &param, const vec_ZZ &x)
{
    Ix.clear();
    int i;
    VHSSElg_CT CTtmp;
    for (i = 0; i < x.length(); ++i)
    {
        VHSSElg_Input(CTtmp, param.pk, x[i]);
        Ix.push_back(CTtmp);
    }
}

void PVHSSElg2_Compute(PROOF &proof, int b, const PVHSSElg2_Para &param, const PVHSSElg2_EK &ekb, vector<PVHSSElg2_CT> &Ix, vector<vector<int>> F_TEST)
{
    int prf_key = 0;
    VHSSElg_MV y_b_res;
    //VHSSElg_Evaluate(y_b_res, b, Ix, param.pk, ekb, prf_key, F_TEST);
    VHSSElg_Evaluate_P_d2(y_b_res, b, Ix, param.pk, ekb, prf_key, param.degree_f);
    double time =GetTime();
    PVHSSElg2_Prove(proof, b, y_b_res[0], y_b_res[2], param);
    cout << "Prove algorithm time: " << (GetTime() - time) * 1000 << " ms\n";
}

bool PVHSSElg2_Verify(const PROOF &pi0, const PROOF &pi1, const PVHSSElg2_Para &param)
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

void PVHSSElg2_Decode(dig_t &y, const PROOF &pi0, const PROOF &pi1, const PVHSSElg2_SK sk)
{
    g1_t in[2];
    g1_new(in[0]);
    g1_new(in[1]);
    g1_sub(in[0], pi1.C[0], pi0.C[0]);
    g1_sub(in[1], pi1.C[1], pi0.C[1]);

    cp_decped_dec1(y, in, sk);
}

void PVHSSElg2_Prove(PROOF &pi, int b, const ZZ &yb, const ZZ &Yb, const PVHSSElg2_Para &param)
{
    bn_t u, v;
    fp12_t mu, delta;

    bn_new(u);
    bn_new(v);
    fp12_new(mu);
    fp12_new(delta);

    DecPed_Com(pi.C, u, param.ck, yb);
    DecPed_Com(pi.D, v, param.ck, Yb);

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

void PVHSSElg2_ACC_TEST(int msg_num, int degree_f)
{
    PVHSSElg2_Para param;
    PVHSSElg2_EK ek0, ek1;
    PVHSSElg2_SK sk;
    param.skLen = 1024;
    param.vkLen = 256;
    param.msg_bits = 1;
    param.degree_f = degree_f;
    param.msg_num = msg_num;
    double time = GetTime();
    PVHSSElg2_Setup(param, ek0, ek1, sk);
    PVHSSElg2_KeyGen(param, sk);
    time = GetTime() - time;
    cout << "KeyGen algo time: " << time * 1000 << " ms\n";

    vec_ZZ X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.msg_bits);
    }

    vector<PVHSSElg2_CT> Ix;
    time = GetTime();
    PVHSSElg2_ProbGen(Ix, param, X);
    time = GetTime() - time;
    cout << "ProbGen algo time: " << time * 1000 << " ms\n";

    vector<vector<int>> F_TEST;
    Random_Func(F_TEST, msg_num, degree_f);

    PROOF pi0, pi1;
    time = GetTime();
    PVHSSElg2_Compute(pi0, 0, param, ek0, Ix, F_TEST);
    time = GetTime() - time;
    cout << "Eval0 algo time: " << time * 1000 << " ms\n";

    time = GetTime();
    PVHSSElg2_Compute(pi1, 1, param, ek1, Ix, F_TEST);
    time = GetTime() - time;
    cout << "Eval1 algo time: " << time * 1000 << " ms\n";

    bool res_acc;
    time = GetTime();
    res_acc = PVHSSElg2_Verify(pi0, pi1, param);
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
    PVHSSElg2_Decode(y_dig, pi0, pi1, sk);
    
    
    // NativeEval(y_native, param.degree_f, msg_num, X, param.ck.g1_order_ZZ, F_TEST);
    y_native = P_d2(X,degree_f) % param.ck.g1_order_ZZ;
    cout << "True result: " << y_native << endl;
    cout << "Eval result: " << y_dig << endl;
    core_clean();
}