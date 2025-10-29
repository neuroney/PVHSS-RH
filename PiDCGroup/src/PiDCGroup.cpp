#include "PiDCGroup.h"

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

void PVHSSElg2_KeyGen(PVHSSElg2_Para &param, PVHSSElg2_SK &sk, bn_t ekp0[2], bn_t ekp1[2])
{
    bn_rand_mod(ekp0[0],param.ck.g1_order); //u_0
    bn_rand_mod(ekp0[1], param.ck.g1_order); //v_0
    bn_rand_mod(ekp1[0], param.ck.g1_order); //u_1
    bn_rand_mod(ekp1[1], param.ck.g1_order); //v_1

    bn_t u, v;
    bn_new(u);
    bn_new(v);
    bn_sub(u,ekp1[0],ekp0[0]);
    bn_sub(v,ekp0[1],ekp1[1]);
    
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
    fp12_mul(param.aux0, tmp, param.aux0);

    g1_new(hu);
    g1_mul(hu, param.ck.pub->gx, u);
    fp12_new(param.aux1);
    pp_map_oatep_k12(param.aux1, hu, param.vk);

    g1_new(hv);
    g1_mul(hv, param.ck.pub->gx, v);
    fp12_new(tmp);
    pp_map_oatep_k12(tmp, hv, g2);
    fp12_mul(param.aux1, tmp, param.aux1);
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

void PVHSSElg2_Compute(PROOF &proof, int b, const PVHSSElg2_Para &param, const PVHSSElg2_EK &ekb,  bn_t ekpb[2], vector<PVHSSElg2_CT> &Ix, vector<vector<int>> F_TEST)
{
    int prf_key = 0;
    VHSSElg_MV y_b_res;
    //VHSSElg_Evaluate(y_b_res, b, Ix, param.pk, ekb, prf_key, F_TEST);
    VHSSElg_Evaluate_P_d2(y_b_res, b, Ix, param.pk, ekb, prf_key, param.degree_f);
    PVHSSElg2_Prove(proof, b, y_b_res[0], y_b_res[2], param, ekpb);
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

void PVHSSElg2_Decode(dig_t &y, const PROOF &pi0, const PROOF &pi1, const PVHSSElg2_SK sk)
{
    g1_t in[2];
    g1_new(in[0]);
    g1_new(in[1]);
    g1_sub(in[0], pi1.C[0], pi0.C[0]);
    g1_sub(in[1], pi1.C[1], pi0.C[1]);

    cp_decped_dec1(y, in, sk);
}

void PVHSSElg2_Prove(PROOF &pi, int b, const ZZ &yb, const ZZ &Yb, const PVHSSElg2_Para &param, bn_t ekpb[2])
{
    DecPed_Com(pi.C, ekpb[0], param.ck, yb);
    DecPed_Com(pi.D, ekpb[1], param.ck, Yb);
}

void PVHSSElg2_ACC_TEST(int msg_num, int degree_f)
{
    PVHSSElg2_Para param;
    PVHSSElg2_EK ek0, ek1;
    PVHSSElg2_SK sk;
    bn_t ekp0[2], ekp1[2];
    param.skLen = 1024;
    param.vkLen = 256;
    param.msg_bits = 1;
    param.degree_f = degree_f;
    param.msg_num = msg_num;
    double time = GetTime();
    PVHSSElg2_Setup(param, ek0, ek1, sk);
    PVHSSElg2_KeyGen(param, sk, ekp0, ekp1);
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
    PVHSSElg2_Compute(pi0, 0, param, ek0, ekp0, Ix, F_TEST);
    time = GetTime() - time;
    cout << "Eval0 algo time: " << time * 1000 << " ms\n";

    time = GetTime();
    PVHSSElg2_Compute(pi1, 1, param, ek1, ekp1, Ix, F_TEST);
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