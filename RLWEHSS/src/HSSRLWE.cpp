
#include "HSSRLWE.h"

void GenData(Data &data, PKE_Para pkePara, vec_ZZ_pX pkePk) {
    data.X.SetLength(pkePara.num_data);
    vec_ZZ_pX C_x, prf;
    C_x.SetLength(4);
    prf.SetLength(2);
    ZZ_pXModulus modulus(pkePara.xN);
    for (int i = 0; i < pkePara.num_data; i++) {
        RandomBits(data.X[i], pkePara.msg_bit);
        HSS_Enc(C_x, pkePara, modulus, pkePk, data.X[i]);
        data.C_X.append(C_x);
    }
    for (int i = 0; i < 10; i++) {
        Random_ZZ_pX(prf[0], pkePara.N, pkePara.q_bit);
        Random_ZZ_pX(prf[1], pkePara.N, pkePara.q_bit);
        data.PRF.append(prf);
    }
}

void PKE_Gen(PKE_Para &pkePara, vec_ZZ_pX &pkePk, vec_ZZ_pX &pkeSk) {
    //initialize the parameters
    SetPara(pkePara);

    NTL::power(pkePara.p, 2, pkePara.p_bit);
    NTL::power(pkePara.q, 2, pkePara.q_bit);
    ZZ_p::init(pkePara.q);

    SetCoeff(pkePara.xN, 0, 1);
    SetCoeff(pkePara.xN, pkePara.N, 1);
    ZZ_pXModulus modulus(pkePara.xN);

    pkePara.twice_p=2*pkePara.p;
    pkePara.twice_q=2*pkePara.q;
    pkePara.half_p=pkePara.p/2;

    //gen sk
    ZZ_pX hat_s, e;
    Random_ZZ_pX(pkePk[0], pkePara.N, pkePara.q_bit);
    RLWESecretKey(hat_s, pkePara.N, pkePara.hsk);
    GaussRand(e, pkePara.N);


    MulMod(pkePk[1], pkePk[0], hat_s, modulus);
    pkePk[1] = pkePk[1] + e;
    //gen sk
    SetCoeff(pkeSk[0], 0, 1);
    pkeSk[1] = hat_s;

}


void PKE_Enc(vec_ZZ_pX &c, const PKE_Para pkePara, ZZ_pXModulus modulus,vec_ZZ_pX pkePk, const ZZ &x) {
    ZZ_pX v, e1, e2, x_ZZ_pX;
    ZZ q_div_p = pkePara.q / pkePara.p;
    ZZ_p coeff;
    RLWESecretKey(v, pkePara.N, pkePara.hsk);
    GaussRand(e1, pkePara.N);
    GaussRand(e2, pkePara.N);
    MulMod(c[0], pkePk[1], v, modulus);
    c[0] = c[0] + e1;
    conv(coeff,q_div_p*x);
    SetCoeff(x_ZZ_pX, 0, coeff);
    c[0] = c[0] + x_ZZ_pX;
    MulMod(c[1], pkePk[0], v, modulus);
    c[1] = e2 - c[1];
}

void PKE_OKDM(vec_ZZ_pX &C, const PKE_Para &pkePara, ZZ_pXModulus& modulus,vec_ZZ_pX &pkePk, const ZZ &x) {
    ZZ zero;
    zero = 0;
    ZZ q_div_p = pkePara.q / pkePara.p;
    vec_ZZ_pX c_xs1, c_xs2;
    ZZ_p coeff;
    ZZ_pX x_ZZ_pX;
    c_xs1.SetLength(2);
    c_xs2.SetLength(2);

    PKE_Enc(c_xs1, pkePara, modulus,pkePk, x);
    PKE_Enc(c_xs2, pkePara, modulus,pkePk, zero);
    conv(coeff,q_div_p*x);
    SetCoeff(x_ZZ_pX, 0, coeff);
    c_xs2[1] = c_xs2[1] + x_ZZ_pX;
    C[0] = c_xs1[0];
    C[1] = c_xs1[1];
    C[2] = c_xs2[0];
    C[3] = c_xs2[1];
}

void PKE_DDec(vec_ZZ_pX &db, const PKE_Para pkePara, ZZ_pXModulus modulus,vec_ZZ_pX pkeSk, vec_ZZ_pX C) {
    ZZ coeff;
    ZZX temp;
    ZZ_pX temp1, temp2;
    MulMod(temp1, pkeSk[0], C[0], modulus);
    MulMod(temp2, pkeSk[1], C[1], modulus);
    temp1 = temp1 + temp2;
    conv(temp,temp1);
    for (int i = 0; i < pkePara.N; i++) {
        GetCoeff(coeff, temp, i);
        coeff = (coeff*pkePara.twice_p+pkePara.q)/(pkePara.twice_q);
        coeff=coeff%pkePara.p;
        if(coeff>pkePara.half_p)
        {
            coeff-=pkePara.p;
        }
        SetCoeff(temp, i, coeff);
    }
    conv(db[0],temp);
    MulMod(temp1, pkeSk[0], C[2], modulus);
    MulMod(temp2, pkeSk[1], C[3], modulus);
    temp1 = temp1 + temp2;
    conv(temp,temp1);
    for (int i = 0; i < pkePara.N; i++) {
        GetCoeff(coeff, temp, i);
        coeff = (coeff*pkePara.twice_p+pkePara.q)/(pkePara.twice_q);
        coeff=coeff%pkePara.p;
        if(coeff>pkePara.half_p)
        {
            coeff-=pkePara.p;
        }
        SetCoeff(temp, i, coeff);
    }
    conv(db[1],temp);
}

void SetPara(PKE_Para & pkePara) {
   
        switch(pkePara.d){
            case 5:
                pkePara.N=32768;
                pkePara.p_bit=319;
                pkePara.q_bit=662;
                break;
                break;
            case 10:
                pkePara.N=65536;
                pkePara.p_bit=576;
                pkePara.q_bit=1177;
                break;
                break;
            case 15:
                pkePara.N=65536;
                pkePara.p_bit=576;
                pkePara.q_bit=1177;
                break;
            default:printf("error\n"); break;
        }

}


void HSS_Gen(vec_ZZ_pX &hssEk_1, vec_ZZ_pX &hssEk_2,
             PKE_Para pkePara, vec_ZZ_pX pkeSk) {

    Random_ZZ_pX(hssEk_1[0], pkePara.N, pkePara.q_bit);
    Random_ZZ_pX(hssEk_1[1], pkePara.N, pkePara.q_bit);

    hssEk_2[0] = pkeSk[0] - hssEk_1[0];
    hssEk_2[1] = pkeSk[1] - hssEk_1[1];


}

void HSS_Enc(vec_ZZ_pX &C, const PKE_Para &pkePara, ZZ_pXModulus &modulus, vec_ZZ_pX &pkePk, const ZZ &x) {
    PKE_OKDM(C, pkePara, modulus, pkePk, x);
}

void HSS_Mult(vec_ZZ_pX &db, const PKE_Para pkePara, ZZ_pXModulus modulus, vec_ZZ_pX pkeSk, vec_ZZ_pX C) {
    PKE_DDec(db, pkePara, modulus, pkeSk, C);
}

void HSS_ConvertInput(vec_ZZ_pX &tb_y, const PKE_Para pkePara, ZZ_pXModulus modulus, vec_ZZ_pX ek, vec_ZZ_pX C_X) {
    tb_y.SetLength(2);
    HSS_Mult(tb_y, pkePara, modulus, ek, C_X);
}

void HSS_Addmemory(vec_ZZ_pX &tb, vec_ZZ_pX C_X, vec_ZZ_pX C_Y) {
    tb.SetLength(2);
    tb[0] = C_X[0] + C_Y[0];
    tb[1] = C_X[1] + C_Y[1];
}

void f(vec_ZZ_pX &tb, int b, int d, int num_data, int loop, int beg_ind, int *ind_var,
       PKE_Para pkePara, ZZ_pXModulus modulus, vec_ZZ_pX ek, Vec<vec_ZZ_pX> C_X, Vec<vec_ZZ_pX> PRF, int &prfkey) {
    if (loop == d) {
        vec_ZZ_pX tb_temp;
        tb_temp.SetLength(2);
        if (d == 1) {
            prfkey = (prfkey + 1) % 10;
            HSS_Mult(tb_temp, pkePara, modulus, ek, C_X[ind_var[0]]);

            if (b == 1) {
                tb_temp[0] = tb_temp[0] + PRF[prfkey][0];
                tb_temp[1] = tb_temp[1] + PRF[prfkey][1];
            } else {
                tb_temp[0] = tb_temp[0] - PRF[prfkey][0];
                tb_temp[1] = tb_temp[1] - PRF[prfkey][1];
            }

            tb[0] = tb[0] + tb_temp[0];
            tb[1] = tb[1] + tb_temp[1];

        } else {
            HSS_Mult(tb_temp, pkePara, modulus, ek, C_X[ind_var[0]]);
            prfkey = (prfkey + 1) % 10;

            if (b == 1) {
                tb_temp[0] = tb_temp[0] + PRF[prfkey][0];
                tb_temp[1] = tb_temp[1] + PRF[prfkey][1];
            } else {
                tb_temp[0] = tb_temp[0] - PRF[prfkey][0];
                tb_temp[1] = tb_temp[1] - PRF[prfkey][1];
            }

            for (int i = 1; i < d; i++) {
                HSS_Mult(tb_temp, pkePara, modulus, tb_temp, C_X[ind_var[i]]);
                prfkey = (prfkey + 1) % 10;
                if (b == 1) {
                    tb_temp[0] = tb_temp[0] + PRF[prfkey][0];
                    tb_temp[1] = tb_temp[1] + PRF[prfkey][1];
                } else {
                    tb_temp[0] = tb_temp[0] - PRF[prfkey][0];
                    tb_temp[1] = tb_temp[1] - PRF[prfkey][1];
                }
            }
            tb[0] = tb[0] + tb_temp[0];
            tb[1] = tb[1] + tb_temp[1];
        }
    } else {
        loop = loop + 1;
        for (int i = beg_ind; i < num_data; i++) {
            ind_var[loop - 1] = i;
            f(tb, b, d, num_data, loop, i, ind_var,pkePara, modulus, ek, C_X, PRF, prfkey);
        }
    }
}


void HSS_Eval(ZZ_pX &tb_y, int b, PKE_Para pkePara, ZZ_pXModulus modulus, vec_ZZ_pX ek, Vec<vec_ZZ_pX> C_X,
              Vec<vec_ZZ_pX> PRF, int &prfkey) {
    int loop = 0;
    int beg_ind = 0;
    int *ind_var = (int *) malloc(sizeof(int) * pkePara.d);
    vec_ZZ_pX tb, tb_temp;
    tb.SetLength(2);
    tb_temp.SetLength(2);
    for (int i = 1; i < pkePara.d + 1; i++) {
        f(tb_temp, b, i, pkePara.num_data, loop, beg_ind, ind_var, pkePara, modulus, ek, C_X, PRF, prfkey);
        prfkey = 1;
        tb[0] = tb[0] + tb_temp[0];
        tb[1] = tb[1] + tb_temp[1];
        tb_temp[0] = 0;
        tb_temp[1] = 0;
    }
    tb_y = tb[0];
}

void HSS_Evaluate_P_d2(vec_ZZ_pX &y_b_res, int b, const vector<vec_ZZ_pX> &Ix, const PKE_Para &pkePara, ZZ_pXModulus modulus, const vec_ZZ_pX &pkeSk, int &prf_key, int degree_f)
{
    vec_ZZ_pX tmp1, tmp2;

    int k = Ix.size(); 

    Vec<vec_ZZ_pX> dp_prev, dp_curr;
    dp_prev.SetLength(1+degree_f);
    dp_curr.SetLength(1+degree_f);


}