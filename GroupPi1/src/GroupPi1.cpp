#include "GroupPi1.h"

void Setup(PVHSSPara &param, PVHSS_EK &ek0, PVHSS_EK &ek1)
{
    HSS_Gen(param.pk, ek0, ek1, param.skLen, param.vkLen);
    Ped_ComGen(param.ck);

    bn_t A;
    bn_new(A);
    ep2_new(param.ck.g2_A);
    ZZ2bn(A, (ek1[1]-ek0[1]) % param.pk.N);
    ep2_mul_gen(param.ck.g2_A, A); // g_2^A
}

void KeyGen(PVHSSPara &param, PVHSS_SK &sk)
{
    RandomBnd(sk, param.ck.g1_order_ZZ);
    HSS_Input(param.pk_f, param.pk, sk);
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
    HSS_Evaluate(y_b_res, b, Ix, param.pk, ekb, prf_key, F_TEST);
   
    HSS_MV sk_b;
    HSS_ConvertInput(sk_b, b, param.pk, ekb, param.pk_f, prf_key);
    y_b_res[0] = y_b_res[0] + sk_b[0];
    y_b_res[2] = y_b_res[2] + sk_b[2];
    
    Ped_Prove(proof, b, y_b_res[0], y_b_res[2], param.ck, prf_key);
}

bool Verify(const PROOF &pi0, const PROOF &pi1, const CK &ck)
{
    fp12_t fp12tmp, lefthand;
    fp12_new(fp12tmp);
    fp12_new(lefthand);

    // Compute e(d_1/d_0, g)
    ep_t eptmp;
    ep_new(eptmp);
    ep_sub(eptmp, pi1.D, pi0.D);
    pp_map_oatep_k12(lefthand, eptmp, ck.g2);
    
    // Compute righthand e_1 /e_0
    fp12_t righthand;
    fp12_new(righthand);
    fp12_inv(righthand, pi0.e);
    fp12_mul(righthand, righthand, pi1.e);

    // Compute e(g, g)^{Ay}
    bn_t y_bn;
    bn_new(y_bn);
    bn_sub(y_bn,pi1.y,pi0.y);
    ep_mul(eptmp, ck.g1, y_bn);
    pp_map_oatep_k12(fp12tmp, eptmp, ck.g2_A);
    
    fp12_mul(righthand, righthand, fp12tmp);
    // Compare lefthand and righthand
    return (fp12_cmp(lefthand, righthand) == RLC_EQ);
}

void Decode(bn_t y, const PROOF &pi0, const PROOF &pi1, const PVHSS_SK sk)
{
    bn_new(y);
    bn_sub(y, pi1.y, pi0.y);
    bn_t sk_bn;
    ZZ2bn(sk_bn, sk);
    bn_sub(y, y, sk_bn);
}

void PVHSS_ACC_TEST(int msg_num, int degree_f)
{
    PVHSSPara param;
    PVHSS_EK ek0, ek1;
    PVHSS_SK sk;
    param.skLen = 1024;
    param.vkLen = 256;
    param.msg_bits = 32;
    param.degree_f = degree_f;
    param.msg_num = msg_num;

    double time = GetTime();
    Setup(param, ek0, ek1);
    KeyGen(param, sk);
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
    res_acc = Verify(pi0, pi1, param.ck);
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
    Decode(y_eval_bn, pi0, pi1, sk);
    bn2ZZ(y_eval, y_eval_bn);
    y_eval = y_eval % param.ck.g1_order_ZZ;
    NativeEval(y_native, param.degree_f, msg_num, X, param.ck.g1_order_ZZ, F_TEST);
    cout << "True result: " << y_native << endl;
    cout << "Eval result: " << y_eval << endl;
    core_clean();
}