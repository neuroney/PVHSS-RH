#include "HSSElg.h"

using namespace NTL;
using namespace std;

namespace pvhss { namespace group { namespace hss {

void ElgamalGen(ElgamalPublicKey &pk, ElgamalSecretKey &d, int skLen)
{
    ZZ p, q;

    // GenGermainPrime(p, 1536); // safe prime
    p = conv<ZZ>("2410312426921032588580116606028314112912093247945688951359675039065257391591803200669085024107346049663448766280888004787862416978794958324969612987890774651455213339381625224770782077917681499676845543137387820057597345857904599109461387122099507964997815641342300677629473355281617428411794163967785870370368969109221591943054232011562758450080579587850900993714892283476646631181515063804873375182260506246992837898705971012525843324401232986857004760339321639");
    // GenGermainPrime(q, 1536);
    q = conv<ZZ>("2410312426921032588580116606028314112912093247945688951359675039065257391591803200669085024107346049663448766280888004787862416978794958324969612987890774651455213339381625224770782077917681499676845543137387820057597345857904599109461387122099507964997815641342300677629473355281617428411794163967785870370368969109221591943054232011562758450080579587850900993714892283476646631181515063804873375182260506246992837898705971012525843324401232986857004760339319223");

    mul(pk.N, p, q);        // N = p * q
    mul(pk.N2, pk.N, pk.N); // N^2

    NTL::RandomBnd(pk.g, pk.N2);
    while (Jacobi(pk.g, pk.N) != 1)
    {
        NTL::RandomBnd(pk.g, pk.N2);
    }

    add(pk.f, pk.N, 1);

    NTL::RandomBits(d, skLen);
    PowerMod(pk.h, pk.g, d, pk.N2);
}

void ElgamalEnc(ElgamalCiphertext &ct, const ElgamalPublicKey &pk, const ZZ &x)
{
    ZZ r;
    NTL::RandomBnd(r, pk.N);

    PowerMod(ct[0], pk.g, r, pk.N2);
    PowerMod(ct[1], pk.h, r, pk.N2);
    MulMod(ct[1], ct[1], 1 + pk.N * x, pk.N2);
}

void ElgamalSkEnc(ElgamalCiphertext &ct, const ElgamalPublicKey &pk, const ZZ &x)
{
    ZZ r;
    NTL::RandomBnd(r, pk.N);

    PowerMod(ct[0], pk.g, r, pk.N2);
    MulMod(ct[0], ct[0], 1 - pk.N * x, pk.N2);
    PowerMod(ct[1], pk.h, r, pk.N2);
}

void ElgamalDec(ZZ &x, const ElgamalPublicKey &pk, const ElgamalSecretKey &sk, const ElgamalCiphertext &ct)
{
    ZZ temp;
    PowerMod(temp, ct[0], -sk, pk.N2);
    MulMod(temp, ct[1], temp, pk.N2);

    sub(temp, temp, 1);
    div(x, temp, pk.N);
}

void HssGen(HssPublicKey &pk, HssEvalKey &ek0, HssEvalKey &ek1, int skLen)
{
    ElgamalSecretKey s;
    pvhss::group::hss::ElgamalGen(pk, s, skLen);

    NTL::RandomBits(ek0, skLen);
    add(ek1, ek0, s);
}


void HssInput(HssCiphertext &I, const HssPublicKey &pk, const ZZ &x)
{
    pvhss::group::hss::ElgamalEnc(I[0], pk, x);
    pvhss::group::hss::ElgamalSkEnc(I[1], pk, x);
}

void HssConvertInput(HssMemoryValue &Mx, int idx, const HssPublicKey &pk, const HssEvalKey &ek, const HssCiphertext &Ix, int &prf_key)
{
    HssMemoryValue M1;
    M1[0] = idx;
    M1[1] = ek;
    HssMul(Mx, idx, pk, Ix, M1, prf_key);
}

void HssMul(HssMemoryValue &Mz, int idx, const HssPublicKey &pk, const HssCiphertext &Ix, const HssMemoryValue &My, int &prf_key)
{
    ZZ temp1, temp2;
    PowerMod(temp1, Ix[0][1], My[0], pk.N2);
    PowerMod(temp2, Ix[0][0], -My[1], pk.N2);
    MulMod(Mz[0], temp1, temp2, pk.N2);
    HssDdlog(Mz[0], pk, Mz[0]);
    Mz[0] = PrfZZ(prf_key++, pk.N) + Mz[0];

    PowerMod(temp1, Ix[1][1], My[0], pk.N2);
    PowerMod(temp2, Ix[1][0], -My[1], pk.N2);
    MulMod(Mz[1], temp1, temp2, pk.N2);
    HssDdlog(Mz[1], pk, Mz[1]);
    Mz[1] = PrfZZ(prf_key++, pk.N) + Mz[1];

}

void HssDdlog(ZZ &z, const HssPublicKey &pk, const ZZ &g)
{
    ZZ h1, h, temp1;
    DivRem(h1, h, g, pk.N); // h = g % N; h1 = g / N
    InvMod(temp1, h, pk.N);
    MulMod(z, h1, temp1, pk.N);
}

void HssAddMemory(HssMemoryValue &Mz, const HssPublicKey &pk, const HssMemoryValue &Mx, const HssMemoryValue &My)
{
    add(Mz[0], Mx[0], My[0]);
    add(Mz[1], Mx[1], My[1]);
}

void HssEvaluate(HssMemoryValue &y_b_res, int b, const vector<HssCiphertext> &Ix, const HssPublicKey &pk, const HssEvalKey &ekb, int &prf_key, vector<vector<int>> F_TEST)
{
    HssMemoryValue M1, Monomial, tmp;
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
                HssMul(tmp, b, pk, Ix[j], Monomial, prf_key);
                copy(begin(tmp), end(tmp), begin(Monomial));
            }
        }
        HssAddMemory(y_b_res, pk, y_b_res, Monomial);
    }
}


// RMS-optimized recurrence: H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]
// Reduces HssMul calls from O(k*d^3) to O(k*d).
void HssEvaluateMPE(HssMemoryValue &y_b_res, int b, const vector<HssCiphertext> &Ix, const HssPublicKey &pk, const HssEvalKey &ekb, int &prf_key, int degree_f)
{
    HssMemoryValue tmp;

    int k = Ix.size();
    Vec<HssMemoryValue> dp_prev, dp_curr;
    dp_prev.SetLength(1+degree_f);
    dp_curr.SetLength(1+degree_f);

    dp_prev[0][0] = b;
    dp_prev[0][1] = ekb;

    for (int i = 1; i <= k; i++) {
        // dp_curr[0] = constant 1 (inherited from dp_prev[0])
        dp_curr[0][0] = 0;
        dp_curr[0][1] = 0;
        HssAddMemory(dp_curr[0], pk, dp_curr[0], dp_prev[0]);

        // H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]  for s = 1..degree_f
        for (int s = 1; s <= degree_f; s++) {
            dp_curr[s][0] = 0;
            dp_curr[s][1] = 0;
            // Start with H_{i-1}[s]
            HssAddMemory(dp_curr[s], pk, dp_curr[s], dp_prev[s]);
            // Add x_i * H_i[s-1]
            HssMul(tmp, b, pk, Ix[i - 1], dp_curr[s - 1], prf_key);
            HssAddMemory(dp_curr[s], pk, dp_curr[s], tmp);
        }
        dp_prev.swap(dp_curr);
    }

    y_b_res[0] = 0;
    y_b_res[1] = 0;
    for (int s = 1; s <= degree_f; s++) {
        HssAddMemory(y_b_res, pk, y_b_res, dp_prev[s]);
    }

}

void HssDec(ZZ &z, const HssMemoryValue &Mx0, const HssMemoryValue &Mx1)
{
    sub(z, Mx1[0], Mx0[0]);
}

}}} // namespace pvhss::group::hss
