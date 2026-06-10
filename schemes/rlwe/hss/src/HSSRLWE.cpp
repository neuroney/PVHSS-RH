
#include "HSSRLWE.h"

namespace pvhss { namespace rlwe { namespace hss {

namespace
{
struct PKEWorkspace
{
    ZZ coeff;
    ZZX temp;
    ZZ_pX temp1;
    ZZ_pX temp2;
};

void RoundQToP(ZZ_pX &out, const ZZ_pX &in, const PKE_Para &pkePara, PKEWorkspace &ws)
{
    conv(ws.temp, in);
    for (int i = 0; i < pkePara.N; i++)
    {
        GetCoeff(ws.coeff, ws.temp, i);
        ws.coeff = (ws.coeff * pkePara.twice_p + pkePara.q) / (pkePara.twice_q);
        ws.coeff = ws.coeff % pkePara.p;
        if (ws.coeff > pkePara.half_p)
        {
            ws.coeff -= pkePara.p;
        }
        SetCoeff(ws.temp, i, ws.coeff);
    }
    conv(out, ws.temp);
}

void DDecOneHalf(ZZ_pX &out, const ZZ_pX &sk0, const ZZ_pX &sk1,
                 const ZZ_pX &c0, const ZZ_pX &c1, const PKE_Para &pkePara,
                 const ZZ_pXModulus &modulus, PKEWorkspace &ws)
{
    MulMod(ws.temp1, sk0, c0, modulus);
    MulMod(ws.temp2, sk1, c1, modulus);
    ws.temp1 += ws.temp2;
    RoundQToP(out, ws.temp1, pkePara, ws);
}
}

void GenerateData(Data &data, const PKE_Para &pkePara, const vec_ZZ_pX &pkePk)
{
    data.X.SetLength(pkePara.num_data);
    vec_ZZ_pX C_x, prf;
    C_x.SetLength(4);
    prf.SetLength(2);
    ZZ_pXModulus modulus(pkePara.xN);
    for (int i = 0; i < pkePara.num_data; i++)
    {
        RandomBits(data.X[i], pkePara.msg_bit);
        HSS_Enc(C_x, pkePara, modulus, pkePk, data.X[i]);
        data.C_X.append(C_x);
    }
    for (int i = 0; i < 10; i++)
    {
        RandomZZpx(prf[0], pkePara.N, pkePara.q_bit);
        RandomZZpx(prf[1], pkePara.N, pkePara.q_bit);
        data.PRF.append(prf);
    }
}

void PKE_Gen(PKE_Para &pkePara, vec_ZZ_pX &pkePk, vec_ZZ_pX &pkeSk)
{
    // initialize the parameters
    SetParams(pkePara);

    NTL::power(pkePara.p, 2, pkePara.p_bit);
    NTL::power(pkePara.q, 2, pkePara.q_bit);
    ZZ_p::init(pkePara.q);

    SetCoeff(pkePara.xN, 0, 1);
    SetCoeff(pkePara.xN, pkePara.N, 1);
    ZZ_pXModulus modulus(pkePara.xN);

    pkePara.twice_p = 2 * pkePara.p;
    pkePara.twice_q = 2 * pkePara.q;
    pkePara.half_p = pkePara.p / 2;

    // gen sk
    ZZ_pX hat_s, e;
    RandomZZpx(pkePk[0], pkePara.N, pkePara.q_bit);
    RlweSecretKey(hat_s, pkePara.N, pkePara.hsk);
    GaussRandom(e, pkePara.N);

    MulMod(pkePk[1], pkePk[0], hat_s, modulus);
    pkePk[1] = pkePk[1] + e;
    // gen sk
    SetCoeff(pkeSk[0], 0, 1);
    pkeSk[1] = hat_s;
}

void PKE_Enc(vec_ZZ_pX &c, const PKE_Para &pkePara, const ZZ_pXModulus &modulus,
             const vec_ZZ_pX &pkePk, const ZZ &x)
{
    ZZ_pX v, e1, e2, x_ZZ_pX;
    ZZ q_div_p = pkePara.q / pkePara.p;
    ZZ_p coeff;
    RlweSecretKey(v, pkePara.N, pkePara.hsk);
    GaussRandom(e1, pkePara.N);
    GaussRandom(e2, pkePara.N);
    MulMod(c[0], pkePk[1], v, modulus);
    c[0] = c[0] + e1;
    conv(coeff, q_div_p * x);
    SetCoeff(x_ZZ_pX, 0, coeff);
    c[0] = c[0] + x_ZZ_pX;
    MulMod(c[1], pkePk[0], v, modulus);
    c[1] = e2 - c[1];
}

void PKE_OKDM(vec_ZZ_pX &C, const PKE_Para &pkePara, const ZZ_pXModulus &modulus,
              const vec_ZZ_pX &pkePk, const ZZ &x)
{
    C.SetLength(4);
    ZZ zero;
    zero = 0;
    ZZ q_div_p = pkePara.q / pkePara.p;
    vec_ZZ_pX c_xs1, c_xs2;
    ZZ_p coeff;
    ZZ_pX x_ZZ_pX;
    c_xs1.SetLength(2);
    c_xs2.SetLength(2);

    PKE_Enc(c_xs1, pkePara, modulus, pkePk, x);
    PKE_Enc(c_xs2, pkePara, modulus, pkePk, zero);
    conv(coeff, q_div_p * x);
    SetCoeff(x_ZZ_pX, 0, coeff);
    c_xs2[1] = c_xs2[1] + x_ZZ_pX;
    C[0] = c_xs1[0];
    C[1] = c_xs1[1];
    C[2] = c_xs2[0];
    C[3] = c_xs2[1];
}

void PKE_DDec(vec_ZZ_pX &db, const PKE_Para &pkePara, const ZZ_pXModulus &modulus,
              const vec_ZZ_pX &pkeSk, const vec_ZZ_pX &C)
{
    PKEWorkspace ws;
    DDecOneHalf(db[0], pkeSk[0], pkeSk[1], C[0], C[1], pkePara, modulus, ws);
    DDecOneHalf(db[1], pkeSk[0], pkeSk[1], C[2], C[3], pkePara, modulus, ws);
}

void SetParams(PKE_Para &pkePara)
{

    pkePara.N = 32768;
    pkePara.p_bit = 319;
    pkePara.q_bit = 662;
}

void HssGen(vec_ZZ_pX &hssEk_1, vec_ZZ_pX &hssEk_2,
            const PKE_Para &pkePara, const vec_ZZ_pX &pkeSk)
{

    RandomZZpx(hssEk_1[0], pkePara.N, pkePara.q_bit);
    RandomZZpx(hssEk_1[1], pkePara.N, pkePara.q_bit);

    hssEk_2[0] = pkeSk[0] - hssEk_1[0];
    hssEk_2[1] = pkeSk[1] - hssEk_1[1];
}

void HSS_Enc(vec_ZZ_pX &C, const PKE_Para &pkePara, const ZZ_pXModulus &modulus,
             const vec_ZZ_pX &pkePk, const ZZ &x)
{
    C.SetLength(4);
    PKE_OKDM(C, pkePara, modulus, pkePk, x);
}

void HSS_Mult(vec_ZZ_pX &db, const PKE_Para &pkePara, const ZZ_pXModulus &modulus,
              const vec_ZZ_pX &pkeSk, const vec_ZZ_pX &C)
{
    db.SetLength(2);
    PKE_DDec(db, pkePara, modulus, pkeSk, C);
}

void HssConvertInput(vec_ZZ_pX &tb_y, const PKE_Para &pkePara,
                     const ZZ_pXModulus &modulus, const vec_ZZ_pX &ek,
                     const vec_ZZ_pX &C_X)
{
    tb_y.SetLength(2);
    HSS_Mult(tb_y, pkePara, modulus, ek, C_X);
}

void HssAddMemory(vec_ZZ_pX &tb, const vec_ZZ_pX &C_X, const vec_ZZ_pX &C_Y)
{
    tb[0] = C_X[0] + C_Y[0];
    tb[1] = C_X[1] + C_Y[1];
}

void HssEvaluatePolyD2(vec_ZZ_pX &y_b_res, int b, const Vec<vec_ZZ_pX> &Ix,
                       const PKE_Para &pkePara, ZZ_pXModulus modulus,
                       const vec_ZZ_pX &pkeSk, int &prf_key, int degree_f,
                       const vec_ZZ_pX &M1)
{
    (void)b;
    (void)prf_key;

    int k = Ix.length();

    Mat<ZZ_pX> dp_prev, dp_curr;
    dp_prev.SetDims(1 + degree_f, 2);
    dp_curr.SetDims(1 + degree_f, 2);

    dp_prev[0] = M1;
    for (int s = 1; s <= degree_f; s++)
    {
        dp_prev[s].SetLength(2);
        dp_prev[s][0] = 0;
        dp_prev[s][1] = 0;
    }

    vec_ZZ_pX chain, next;
    chain.SetLength(2);
    next.SetLength(2);

    for (int i = 1; i <= k; i++)
    {
        for (int s = 0; s <= degree_f; s++)
        {
            dp_curr[s].SetLength(2);
            dp_curr[s][0] = 0;
            dp_curr[s][1] = 0;
            HssAddMemoryInPlace(dp_curr[s], dp_prev[s]);
        }

        for (int r = 0; r < degree_f; r++)
        {
            chain[0] = dp_prev[r][0];
            chain[1] = dp_prev[r][1];

            for (int j = 1; r + j <= degree_f; j++)
            {
                HSS_Mult(next, pkePara, modulus, chain, Ix[i - 1]);
                chain.swap(next);

                HssAddMemoryInPlace(dp_curr[r + j], chain);
            }
        }
        dp_prev.swap(dp_curr);
    }
    y_b_res.SetLength(2);
    y_b_res[0] = 0;
    y_b_res[1] = 0;
    for (int s = 1; s <= degree_f; s++)
    {
        HssAddMemoryInPlace(y_b_res, dp_prev[s]);
    }
}

}}} // namespace pvhss::rlwe::hss
