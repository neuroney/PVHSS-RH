#include "PVHSS.h"
#include "GenData.h"
#include <algorithm>
#include <stdlib.h>
using namespace std;

void Eval_Poly(int deg, int num_data)
{
    // para
    cout << "******************** Setup ********************"
         << "\n";
    PKE_Para pkePara;
    PVHSS_Para pvhssPara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2, C_alpha;
    ZZ_pXModulus modulus;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    C_alpha.SetLength(4);
    pkePara.msg_bit = 32;
    pkePara.d = deg;
    pkePara.num_data = num_data;
    CZTimingResult timing = CZMeasureTimeMs([&]() {
        PKE_Gen(pkePara, pkePk, pkeSk, 1);
        build(modulus, pkePara.xN);
        PVHSS_Gen(hssEk_1, hssEk_2, C_alpha, pvhssPara, pkePara, modulus, pkePk, pkeSk);
    }, 1);
    CZPrintTimeMs("running time Setup", timing);

    // data
    Data data;
    ZZ T1, T2;
    ep_t g1T1;
    ep2_t g2T2;
    ZZ_pX t1y, t2y;
    timing = CZMeasureTimeMs([&]() {
        GenData(data, pkePara, modulus, pkePk);
        ep_new(g1T1);
        ep2_new(g2T2);
    }, 1);
    CZPrintTimeMs("Enc time Setup", timing);

    // eval
    cout << "******************** Server 1 Evaluating ********************" << "\n";
    timing = CZMeasureTimeMs([&]() {
        PVHSS_Eval(t1y, g1T1, g2T2, 1, pvhssPara, pkePara,
                   modulus, hssEk_1, C_alpha, data.C_X, data.PRF, data.F_TEST, pkePara.d, data.C_1);
    }, 1);
    CZPrintTimeMs("running time Server 1", timing);

    cout << "******************** Server 2 Evaluating ********************" << "\n";
    timing = CZMeasureTimeMs([&]() {
        PVHSS_Eval(t2y, g1T1, g2T2, 2, pvhssPara, pkePara,
                   modulus, hssEk_2, C_alpha, data.C_X, data.PRF, data.F_TEST, pkePara.d, data.C_1);
    }, 1);
    CZPrintTimeMs("running time Server 2", timing);

    ZZ_pX y_ZZ_pX;
    y_ZZ_pX = t1y + t2y;
        cout << 1 << endl;
    y_ZZ_pX = t1y + t2y;
    cout << 2 << endl;


    cout << "******************** Client Verifying ********************" << "\n";
    timing = CZMeasureTimeMsAdaptive([&]() {
        PVHSS_ver(t1y, t2y, g1T1, g2T2, pvhssPara);
    }, 1);
    CZPrintTimeMs("running time Verification", timing);

    cout << "******************** Client Decoding ********************" << "\n";

    ZZ res;
    timing = CZMeasureTimeMsAdaptive([&]() {
        PVHSS_Dec(res, pvhssPara, pkePara, t1y, t2y);
    }, 1);
    CZPrintTimeMs("running time Decoding", timing);




    ZZ y_native;
    NativeEval(y_native, pkePara.d, pkePara.num_data, data.X_decimal, pvhssPara.g1_order_ZZ, data.F_TEST);
    cout << "native computing y =" << y_native << "\n";
}

void Run_Gen(int msg_bit)
{
    // para
    PKE_Para pkePara;
    PVHSS_Para pvhssPara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2, C_alpha;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    C_alpha.SetLength(4);
    pkePara.msg_bit = msg_bit;
    pkePara.num_data = 2;
    PKE_Gen(pkePara, pkePk, pkeSk, 0);
    ZZ_pXModulus modulus(pkePara.xN);
    PVHSS_Gen(hssEk_1, hssEk_2, C_alpha, pvhssPara, pkePara, modulus, pkePk, pkeSk);
}

void Time_Gen(int msg_bit, int cyctimes)
{
    CZTimingResult timing = CZMeasureTimeMs([&]() {
        Run_Gen(msg_bit);
    }, cyctimes);
    CZPrintTimeMs("Gen algo time", timing);
}

void Time_Enc(int msg_bit, int cyctimes)
{
    // para
    PKE_Para pkePara;
    PVHSS_Para pvhssPara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2, C_alpha;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    C_alpha.SetLength(4);
    pkePara.msg_bit = msg_bit;
    pkePara.num_data = 2;
    PKE_Gen(pkePara, pkePk, pkeSk, 0);
    ZZ_pXModulus modulus(pkePara.xN);
    PVHSS_Gen(hssEk_1, hssEk_2, C_alpha, pvhssPara, pkePara, modulus, pkePk, pkeSk);

    vec_ZZ_pX C;
    ZZ_pX x;
    C.SetLength(4);
    CZTimingResult timing = CZMeasureTimeMs([&]() {
        Random_ZZ_pX(x, pkePara.msg_bit, 1);
        PVHSS_Enc(C, pkePara, modulus, pkePk, x);
    }, cyctimes);
    CZPrintTimeMs("Enc algo time", timing);
}

void Time_Ver(int msg_bit, int cyctimes)
{
    // para
    PKE_Para pkePara;
    PVHSS_Para pvhssPara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2, C_alpha;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    C_alpha.SetLength(4);
    pkePara.msg_bit = msg_bit;
    pkePara.num_data = 2;
    PKE_Gen(pkePara, pkePk, pkeSk, 0);
    ZZ_pXModulus modulus(pkePara.xN);
    PVHSS_Gen(hssEk_1, hssEk_2, C_alpha, pvhssPara, pkePara, modulus, pkePk, pkeSk);
    ZZ_pX y_ZZ_pZ;
    ZZ_p y_ZZ_p, y1, y2, y;
    ZZ y_ZZ;
    ep_t g1T1;
    ep2_t g2T2;
    ep_new(g1T1);
    ep2_new(g2T2);
    ZZ_pX y_ZZ_pX, y1_ZZ_pX, y2_ZZ_pX;
    Random_ZZ_pX(y_ZZ_pX, pkePara.N, pkePara.msg_bit);
    Random_ZZ_pX(y1_ZZ_pX, pkePara.N, pkePara.q_bit);
    y2_ZZ_pX = y_ZZ_pX - y1_ZZ_pX;
    CZTimingResult timing = CZMeasureTimeMsAdaptive([&]() {
        fp12_t gtT1;
        fp12_t gtT2;
        fp12_t right_side;
        fp12_t left_side;
        fp12_new(gtT1);
        fp12_new(gtT2);
        fp12_new(right_side);
        fp12_new(left_side);
        bn_t y_bn;
        bn_new(y_bn);
        ZZX y_ZZX;
        y_ZZ_pX = y1_ZZ_pX + y2_ZZ_pX;
        conv(y_ZZX, y_ZZ_pX);
        ZZ_p::init(pvhssPara.g1_order_ZZ);

        ZZ two_ZZ;
        ZZ_p two_ZZ_p;
        bn_t Tb_bn;
        two_ZZ_p = 2;
        bn_new(Tb_bn);
        conv(y_ZZ_pX, y_ZZX);
        eval(y_ZZ_p, y_ZZ_pX, two_ZZ_p);

        conv(y_ZZ, y_ZZ_p);
        ZZ2bn(y_bn, y_ZZ);

        fp12_exp(right_side, pvhssPara.A_gT, y_bn);
        fp12_mul(right_side, right_side, pvhssPara.const_gT);

        pp_map_oatep_k12(gtT1, g1T1, pvhssPara.g2_gen);
        pp_map_oatep_k12(gtT2, pvhssPara.g1_gen, g2T2);
        fp12_mul(left_side, gtT1, gtT2);

        if (fp12_cmp(left_side, right_side) == RLC_EQ)
        {
        }
        else
        {
        }
    }, cyctimes);
    CZPrintTimeMs("Ver algo time", timing);
}

void Time_Eval_Subalgo(int msg_bit, int cyctimes)
{
    // para
    PKE_Para pkePara;
    PVHSS_Para pvhssPara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2, C_alpha;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    C_alpha.SetLength(4);
    pkePara.msg_bit = msg_bit;
    pkePara.num_data = 2;
    PKE_Gen(pkePara, pkePk, pkeSk, 0);

    ZZ_pXModulus modulus(pkePara.xN);
    PVHSS_Gen(hssEk_1, hssEk_2, C_alpha, pvhssPara, pkePara, modulus, pkePk, pkeSk);
    // data
    Data data;
    GenData(data, pkePara, modulus, pkePk);

    // load
    vec_ZZ_pX db1, db2;
    db1.SetLength(2);
    db2.SetLength(2);
    CZTimingResult timing = CZMeasureTimeMs([&]() {
        int index = rand() % 10;
        PVHSS_Mult(db1, pkePara, modulus, hssEk_1, data.C_X[rand() % pkePara.num_data]);
        db1[0] = db1[0] + data.PRF[index][0];
        db1[1] = db1[1] + data.PRF[index][1];
    }, cyctimes);
    CZPrintTimeMs("Load algo time", timing);

    // add1
    Random_ZZ_pX(db1[0], pkePara.N, pkePara.q_bit);
    Random_ZZ_pX(db1[1], pkePara.N, pkePara.q_bit);
    Random_ZZ_pX(db2[0], pkePara.N, pkePara.q_bit);
    Random_ZZ_pX(db2[1], pkePara.N, pkePara.q_bit);
    int index = rand() % 10;
    vec_ZZ_pX add_tmp;
    add_tmp.SetLength(2);
    timing = CZMeasureTimeMsAdaptive([&]() {
        add_tmp[0] = db1[0] + db2[0] + data.PRF[index][0];
        add_tmp[1] = db1[1] + db2[1] + data.PRF[index][1];
    }, cyctimes);
    CZPrintTimeMs("Add1 algo time", timing);

    // add2
    vec_ZZ_pX C;
    C.SetLength(4);
    timing = CZMeasureTimeMsAdaptive([&]() {
        C[0] = data.C_X[0][0] + data.C_X[1][0];
        C[1] = data.C_X[0][1] + data.C_X[1][1];
        C[2] = data.C_X[0][2] + data.C_X[1][2];
        C[3] = data.C_X[0][3] + data.C_X[1][3];
    }, cyctimes);
    CZPrintTimeMs("Add2 algo time", timing);

    // cmult
    ZZ constant = RandomBits_ZZ(msg_bit);
    PVHSS_Mult(db1, pkePara, modulus, hssEk_1, data.C_X[rand() % pkePara.num_data]);
    index = rand() % 10;
    vec_ZZ_pX cmult_tmp;
    cmult_tmp.SetLength(2);
    timing = CZMeasureTimeMsAdaptive([&]() {
        ZZ_pX_ScaleMul_ZZ(cmult_tmp[0], db1[0], constant);
        ZZ_pX_ScaleMul_ZZ(cmult_tmp[1], db1[1], constant);
        cmult_tmp[0] = cmult_tmp[0] + data.PRF[index][0];
        cmult_tmp[1] = cmult_tmp[1] + data.PRF[index][1];
    }, cyctimes);
    CZPrintTimeMs("cMult algo time", timing);

    // mult
    timing = CZMeasureTimeMs([&]() {
        PVHSS_Mult(db1, pkePara, modulus, hssEk_1, data.C_X[rand() % pkePara.num_data]);
        int index = rand() % 10;
        PVHSS_Mult(db1, pkePara, modulus, db1, data.C_X[rand() % pkePara.num_data]);
        db1[0] = db1[0] + data.PRF[index][0];
        db1[1] = db1[1] + data.PRF[index][1];
    }, cyctimes);
    CZPrintTimeMs("Mult algo time", timing);

    // output
    int b = 1;
    ep_t g1T1;
    ep2_t g2T2;
    ep_new(g1T1);
    ep2_new(g2T2);
    ZZ_p yb;
    vec_ZZ_pX tb;
    tb.SetLength(2);
    PVHSS_Mult(tb, pkePara, modulus, hssEk_1, data.C_X[rand() % pkePara.num_data]);

    timing = CZMeasureTimeMs([&]() {
        ZZX tb0;
        ZZ two_ZZ, Tb;
        ZZ_p two_ZZ_p;
        bn_t Tb_bn;
        two_ZZ_p = 2;
        bn_new(Tb_bn);
        eval(yb, tb[0], two_ZZ_p);
        PVHSS_Mult(tb, pkePara, modulus, tb, C_alpha);
        conv(tb0, tb[0]);
        eval_ZZX(Tb, tb0);
        if (b == 1)
        {
            Tb = Tb % pvhssPara.g1_order_ZZ;
            ZZ2bn(Tb_bn, Tb);
            ep_mul_gen(g1T1, Tb_bn);
        }
        else
        {
            Tb = Tb % pvhssPara.g2_order_ZZ;
            ZZ2bn(Tb_bn, Tb);
            ep2_mul_gen(g2T2, Tb_bn);
        }
    }, cyctimes);
    CZPrintTimeMs("Output algo time", timing);
}
