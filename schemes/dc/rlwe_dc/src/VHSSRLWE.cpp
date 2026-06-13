
#include "VHSSRLWE.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace rlwe { namespace dc {

static void EncodeBinaryPolynomial(ZZ_pX &out, ZZ value, int bits)
{
    ZZX encoded;
    clear(out);
    for (int i = 0; i < bits; ++i)
    {
        SetCoeff(encoded, i, value % 2);
        value = (value - (value % 2)) / 2;
    }
    conv(out, encoded);
}

void GenerateData(Data &data, const PKE_Para& pkePara, const vec_ZZ_pX& pkePk)
{
    data.X.SetLength(pkePara.num_data);
    vec_ZZ_pX C_x, prf;
    C_x.SetLength(4);
    prf.SetLength(2);
    ZZ_pXModulus modulus(pkePara.xN);
    for (int i = 0; i < pkePara.num_data; i++)
    {
        NTL::RandomBits(data.X[i], pkePara.msg_bit);
        VHSS_Enc(C_x, pkePara, modulus, pkePk, data.X[i]);
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

void PKE_Enc(vec_ZZ_pX &c, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkePk, const ZZ &x)
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

void PKE_OKDM(vec_ZZ_pX &C, const PKE_Para &pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkePk, const ZZ &x)
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

// Helper: round one polynomial from mod q to mod p
static inline void RoundQToP(ZZ_pX& out, const ZZ_pX& in, const PKE_Para& para, PKEWorkspace& ws)
{
    conv(ws.temp, in);
    for (int i = 0; i < para.N; i++)
    {
        GetCoeff(ws.coeff, ws.temp, i);
        ws.coeff = (ws.coeff * para.twice_p + para.q) / (para.twice_q);
        ws.coeff = ws.coeff % para.p;
        if (ws.coeff > para.half_p)
        {
            ws.coeff -= para.p;
        }
        SetCoeff(ws.temp, i, ws.coeff);
    }
    conv(out, ws.temp);
}

// Helper: decrypt and round one half of the dual ciphertext
static inline void DDecOneHalf(
    ZZ_pX& out,
    const ZZ_pX& sk0,
    const ZZ_pX& sk1,
    const ZZ_pX& c0,
    const ZZ_pX& c1,
    const PKE_Para& para,
    const ZZ_pXModulus& modulus,
    PKEWorkspace& ws)
{
    MulMod(ws.temp1, sk0, c0, modulus);
    MulMod(ws.temp2, sk1, c1, modulus);
    ws.temp1 += ws.temp2;
    RoundQToP(out, ws.temp1, para, ws);
}

void PKE_DDec(vec_ZZ_pX &db, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkeSk, const vec_ZZ_pX& C)
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
    // switch (pkePara.d)
    // {
    // case 5:
    //     pkePara.N = 32768;
    //     pkePara.p_bit = 319;
    //     pkePara.q_bit = 662;
    //     break;
    // case 10:
    //     pkePara.N = 65536;
    //     pkePara.p_bit = 576;
    //     pkePara.q_bit = 1177;
    //     break;
    // case 15:
}

void HssGen(vec_ZZ_pX &hssEk_1, vec_ZZ_pX &hssEk_2,
             const PKE_Para& pkePara, const vec_ZZ_pX& pkeSk)
{

    RandomZZpx(hssEk_1[0], pkePara.N, pkePara.q_bit);
    RandomZZpx(hssEk_1[1], pkePara.N, pkePara.q_bit);

    hssEk_2[0] = pkeSk[0] - hssEk_1[0];
    hssEk_2[1] = pkeSk[1] - hssEk_1[1];
}

void VHSS_Enc(vec_ZZ_pX &C, const PKE_Para &pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkePk, const ZZ &x)
{
    C.SetLength(4);
    PKE_OKDM(C, pkePara, modulus, pkePk, x);
}

void VHSS_Mult(vec_ZZ_pX &db, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkeSk, const vec_ZZ_pX& C)
{
    db.SetLength(2);
    PKE_DDec(db, pkePara, modulus, pkeSk, C);
}

void HssConvertInput(vec_ZZ_pX &tb_y, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& ek, const vec_ZZ_pX& C_X)
{
    tb_y.SetLength(2);
    VHSS_Mult(tb_y, pkePara, modulus, ek, C_X);
}

void HssAddMemory(vec_ZZ_pX &tb, const vec_ZZ_pX &C_X, const vec_ZZ_pX& C_Y)
{
    tb[0] = C_X[0] + C_Y[0];
    tb[1] = C_X[1] + C_Y[1];
}

ZZ HssOutputCoeff(const ZZ_p& coeff, const PKE_Para& pkePara, const ZZ& output_mod)
{
    ZZ value = rep(coeff);
    if (value > pkePara.q / 2)
    {
        value -= pkePara.q;
    }
    value %= output_mod;
    if (value < 0)
    {
        value += output_mod;
    }
    return value;
}

ZZ HssOutputPolyAtTwo(const ZZ_pX& poly, const PKE_Para& pkePara, const ZZ& output_mod)
{
    ZZX coeffs;
    conv(coeffs, poly);

    ZZ result(0);
    ZZ power_of_two(1);
    const ZZ half_q = pkePara.q / 2;
    for (long i = 0; i <= deg(coeffs); ++i)
    {
        ZZ value;
        GetCoeff(value, coeffs, i);
        if (value > half_q)
        {
            value -= pkePara.q;
        }
        value %= output_mod;
        if (value < 0)
        {
            value += output_mod;
        }
        result += value * power_of_two;
        result %= output_mod;
        power_of_two *= 2;
        power_of_two %= output_mod;
    }
    return result;
}

// RMS-optimized recurrence: H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]
// Reduces VHSS_Mult calls from O(k*d^2) to O(k*d).
void HssEvaluateMPE(vec_ZZ_pX &y_b_res, int b, const vector<vec_ZZ_pX> &Ix, const PKE_Para &pkePara, ZZ_pXModulus modulus, const vec_ZZ_pX &pkeSk, int &prf_key, int degree_f, const vec_ZZ_pX &M1)
{
    int k = Ix.size();

    Mat<ZZ_pX> dp_prev, dp_curr;
    dp_prev.SetDims(1 + degree_f, 2);
    dp_curr.SetDims(1 + degree_f, 2);

    dp_prev[0] = M1;
    for (int s = 1; s <= degree_f; s++) {
        dp_prev[s].SetLength(2);
        dp_prev[s][0] = 0;
        dp_prev[s][1] = 0;
    }

    vec_ZZ_pX product;
    product.SetLength(2);

    for (int i = 1; i <= k; i++)
    {
        // dp_curr[0] = constant 1 (inherited from dp_prev[0])
        dp_curr[0] = dp_prev[0];

        // H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]  for s = 1..degree_f
        for (int s = 1; s <= degree_f; s++) {
            dp_curr[s].SetLength(2);
            dp_curr[s][0] = 0;
            dp_curr[s][1] = 0;
            // Start with H_{i-1}[s]
            HssAddMemoryInPlace(dp_curr[s], dp_prev[s]);
            // Add x_i * H_i[s-1]
            VHSS_Mult(product, pkePara, modulus, dp_curr[s - 1], Ix[i - 1]);
            HssAddMemoryInPlace(dp_curr[s], product);
        }
        dp_prev.swap(dp_curr);
    }
    y_b_res.SetLength(2);
    y_b_res[0] = 0;
    y_b_res[1] = 0;
    for (int s = 1; s <= degree_f; s++) {
        HssAddMemoryInPlace(y_b_res, dp_prev[s]);
    }
}


void VHSS_Gen(VHSS_Para &vhssPara, const PKE_Para& pkePara, const ZZ_pXModulus& modulus, const vec_ZZ_pX& pkeSk)
{
    ZZ alpha_scalar;
    do { RandomBits(alpha_scalar, 255); } while (IsZero(alpha_scalar));
    EncodeBinaryPolynomial(vhssPara.alpha, alpha_scalar, 255);
    vec_ZZ_pX alpha_pkeSk;
    alpha_pkeSk.SetLength(2);
    vhssPara.vhssEk_1.SetLength(2);
    vhssPara.vhssEk_2.SetLength(2);
    vhssPara.vhssEk_3.SetLength(2);
    vhssPara.vhssEk_4.SetLength(2);

    alpha_pkeSk[0] = vhssPara.alpha;
    MulMod(alpha_pkeSk[1], vhssPara.alpha, pkeSk[1], modulus);

    RandomZZpx(vhssPara.vhssEk_1[0], pkePara.N, pkePara.q_bit);
    RandomZZpx(vhssPara.vhssEk_1[1], pkePara.N, pkePara.q_bit);
    RandomZZpx(vhssPara.vhssEk_3[0], pkePara.N, pkePara.q_bit);
    RandomZZpx(vhssPara.vhssEk_3[1], pkePara.N, pkePara.q_bit);

    vhssPara.vhssEk_2[0] = pkeSk[0] + vhssPara.vhssEk_1[0];
    vhssPara.vhssEk_2[1] = pkeSk[1] + vhssPara.vhssEk_1[1];
    vhssPara.vhssEk_4[0] = alpha_pkeSk[0] + vhssPara.vhssEk_3[0];
    vhssPara.vhssEk_4[1] = alpha_pkeSk[1] + vhssPara.vhssEk_3[1];
}

}}} // namespace pvhss::rlwe::dc
