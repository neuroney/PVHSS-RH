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
    //HSS_Evaluate(y_b_res, b, Ix, param.pk, ekb, prf_key, F_TEST);
    HSS_Evaluate_P_d2(y_b_res, b, Ix, param.pk, ekb, prf_key, param.degree_f);
    
    HSS_MV sk_b;
    HSS_ConvertInput(sk_b, b, param.pk, ekb, param.pk_f, prf_key);
    y_b_res[0] = y_b_res[0] + sk_b[0];
    y_b_res[2] = y_b_res[2] + sk_b[2];
    
    Ped_Prove(proof, b, y_b_res[0], y_b_res[2], param.ck, prf_key);
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
    ZZ2bn(y_bn, (pi1.y - pi0.y) % ck.g1_order_ZZ);
    ep_mul(eptmp, ck.g1, y_bn);
    pp_map_oatep_k12(righthand, eptmp, ck.g2_A);
    fp12_mul(righthand, righthand, pi1.e);
    
    // Compare lefthand and righthand
    return (fp12_cmp(lefthand, righthand) == RLC_EQ);
}

void Decode(ZZ& y, const PROOF &pi0, const PROOF &pi1, const PVHSS_SK sk)
{
    sub(y, pi1.y, pi0.y);
    sub(y, y, sk);
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
        //X[i] = i+1;
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
    Decode(y_eval, pi0, pi1, sk);
    y_eval = y_eval % param.ck.g1_order_ZZ;
    y_native = P_d(X, degree_f) % param.ck.g1_order_ZZ;
    // NativeEval(y_native, param.degree_f, msg_num, X, param.ck.g1_order_ZZ, F_TEST);
    cout << "True result: " << y_native << endl;
    cout << "Eval result: " << y_eval << endl;
    core_clean();
}