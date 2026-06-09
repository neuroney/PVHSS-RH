#include "VHSSElg.h"

using namespace NTL;
using namespace std;

void VhssElgamalGen(VhssElgamalPk &pk, VhssElgamalEk &ek0, VhssElgamalEk &ek1, int skLen, int vkLen)
{
    ElgamalSecretKey s;
    ElgamalGen(pk, s, skLen);

    ZZ VK;
    NTL::RandomBits(VK, vkLen);

    NTL::RandomBits(ek0[0], skLen);
    add(ek1[0], ek0[0], s);

    NTL::RandomBits(ek0[1], vkLen);
    add(ek1[1], ek0[1], VK);

    NTL::RandomBits(ek0[2], vkLen + skLen);

    add(ek1[2], ek0[2], (s * VK) % pk.N);
}

void VhssElgamalGen(VhssElgamalPk &pk, VhssElgamalVk &vk, VhssElgamalEk &ek0, VhssElgamalEk &ek1, int skLen, int vkLen)
{
    ElgamalSecretKey s;
    ElgamalGen(pk, s, skLen);

    NTL::RandomBits(vk, vkLen);

    NTL::RandomBits(ek0[0], skLen);
    add(ek1[0], ek0[0], s);

    NTL::RandomBits(ek0[1], vkLen);
    add(ek1[1], ek0[1], vk);

    NTL::RandomBits(ek0[2], vkLen + skLen);

    add(ek1[2], ek0[2], (s * vk) % pk.N);
}

void VhssElgamalInput(VhssElgamalCt &I, const VhssElgamalPk &pk, const ZZ &x)
{
    ElgamalEnc(I[0], pk, x);
    ElgamalSkEnc(I[1], pk, x);
}

void VhssElgamalConvertInput(VhssElgamalMv &Mx, int idx, const VhssElgamalPk &pk, const VhssElgamalEk &ek, const VhssElgamalCt &Ix, int &prf_key)
{
    VhssElgamalMv M1;
    M1[0] = idx;
    M1[1] = ek[0];
    M1[2] = ek[1];
    M1[3] = ek[2];
    VhssElgamalMul(Mx, idx, pk, Ix, M1, prf_key);
}

void VhssElgamalMul(VhssElgamalMv &Mz, int idx, const VhssElgamalPk &pk, const VhssElgamalCt &Ix, const VhssElgamalMv &My, int &prf_key)
{
    
    ZZ temp1, temp2;
    PowerMod(temp1, Ix[0][1], My[0], pk.N2);
    PowerMod(temp2, Ix[0][0], -My[1], pk.N2);
    MulMod(Mz[0], temp1, temp2, pk.N2);
    VhssElgamalDdlog(Mz[0], pk, Mz[0]);
    PrfZZ(temp1, prf_key++, pk.N);
    AddMod(Mz[0], Mz[0], temp1, pk.N);

    PowerMod(temp1, Ix[1][1], My[0], pk.N2);
    PowerMod(temp2, Ix[1][0], -My[1], pk.N2);
    MulMod(Mz[1], temp1, temp2, pk.N2);
    VhssElgamalDdlog(Mz[1], pk, Mz[1]);
    PrfZZ(temp1, prf_key++, pk.N);
    AddMod(Mz[1], Mz[1], temp1, pk.N);

    PowerMod(temp1, Ix[0][1], My[2], pk.N2);
    PowerMod(temp2, Ix[0][0], -My[3], pk.N2);
    MulMod(Mz[2], temp1, temp2, pk.N2);
    VhssElgamalDdlog(Mz[2], pk, Mz[2]);
    PrfZZ(temp1, prf_key++, pk.N);
    AddMod(Mz[2], Mz[2], temp1, pk.N);

    PowerMod(temp1, Ix[1][1], My[2], pk.N2);
    PowerMod(temp2, Ix[1][0], -My[3], pk.N2);
    MulMod(Mz[3], temp1, temp2, pk.N2);
    VhssElgamalDdlog(Mz[3], pk, Mz[3]);
    PrfZZ(temp1, prf_key++, pk.N);
    AddMod(Mz[3], Mz[3], temp1, pk.N);
}

void VhssElgamalDdlog(ZZ &z, const VhssElgamalPk &pk, const ZZ &g)
{
    ZZ h1, h, temp1;
    DivRem(h1, h, g, pk.N); // h = g % N; h1 = g / N
    InvMod(temp1, h, pk.N);
    MulMod(z, h1, temp1, pk.N);
}

void VhssElgamalAddMemory(VhssElgamalMv &Mz, const VhssElgamalPk &pk, const VhssElgamalMv &Mx, const VhssElgamalMv &My)
{
    AddMod(Mz[0], Mx[0], My[0], pk.N);
    AddMod(Mz[1], Mx[1], My[1], pk.N);
    AddMod(Mz[2], Mx[2], My[2], pk.N);
    AddMod(Mz[3], Mx[3], My[3], pk.N);
}

void VhssElgamalEvaluate(VhssElgamalMv &y_b_res, int b, const vector<VhssElgamalCt> &Ix, const VhssElgamalPk &pk, const VhssElgamalEk &ekb, int &prf_key, vector<vector<int>> F_TEST)
{
    VhssElgamalMv M1, Monomial, tmp;
    M1[0] = b;
    M1[1] = ekb[0];
    M1[2] = ekb[1];
    M1[3] = ekb[2];

    y_b_res[0] = 0;
    y_b_res[1] = 0;
    y_b_res[2] = 0;
    y_b_res[3] = 0;

    int i, j, k;
    for (i = 0; i < F_TEST.size(); ++i)
    {
        copy(begin(M1), end(M1), begin(Monomial));
        for (j = 0; j < Ix.size(); ++j)
        {
            for (k = 0; k < F_TEST[i][j]; ++k)
            {
                VhssElgamalMul(tmp, b, pk, Ix[j], Monomial, prf_key);
                copy(begin(tmp), end(tmp), begin(Monomial));
            }
        }
        VhssElgamalAddMemory(y_b_res, pk, y_b_res, Monomial);
    }
}

void VhssElgamalEvaluatePD(VhssElgamalMv &y_b_res, int b, const vector<VhssElgamalCt> &Ix, const VhssElgamalPk &pk, const VhssElgamalEk &ekb, int &prf_key, int degree_f)
{

    VhssElgamalMv tmp1, tmp2;

    int k = Ix.size(); 

    Mat<VhssElgamalMv> dp;
    dp.SetDims(k + 1, degree_f + 1);
    dp[0][0][0] = b;
    dp[0][0][1] = ekb[0];
    dp[0][0][2] = ekb[1];
    dp[0][0][3] = ekb[2]; 
    
    // 动态规划填表
    for (int i = 1; i <= k; i++) { // 依次加入 x1, x2, ..., x5
        for (int s = 0; s <= degree_f; s++) { // 目标和从 0 到 d
            dp[i][s][0] = 0;
            dp[i][s][1] = 0;
            dp[i][s][2] = 0;
            dp[i][s][3] = 0;
            for (int j = 0; j <= s; j++) { 
                copy(begin(dp[i - 1][s - j]), end(dp[i - 1][s - j]), begin(tmp1));
                for (int h=0; h < j;++h) {
                    VhssElgamalMul(tmp2, b, pk, Ix[i - 1], tmp1, prf_key);
                    copy(begin(tmp2), end(tmp2), begin(tmp1));
                }
                VhssElgamalAddMemory(dp[i][s], pk, dp[i][s], tmp1);
            }
        }
    }

    y_b_res[0] = 0;
    y_b_res[1] = 0;
    y_b_res[2] = 0;
    y_b_res[3] = 0;
    for (int s = 1; s <= degree_f; s++) {
        VhssElgamalAddMemory(y_b_res, pk, y_b_res, dp[k][s]); 
    }

}



void VhssElgamalEvaluatePD2(VhssElgamalMv &y_b_res, int b, const vector<VhssElgamalCt> &Ix, const VhssElgamalPk &pk, const VhssElgamalEk &ekb, int &prf_key, int degree_f)
{
    VhssElgamalMv tmp1, tmp2;
    
    int k = Ix.size(); 

    Vec<VhssElgamalMv> dp_prev, dp_curr;
    dp_prev.SetLength(1+degree_f);
    dp_curr.SetLength(1+degree_f);
    
    dp_prev[0][0] = b;
    dp_prev[0][1] = ekb[0];
    dp_prev[0][2] = ekb[1];
    dp_prev[0][3] = ekb[2]; 
    
    // 动态规划填表
    for (int i = 1; i <= k; i++) { // 依次加入 x1, x2, ..., x5
        for (int s = 0; s <= degree_f; s++) { // 目标和从 0 到 d
            dp_curr[s][0] = 0;
            dp_curr[s][1] = 0;
            dp_curr[s][2] = 0;
            dp_curr[s][3] = 0;
            VhssElgamalAddMemory(dp_curr[s], pk, dp_curr[s], dp_prev[s]);
            for (int j = 1; j <= s; j++) { 

                copy(begin(dp_prev[s - j]), end(dp_prev[s - j]), begin(tmp1));
                for (int h=0; h < j;++h) {
                    VhssElgamalMul(tmp2, b, pk, Ix[i - 1], tmp1, prf_key);
                    copy(begin(tmp2), end(tmp2), begin(tmp1));
                }
                VhssElgamalAddMemory(dp_curr[s], pk, dp_curr[s], tmp1);
            }
        }
        dp_prev.swap(dp_curr);
    }

    y_b_res[0] = 0;
    y_b_res[1] = 0;
    y_b_res[2] = 0;
    y_b_res[3] = 0;
    for (int s = 1; s <= degree_f; s++) {
        VhssElgamalAddMemory(y_b_res, pk, y_b_res, dp_prev[s]); 
    }

}

bool VhssElgamalVerify(const VhssElgamalMv &y_0_res, const VhssElgamalMv &y_1_res, const VhssElgamalVk &vk)
{
    // Verification logic goes here
    if (y_1_res[2] - y_0_res[2] == ((y_1_res[0] - y_0_res[0]) * vk)) {
        return true; // If both results are zero, verification fails
    }
    return false;
}