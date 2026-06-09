#include "tester.h"

using namespace NTL;
using namespace std;

void RunGen(int msg_num, int degree_f) {
    PKE_Para pkePara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    pkePara.msg_bit = 32;
    pkePara.d = degree_f;
    pkePara.num_data = msg_num;
    PKE_Gen(pkePara, pkePk, pkeSk);
    ZZ_pXModulus modulus(pkePara.xN);
    HssGen(hssEk_1, hssEk_2, pkePara, pkeSk);
}

void TimeGen(int msg_num, int degree_f, int cyctimes) {
    TimingResult timing = MeasureTimeMs([&]() {
        RunGen(msg_num, degree_f);
    }, cyctimes);
    PrintTimeMs("gen algo time", timing);
}


void TimeEnc(int msg_num, int degree_f,  int cyctimes) {
    //para
    PKE_Para pkePara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    pkePara.msg_bit = 32;
    pkePara.num_data = msg_num;
    pkePara.d = degree_f;
    PKE_Gen(pkePara, pkePk, pkeSk);
    ZZ_pXModulus modulus(pkePara.xN);
    HssGen(hssEk_1, hssEk_2, pkePara, pkeSk);

    vec_ZZ_pX C;
    ZZ x;
    C.SetLength(4);
    TimingResult timing = MeasureTimeMs([&]() {
        x = NTL::RandomBits_ZZ(pkePara.msg_bit);
        HSS_Enc(C, pkePara, modulus, pkePk, x);
    }, cyctimes);
    PrintTimeMs("Enc algo time", timing);
}

void Time_Dec(int msg_num, int degree_f, int cyctimes)
{
    //para
    PKE_Para pkePara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    pkePara.msg_bit = 32;
    pkePara.num_data = msg_num;
    pkePara.d = degree_f;
    PKE_Gen(pkePara, pkePk, pkeSk);
    ZZ_pXModulus modulus(pkePara.xN);
    HssGen(hssEk_1, hssEk_2, pkePara, pkeSk);

    ZZ_pX y_ZZ_pZ,y1,y2;
    ZZ_p y_ZZ_p;
    ZZ y_ZZ;
    NTL::RandomBits(y_ZZ,pkePara.msg_bit);
    conv(y_ZZ_p,y_ZZ);
    SetCoeff(y_ZZ_pZ,0,y_ZZ_p);
    RandomZZpx(y1,pkePara.N,pkePara.q_bit);
    y2=y_ZZ_pZ-y1;
    TimingResult timing = MeasureTimeMsAdaptive([&]() {
        ZZ_pX tmp = y1 + y2;
    }, cyctimes);
    PrintTimeMs("Dec algo time", timing);
}


void TimeEvalSubalgo(int msg_num, int degree_f,  int cyctimes) {
    //para
    PKE_Para pkePara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    pkePara.msg_bit = 32;
    pkePara.num_data = msg_num;
    pkePara.d = degree_f;
    PKE_Gen(pkePara, pkePk, pkeSk);
    ZZ_pXModulus modulus(pkePara.xN);
    HssGen(hssEk_1, hssEk_2, pkePara, pkeSk);
    //data
    Data data;
    GenerateData(data, pkePara, pkePk);

    //load
    vec_ZZ_pX db1,db2;
    db1.SetLength(2);
    db2.SetLength(2);
    TimingResult timing = MeasureTimeMs([&]() {
        int index=rand()%10;
        HSS_Mult(db1,pkePara,modulus,hssEk_1,data.C_X[rand()%pkePara.num_data]);
        db1[0]=db1[0]+data.PRF[index][0];
        db1[1]=db1[1]+data.PRF[index][1];
    }, cyctimes);
    PrintTimeMs("Load algo time", timing);


    //add1
    RandomZZpx(db1[0],pkePara.N,pkePara.q_bit);
    RandomZZpx(db1[1],pkePara.N,pkePara.q_bit);
    RandomZZpx(db2[0],pkePara.N,pkePara.q_bit);
    RandomZZpx(db2[1],pkePara.N,pkePara.q_bit);
    int index=rand()%10;
    vec_ZZ_pX add_tmp;
    add_tmp.SetLength(2);
    timing = MeasureTimeMsAdaptive([&]() {
        add_tmp[0]=db1[0]+db2[0]+data.PRF[index][0];
        add_tmp[1]=db1[1]+db2[1]+data.PRF[index][1];
    }, cyctimes);
    PrintTimeMs("Add1 algo time", timing);

    //add2
    vec_ZZ_pX C;
    C.SetLength(4);
    timing = MeasureTimeMsAdaptive([&]() {
        C[0]=data.C_X[0][0]+data.C_X[1][0];
        C[1]=data.C_X[0][1]+data.C_X[1][1];
        C[2]=data.C_X[0][2]+data.C_X[1][2];
        C[3]=data.C_X[0][3]+data.C_X[1][3];
    }, cyctimes);
    PrintTimeMs("Add2 algo time", timing);

    //mult
    timing = MeasureTimeMs([&]() {
        HSS_Mult(db1,pkePara,modulus,hssEk_1,data.C_X[rand()%pkePara.num_data]);
        int index=rand()%10;
        HSS_Mult(db1,pkePara,modulus,db1,data.C_X[rand()%pkePara.num_data]);
        db1[0]=db1[0]+data.PRF[index][0];
        db1[1]=db1[1]+data.PRF[index][1];
    }, cyctimes);
    PrintTimeMs("Mult algo time", timing);


    //out
    ZZ_pX yb_ZZ_pX;
    ZZX yb_ZZX;
    ZZ coeff;
    ZZ msg_size= power_ZZ(2,pkePara.msg_bit);
    RandomZZpx(yb_ZZ_pX,pkePara.N,pkePara.q_bit);
    timing = MeasureTimeMsAdaptive([&]() {
        conv(yb_ZZX,yb_ZZ_pX);
        for(int j=0;j<pkePara.N;j++)
        {
            GetCoeff(coeff,yb_ZZX,j);
            coeff=coeff%msg_size;
            SetCoeff(yb_ZZX,j,coeff);
        }
    }, cyctimes);
    PrintTimeMs("Output algo time", timing);

}

void Time_Eval(int msg_num, int degree_f, int cyctimes) {
    //para
    PKE_Para pkePara;
    vec_ZZ_pX pkePk, pkeSk, hssEk_1, hssEk_2;
    pkePk.SetLength(2);
    pkeSk.SetLength(2);
    hssEk_1.SetLength(2);
    hssEk_2.SetLength(2);
    pkePara.msg_bit = 32;
    pkePara.d = degree_f;
    pkePara.num_data = msg_num;
    PKE_Gen(pkePara, pkePk, pkeSk);
    ZZ_pXModulus modulus(pkePara.xN);
    HssGen(hssEk_1, hssEk_2, pkePara, pkeSk);
    //data
    Data data;
    GenerateData(data, pkePara, pkePk);
    vec_ZZ_pX y1, y2;
    int prf_key;

    vec_ZZ_pX C1;
    vec_ZZ_pX M1, M2;
    HSS_Enc(C1, pkePara, modulus, pkePk, ZZ(1));
    HssConvertInput(M1, pkePara, modulus, hssEk_1, C1);
    HssConvertInput(M2, pkePara, modulus, hssEk_2, C1);

    TimingResult timing = MeasureTimeMs([&]() {
        prf_key = 0;
        HssEvaluatePolyD2(y1, 1, data.C_X, pkePara, modulus, hssEk_1, prf_key, degree_f, M1);
    }, cyctimes);
    PrintTimeMs("Eval algo time", timing);
}


void HSS_TIME_TEST(int msg_num, int degree_f, int cyctimes)
{
    TimeGen(msg_num, degree_f, cyctimes);
    TimeEnc(msg_num, degree_f, cyctimes);
    Time_Dec(msg_num, degree_f, cyctimes);
    Time_Eval(msg_num, degree_f, cyctimes);

}
