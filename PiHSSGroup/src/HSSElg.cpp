#include "HSSElg.h"

void Elgamal_Gen(Elgamal_PK &pk, Elgamal_SK &d, int skLen)
{
    ZZ p, q;

    // GenGermainPrime(p, 1536); // safe prime
    p = conv<ZZ>("2410312426921032588580116606028314112912093247945688951359675039065257391591803200669085024107346049663448766280888004787862416978794958324969612987890774651455213339381625224770782077917681499676845543137387820057597345857904599109461387122099507964997815641342300677629473355281617428411794163967785870370368969109221591943054232011562758450080579587850900993714892283476646631181515063804873375182260506246992837898705971012525843324401232986857004760339321639");
    // GenGermainPrime(q, 1536);
    q = conv<ZZ>("2410312426921032588580116606028314112912093247945688951359675039065257391591803200669085024107346049663448766280888004787862416978794958324969612987890774651455213339381625224770782077917681499676845543137387820057597345857904599109461387122099507964997815641342300677629473355281617428411794163967785870370368969109221591943054232011562758450080579587850900993714892283476646631181515063804873375182260506246992837898705971012525843324401232986857004760339319223");

    mul(pk.N, p, q);        // N = p * q
    mul(pk.N2, pk.N, pk.N); // N^2

    RandomBnd(pk.g, pk.N2);
    while (Jacobi(pk.g, pk.N) != 1)
    {
        RandomBnd(pk.g, pk.N2);
    }

    add(pk.f, pk.N, 1);

    RandomBits(d, skLen);
    PowerMod(pk.h, pk.g, d, pk.N2);
}

void Elgamal_Enc(Elgamal_CT &ct, const Elgamal_PK &pk, const ZZ &x)
{
    ZZ r;
    RandomBnd(r, pk.N);

    PowerMod(ct[0], pk.g, r, pk.N2);
    PowerMod(ct[1], pk.h, r, pk.N2);
    MulMod(ct[1], ct[1], 1 + pk.N * x, pk.N2);
}

void Elgamal_skEnc(Elgamal_CT &ct, const Elgamal_PK &pk, const ZZ &x)
{
    ZZ r;
    RandomBnd(r, pk.N);

    PowerMod(ct[0], pk.g, r, pk.N2);
    MulMod(ct[0], ct[0], 1 - pk.N * x, pk.N2);
    PowerMod(ct[1], pk.h, r, pk.N2);
}

void Elgamal_Dec(ZZ &x, const Elgamal_PK &pk, const Elgamal_SK &sk, const Elgamal_CT &ct)
{
    ZZ temp;
    PowerMod(temp, ct[0], -sk, pk.N2);
    MulMod(temp, ct[1], temp, pk.N2);

    sub(temp, temp, 1);
    div(x, temp, pk.N);
}

void HSS_Gen(HSS_PK &pk, HSS_EK &ek0, HSS_EK &ek1, int skLen)
{
    Elgamal_SK s;
    Elgamal_Gen(pk, s, skLen);

    RandomBits(ek0, skLen);
    add(ek1, ek0, s);
}


void HSS_Input(HSS_CT &I, const HSS_PK &pk, const ZZ &x)
{
    Elgamal_Enc(I[0], pk, x);
    Elgamal_skEnc(I[1], pk, x);
}

void HSS_ConvertInput(HSS_MV &Mx, int idx, const HSS_PK &pk, const HSS_EK &ek, const HSS_CT &Ix, int &prf_key)
{
    HSS_MV M1;
    M1[0] = idx;
    M1[1] = ek;
    HSS_Mul(Mx, idx, pk, Ix, M1, prf_key);
}

void HSS_Mul(HSS_MV &Mz, int idx, const HSS_PK &pk, const HSS_CT &Ix, const HSS_MV &My, int &prf_key)
{
    ZZ temp1, temp2;
    PowerMod(temp1, Ix[0][1], My[0], pk.N2);
    PowerMod(temp2, Ix[0][0], -My[1], pk.N2);
    MulMod(Mz[0], temp1, temp2, pk.N2);
    HSS_DDLog(Mz[0], pk, Mz[0]);
    Mz[0] = PRF_ZZ(prf_key++, pk.N) + Mz[0];

    PowerMod(temp1, Ix[1][1], My[0], pk.N2);
    PowerMod(temp2, Ix[1][0], -My[1], pk.N2);
    MulMod(Mz[1], temp1, temp2, pk.N2);
    HSS_DDLog(Mz[1], pk, Mz[1]);
    Mz[1] = PRF_ZZ(prf_key++, pk.N) + Mz[1];

}

void HSS_DDLog(ZZ &z, const HSS_PK &pk, const ZZ &g)
{
    ZZ h1, h, temp1;
    DivRem(h1, h, g, pk.N); // h = g % N; h1 = g / N
    InvMod(temp1, h, pk.N);
    MulMod(z, h1, temp1, pk.N);
}

void HSS_AddMemory(HSS_MV &Mz, const HSS_PK &pk, const HSS_MV &Mx, const HSS_MV &My)
{
    add(Mz[0], Mx[0], My[0]);
    add(Mz[1], Mx[1], My[1]);
}

void HSS_Evaluate(HSS_MV &y_b_res, int b, const vector<HSS_CT> &Ix, const HSS_PK &pk, const HSS_EK &ekb, int &prf_key, vector<vector<int>> F_TEST)
{
    HSS_MV M1, Monomial, tmp;
    M1[0] = b;
    M1[1] = ekb;

    y_b_res[0] = 0;
    y_b_res[1] = 0;

    int i, j, k;
    for (i = 0; i < F_TEST.size(); ++i)
    {
        copy(begin(M1), end(M1), begin(Monomial));
        for (j = 0; j < Ix.size(); ++j)
        {
            for (k = 0; k < F_TEST[i][j]; ++k)
            {
                HSS_Mul(tmp, b, pk, Ix[j], Monomial, prf_key);
                copy(begin(tmp), end(tmp), begin(Monomial));
            }
        }
        HSS_AddMemory(y_b_res, pk, y_b_res, Monomial);
    }
}


void HSS_Evaluate_P_d2(HSS_MV &y_b_res, int b, const vector<HSS_CT> &Ix, const HSS_PK &pk, const HSS_EK &ekb, int &prf_key, int degree_f)
{
    HSS_MV tmp1, tmp2;

    int k = Ix.size(); 
    Vec<HSS_MV> dp_prev, dp_curr;
    dp_prev.SetLength(1+degree_f);
    dp_curr.SetLength(1+degree_f);
    
    dp_prev[0][0] = b;
    dp_prev[0][1] = ekb;
    
    // 动态规划填表
    for (int i = 1; i <= k; i++) { // 依次加入 x1, x2, ..., x5
        for (int s = 0; s <= degree_f; s++) { // 目标和从 0 到 d
            dp_curr[s][0] = 0;
            dp_curr[s][1] = 0;
            HSS_AddMemory(dp_curr[s], pk, dp_curr[s], dp_prev[s]);
            for (int j = 1; j <= s; j++) { 
                copy(begin(dp_prev[s - j]), end(dp_prev[s - j]), begin(tmp1));
                for (int h=0; h < j;++h) {
                    HSS_Mul(tmp2, b, pk, Ix[i - 1], tmp1, prf_key);
                    copy(begin(tmp2), end(tmp2), begin(tmp1));
                }
                HSS_AddMemory(dp_curr[s], pk, dp_curr[s], tmp1);
            }
        }
        dp_prev.swap(dp_curr);
    }

    y_b_res[0] = 0;
    y_b_res[1] = 0;
    for (int s = 1; s <= degree_f; s++) {
        HSS_AddMemory(y_b_res, pk, y_b_res, dp_prev[s]); 
    }

}

void HSS_Dec(ZZ &z, const HSS_MV &Mx0, const HSS_MV &Mx1)
{
    sub(z, Mx1[0], Mx0[0]);
}