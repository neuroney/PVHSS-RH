#include "tester.h"

void PVHSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    std::cout << "*******************************************************" << std::endl;
    std::cout << "        Performance Test Results for PVHSS_time        " << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "degree_f: " << degree_f << "        msg_num: " << msg_num << "        cyctimes: " << cyctimes << std::endl;
    PVHSSPara param;
    PVHSS_EK ek0, ek1;
    PVHSS_SK sk;
    param.skLen = 256;
    param.vkLen = 256;
    param.msg_bits = 32;
    param.degree_f = degree_f;
    param.msg_num = msg_num;

    PROOF pi0, pi1;

    auto *Time = new double[cyctimes];
    double time, mean, stdev;

    // Setup Phase
    for (int i = 0; i < cyctimes; i++)
    {
        PVHSSPara param00;
        PVHSS_EK ek000, ek100;
        PVHSS_SK sk000;
        time = GetTime();
        param00.skLen = 256;
        param00.vkLen = 256;
        param00.msg_bits = 32;
        param00.degree_f = degree_f;
        param00.msg_num = msg_num;
        Setup(param00, ek000, ek100);
        KeyGen(param00, sk000);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);

    cout << "Setup algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    // Key Generation Phase
    Setup(param, ek0, ek1);
    KeyGen(param, sk);

    // Input Generation Phase
    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.msg_bits);
    }

    // Input Processing Phase
    vector<PVHSS_CT> Ix;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        ProbGen(Ix, param, X);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Input algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // Polynomial Generation Phase
    vector<vector<int>> F_TEST;
    Random_Func(F_TEST, msg_num, degree_f);

    // Evaluation Phase for Server 0
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        Compute(pi0, 0, param, ek0, Ix, F_TEST);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluation 0 algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // Evaluation Phase for Server 1
    for (int i = 0; i < 1; i++)
    {
        time = GetTime();
        Compute(pi1, 1, param, ek1, Ix, F_TEST);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, 1);
    cout << "Evaluation 1 algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // Verification Phase
    bool acc;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        acc = Verify(pi0, pi1, param.ck);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Verification algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // if (acc)
    // {
    //     cout << "Verification Passed\n";
    // }
    // else
    // {
    //     cout << "Verification Failed\n";
    // }

    // Decryption Phase
    bn_t y;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        Decode(y, pi0, pi1,sk);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Decryption algorithm time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
    std::cout << "-------------------------------------------------------" << std::endl;

}



void HSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    HSS_PK pk;
    HSS_EK ek0, ek1;
    int msg_bits = 256;
        int skLen = 256;
    int vkLen = 256;

    auto *Time = new double[cyctimes];
    double time, mean, stdev;
    for (int i = 0; i < cyctimes; i++)
    {
        time = GetTime();
        HSS_Gen(pk, ek0, ek1, skLen, vkLen);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "HSS_Gen algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    for (int i = 0; i < cyctimes; i++)
    {
        ZZ X_single;
        RandomBits(X_single, msg_bits);
        HSS_CT X_Input;
        time = GetTime();
        HSS_Input(X_Input, pk, X_single);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "HSS_Input algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    for (int i = 0; i < cyctimes; i++)
    {
        ZZ X_single;
        RandomBits(X_single, msg_bits);
        HSS_CT X_Input;
        HSS_MV X_Memory;
        HSS_Input(X_Input, pk, X_single);
        int prf_key = 0;
        time = GetTime();
        HSS_ConvertInput(X_Memory, 0, pk, ek0, X_Input, prf_key);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "HSS_ConvertInput algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    for (int i = 0; i < cyctimes; i++)
    {
        ZZ X1, X2;
        RandomBits(X1, msg_bits);
        RandomBits(X2, msg_bits);
        HSS_CT X_Input_1, X_Input_2;
        HSS_Input(X_Input_1, pk, X1);
        HSS_Input(X_Input_2, pk, X2);

        HSS_MV z_Memory, X_Memory_1, X_Memory_2;
        int prf_key = 0;
        HSS_ConvertInput(X_Memory_1, 0, pk, ek0, X_Input_1, prf_key);
        HSS_ConvertInput(X_Memory_2, 0, pk, ek0, X_Input_2, prf_key);

        time = GetTime();
        HSS_AddMemory(z_Memory, pk, X_Memory_1, X_Memory_2);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "HSS_AddMemory algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    for (int i = 0; i < cyctimes; i++)
    {
        ZZ X;
        RandomBits(X, msg_bits);
        HSS_CT X_Input;
        HSS_Input(X_Input, pk, X);

        HSS_MV z_Memory, X_Memory;
        int prf_key = 0;
        HSS_ConvertInput(X_Memory, 0, pk, ek0, X_Input, prf_key);

        time = GetTime();
        HSS_Mul(z_Memory, 0,pk, X_Input, X_Memory, prf_key);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "HSS_Mul algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    vector<vector<int>> F_TEST;
    Random_Func(F_TEST, msg_num, degree_f);

    Vec<ZZ> X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], msg_bits);
    }

    vector<HSS_CT> Ix;
    HSS_CT CTtmp;
    for (int i = 0; i < Ix.size(); ++i)
    {
        HSS_Input(CTtmp,pk,X[i]);
        Ix.push_back(CTtmp);
    }

    HSS_MV y0, y1;
    for (int i = 0; i < cyctimes; i++)
    {
        int prf_key = 0;
        time = GetTime();
        HSS_Evaluate(y0, 0, Ix, pk, ek0, prf_key, F_TEST);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluate0 algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

    for (int i = 0; i < cyctimes; i++)
    {
        int prf_key = 0;
        time = GetTime();
        HSS_Evaluate(y1, 1, Ix, pk, ek1, prf_key, F_TEST);
        Time[i] = GetTime() - time;
    }
    DataProcess(mean, stdev, Time, cyctimes);
    cout << "Evaluate1 algo time: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";
}


// void Depth2Tree(vec_ZZ &res, vec_ZZ Ip, vec_ZZ Ilc, vec_ZZ Irc, vec_ZZ M1_b, int B, Para param, EK ekb, int &prf_key)
// {
//     Vec<ZZ> Mp, temp1, temp2;
//     Mp.SetLength(param.pk.l + 1);
//     temp1.SetLength(param.pk.l + 1);
//     temp2.SetLength(param.pk.l + 1);

//     HSS_SubMemory(temp1, param.pk, M1_b, Mp);           //  1-p
//     HSS_Mul(temp1, param.pk, ekb, Ilc, temp1, prf_key); // (1-p) * lc
//     HSS_Mul(temp2, param.pk, ekb, Irc, Mp, prf_key);    // p * rc
//     HSS_AddMemory(res, param.pk, temp1, temp2);
// }

// // lc M, rc I
// void Depth2Tree_Type1(vec_ZZ &res, vec_ZZ Ip, vec_ZZ Mlc, vec_ZZ Irc, vec_ZZ M1_b, int B, Para param, EK ekb, int &prf_key)
// {
//     Vec<ZZ> temp1, Mrc;
//     Mrc.SetLength(param.pk.l + 1);
//     temp1.SetLength(param.pk.l + 1);

//     HSS_Mul(Mrc, param.pk, ekb, Irc, M1_b, prf_key);   // rc
//     HSS_SubMemory(temp1, param.pk, Mrc, Mlc);          // rc - lc
//     HSS_Mul(temp1, param.pk, ekb, Ip, temp1, prf_key); // p * (rc - lc)
//     HSS_AddMemory(temp1, param.pk, Mlc, temp1);        // lc + p * (rc - lc)
// }

// // lc I, rc M
// void Depth2Tree_Type2(vec_ZZ &res, vec_ZZ Ip, vec_ZZ Ilc, vec_ZZ Mrc, vec_ZZ M1_b, int B, Para param, EK ekb, int &prf_key)
// {
//     Vec<ZZ> temp1, Mlc;
//     Mlc.SetLength(param.pk.l + 1);
//     temp1.SetLength(param.pk.l + 1);

//     HSS_Mul(Mlc, param.pk, ekb, Ilc, M1_b, prf_key);   // lc
//     HSS_SubMemory(temp1, param.pk, Mrc, Mlc);          // rc - lc
//     HSS_Mul(temp1, param.pk, ekb, Ip, temp1, prf_key); // p * (rc - lc)
//     HSS_AddMemory(temp1, param.pk, Mlc, temp1);        // lc + p * (rc - lc)
// }

// // lc M, rc M
// void Depth2Tree_Type3(vec_ZZ &res, vec_ZZ Ip, vec_ZZ Mlc, vec_ZZ Mrc, vec_ZZ M1_b, int B, Para param, EK ekb, int &prf_key)
// {
//     Vec<ZZ> temp1;
//     temp1.SetLength(param.pk.l + 1);

//     HSS_SubMemory(temp1, param.pk, Mrc, Mlc);          // rc - lc
//     HSS_Mul(temp1, param.pk, ekb, Ip, temp1, prf_key); // p * (rc - lc)
//     HSS_AddMemory(temp1, param.pk, Mlc, temp1);        // lc + p * (rc - lc)
// }

// void DT_TIME_TEST(int model, int depth, int cyctimes)
// {
//     Para param;
//     EK ek0, ek1;
//     PROOF pi0, pi1;
//     param.msg_bits = 32;

//     KeyGen(param, ek0, ek1, depth);

//     int num_node = 20;
//     int num_class = 20;
//     vec_ZZ b, c;
//     b.SetLength(num_node);
//     c.SetLength(num_class);
//     for (int i = 0; i < num_node; i++)
//     {
//         b[i] = random() % 2;
//         c[i] = 1;
//     }
//     Vec<vec_ZZ> C_b, C_c;
//     C_b.SetLength(num_node);
//     C_c.SetLength(num_class);
//     for (int i = 0; i < num_node; i++)
//     {
//         C_b[i].SetLength(param.pk.l + 1);
//         HSS_Input(C_b[i], param.pk, b[i]);
//     }
//     for (int i = 0; i < num_class; i++)
//     {
//         C_c[i].SetLength(param.pk.l + 1);
//         HSS_Input(C_c[i], param.pk, c[i]);
//     }

//     auto Time = new double[cyctimes];
//     double time, mean, stdev;
//     cout << "******************** Server 1 Evaluating ********************"
//          << "\n";

//     bn_t y0;
//     for (int i = 0; i < cyctimes; i++)
//     {
//         time = GetTime();
//         Eval_DT(y0, pi0, 0, param, ek0, model, depth, C_b, C_c);
//         Time[i] = GetTime() - time;
//     }
//     DataProcess(mean, stdev, Time, cyctimes);

//     cout << "running time Server 1: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

//     cout << "******************** Server 2 Evaluating ********************"
//          << "\n";

//     bn_t y1;
//     for (int i = 0; i < cyctimes; i++)
//     {
//         time = GetTime();
//         Eval_DT(y1, pi1, 1, param, ek1, model, depth, C_b, C_c);
//         Time[i] = GetTime() - time;
//     }
//     DataProcess(mean, stdev, Time, cyctimes);
//     cout << "running time Server 2: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

//     cout << "******************** Client Decrypting ********************"
//          << "\n";
//     for (int i = 0; i < cyctimes; i++)
//     {
//         time = GetTime();
//         verify(y0, y1, pi0, pi1, param.ck);
//         Time[i] = GetTime() - time;
//     }
//     DataProcess(mean, stdev, Time, cyctimes);
//     cout << "running time Client: " << mean * 1000 << " ms  RSD: " << stdev * 100 << "%\n";

//     bn_t y;
//     bn_new(y);
//     bn_sub(y, y1, y0);
//     ZZ y_ZZ;
//     bn2ZZ(y_ZZ, y);
//     cout << y_ZZ << endl;
// }

// void Eval_DT(bn_t y, PROOF &pi, int B, Para param, EK ekb, int model, int depth,
//              Vec<vec_ZZ> Ibs, Vec<vec_ZZ> Ics)
// {
//     Vec<ZZ> temp1, temp2, temp3, M1_b;
//     temp1.SetLength(param.pk.l + 1);
//     temp2.SetLength(param.pk.l + 1);
//     temp3.SetLength(param.pk.l + 1);
//     M1_b.SetLength(param.pk.l + 1);
//     int prf_key = 0;

//     HSS_GenM1(M1_b, B, param.pk, ekb, prf_key);
//     if (model == 2)
//     {
//         switch (depth)
//         {
//         case 4:
//             Depth2Tree(temp1, Ibs[3], Ics[2], Ics[3], M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[4], Ics[4], Ics[5], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp1, Ibs[1], temp1, temp2, M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[2], Ics[0], Ics[1], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp1, Ibs[0], temp1, temp2, M1_b, B, param, ekb, prf_key);
//             break;
//         case 6:
//             Depth2Tree(temp1, Ibs[5], Ics[5], Ics[6], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type2(temp1, Ibs[4], Ics[4], temp1, M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type2(temp1, Ibs[3], Ics[3], temp1, M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[2], Ics[1], Ics[2], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp3, Ibs[1], temp2, temp1, M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type1(temp1, Ibs[0], temp3, Ics[0], M1_b, B, param, ekb, prf_key);
//             break;
//         case 8:
//             Depth2Tree(temp1, Ibs[8], Ics[8], Ics[9], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type2(temp1, Ibs[7], Ics[7], temp1, M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type2(temp1, Ibs[6], Ics[6], temp1, M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type2(temp1, Ibs[5], Ics[5], temp1, M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type1(temp1, Ibs[3], temp1, Ics[2], M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[4], Ics[3], Ics[4], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type1(temp2, Ibs[2], temp2, Ics[1], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp1, Ibs[1], temp2, temp1, M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type1(temp1, Ibs[0], temp1, Ics[0], M1_b, B, param, ekb, prf_key);
//             break;
//         default:
//             printf("error\n");
//             break;
//         }
//     }
//     else
//     {
//         switch (depth)
//         {
//         case 4:
//             Depth2Tree(temp1, Ibs[3], Ics[3], Ics[4], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type1(temp1, Ibs[2], temp1, Ics[2], M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[1], Ics[0], Ics[1], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp1, Ibs[0], temp2, temp1, M1_b, B, param, ekb, prf_key);
//             break;
//         case 6:
//             Depth2Tree(temp1, Ibs[7], Ics[6], Ics[7], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type2(temp1, Ibs[5], Ics[4], temp1, M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[8], Ics[8], Ics[9], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type1(temp2, Ibs[6], temp2, Ics[5], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp1, Ibs[3], temp1, temp2, M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[4], Ics[2], Ics[3], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp1, Ibs[2], temp1, temp2, M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[1], Ics[0], Ics[1], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp1, Ibs[0], temp2, temp1, M1_b, B, param, ekb, prf_key);
//             break;
//         case 8:
//             Depth2Tree(temp1, Ibs[12], Ics[11], Ics[12], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type1(temp1, Ibs[10], temp1, Ics[9], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type1(temp1, Ibs[8], temp1, Ics[7], M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[13], Ics[13], Ics[14], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type1(temp2, Ibs[11], temp2, Ics[10], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type1(temp2, Ibs[9], temp2, Ics[8], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp1, Ibs[6], temp1, temp2, M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[7], Ics[5], Ics[6], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp1, Ibs[5], temp1, temp2, M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[4], Ics[3], Ics[4], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp1, Ibs[2], temp2, temp1, M1_b, B, param, ekb, prf_key);
//             Depth2Tree(temp2, Ibs[3], Ics[1], Ics[2], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type1(temp2, Ibs[1], temp1, Ics[0], M1_b, B, param, ekb, prf_key);
//             Depth2Tree_Type3(temp1, Ibs[0], temp2, temp1, M1_b, B, param, ekb, prf_key);
//             break;
//         default:
//             printf("error\n");
//             break;
//         }
//     }

//     bn_null(y);
//     bn_new(y);
//     ZZ2bn(y, temp1[0] % param.ck.g1_order_ZZ);

//     // Y_b = HSS.HSS_Mul(y_b, I_A)
//     Vec<ZZ> Y_ZZ;
//     HSS_Mul(Y_ZZ, param.pk, ekb, param.I_A, temp1, prf_key);

//     BivPE_Prove(pi, B, Y_ZZ[0], param.ck, prf_key);
// }

// void select(ZZ &res, int b, PK pk, EK ekb, Vec<ZZ> ct_idx, vector<Vec<ZZ>> y, int &prf_key)
// {
//     double time = GetTime();
//     int len = y.size(); // m

//     int j, k;
//     Vec<ZZ> Dj, Z;
//     Dj.SetLength(pk.l + 1);
//     Z.SetLength(pk.l + 1);
//     for (int i = 0; i < pk.l + 1; ++i)
//     {
//         Z[i] = ZZ(0);
//     }

//     ZZ tmp0(1);
//     Vec<ZZ> tmp1, tmp2;
//     tmp1.SetLength(pk.l + 1);
//     tmp2.SetLength(pk.l + 1);

//     // for (j = len - 1; j > -1; --j)
//     // {
//     //     // D[j]
//     //     if (j != len - 1)
//     //     {
//     //         tmp0 *= (j + 1);
//     //     }

//     //     for (int i = 0; i < pk.l + 1; ++i)
//     //     {
//     //         tmp1[i] = ZZ(0);
//     //     }
//     //     for (k = 0; k <= j; ++k)
//     //     {

//     //         // HSS_cMul(tmp2, pk, ekb, Combine(j, k) * power_ZZ(-1, j - k), y[k], prf_key);
//     //         HSS_cMul(tmp2, pk, ekb, ZZ(9), y[k], prf_key);
//     //         HSS_AddMemory(tmp1, pk, tmp1, tmp2);
//     //     }

//     //     HSS_cMul(Dj, pk, ekb, tmp0, tmp1, prf_key);

//     //     // Z + D[j]
//     //     HSS_AddMemory(Z, pk, Z, Dj);

//     //     HSS_Input(tmp2, pk, ZZ(1 - j));
//     //     HSS_AddInput(tmp2, pk, ct_idx, tmp2);

//     //     HSS_Mul(Z, pk, ekb, tmp2, Z, prf_key);
//     // }
//     for (int i = 0; i < pk.l + 1; ++i)
//     {
//         RandomBnd(Z[i], pk.N);
//     }

//     ZZ len_factorial(1), inv_len_factorial; // (m-1)!
//     for (j = 2; j < len; ++j)
//     {
//         MulMod(len_factorial, len_factorial, ZZ(j), pk.N);
//     }
//     InvMod(inv_len_factorial, len_factorial, pk.N);
//     HSS_cMul(Z, pk, ekb, inv_len_factorial, Z, prf_key);
//     res = Z[0];
//     time = GetTime() - time;
//     cout << "Select algo time: " << time * 1000 << " ms\n";
// }

// void selectTest()
// {
//     Para param;
//     EK ek0, ek1;
//     PROOF pi0, pi1;
//     KeyGen(param, ek0, ek1, 1);
//     ZZ idx(1), res0;
//     Vec<ZZ> ct_idx;
//     ct_idx.SetLength(param.pk.l + 1);

//     HSS_Input(ct_idx, param.pk, idx);

//     int prf_key;

//     vector<Vec<ZZ>> y0;
//     Vec<ZZ> tmp;
//     tmp.SetLength(param.pk.l + 1);
//     int num;
//     for (int depth = 14; depth < 21; ++depth)
//     {
//         num = depth * 2048;
//         for (int i = 0; i < num; ++i)
//         {
//             // HSS_Input(tmp, param.pk, ZZ(i));

//             prf_key = 1;
//             // HSS_ConvertInput(tmp, 0, param.pk, ek0, tmp, prf_key);
//             for (int kk = 0; kk < param.pk.l + 1; ++kk)
//             {
//                 RandomBnd(tmp[kk], param.pk.N);
//             }
//             y0.push_back(tmp);

//             // prf_key = 1;
//             // HSS_Load(tmp, 1, pk, ek1, ct_y[i], prf_key);
//             // y1.push_back(tmp);
//         }
//         prf_key = 1;

//         cout << "Block number: " << num << endl;
//         select(res0, 0, param.pk, ek0, ct_idx, y0, prf_key);
//     }
// }