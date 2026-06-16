#include "VHSSElg.h"

#include <stdexcept>
#include <algorithm>

using namespace NTL;
using namespace std;

namespace
{
struct VhssElgamalPreparedInput
{
    ZZ enc_value_base;
    ZZ enc_mask_inv_base;
    ZZ skenc_value_base;
    ZZ skenc_mask_inv_base;
};

// Fixed-base exponentiation table: precomputes base^(2^i) mod modulus for i=0..num_bits-1.
// Eliminates the per-exponentiation squaring cost in PowerMod when the same base is
// reused across many nodes (as in PIR selection tree building).
struct FixedBaseTable
{
    vector<ZZ> enc_val_pow2;
    vector<ZZ> enc_mask_pow2;
    vector<ZZ> skenc_val_pow2;
    vector<ZZ> skenc_mask_pow2;
    long num_bits;
};

void VhssElgamalZeroMemory(VhssElgamalMv &Mz)
{
    Mz[0] = 0;
    Mz[1] = 0;
    Mz[2] = 0;
    Mz[3] = 0;
}

void VhssElgamalOneMemory(VhssElgamalMv &Mz, int b, const VhssElgamalEk &ekb)
{
    Mz[0] = b;
    Mz[1] = ekb[0];
    Mz[2] = ekb[1];
    Mz[3] = ekb[2];
}

VhssElgamalPreparedInput VhssElgamalPrepareInput(const VhssElgamalCt &Ix,
                                                 const VhssElgamalPk &pk)
{
    VhssElgamalPreparedInput prepared;
    prepared.enc_value_base = Ix[0][1];
    InvMod(prepared.enc_mask_inv_base, Ix[0][0], pk.N2);
    prepared.skenc_value_base = Ix[1][1];
    InvMod(prepared.skenc_mask_inv_base, Ix[1][0], pk.N2);
    return prepared;
}

// Precompute base^(2^i) mod modulus for i = 0..num_bits-1.
vector<ZZ> PrecomputePow2Table(const ZZ &base, const ZZ &modulus, long num_bits)
{
    if (num_bits <= 0)
    {
        return {};
    }
    vector<ZZ> table(static_cast<size_t>(num_bits));
    table[0] = base % modulus;
    for (long i = 1; i < num_bits; ++i)
    {
        SqrMod(table[static_cast<size_t>(i)], table[static_cast<size_t>(i - 1)], modulus);
    }
    return table;
}

// Build all four fixed-base tables for one depth from a prepared input.
FixedBaseTable BuildFixedBaseTable(const VhssElgamalPreparedInput &prep,
                                   const ZZ &modulus, long num_bits)
{
    FixedBaseTable tbl;
    tbl.num_bits = num_bits;
    tbl.enc_val_pow2 = PrecomputePow2Table(prep.enc_value_base, modulus, num_bits);
    tbl.enc_mask_pow2 = PrecomputePow2Table(prep.enc_mask_inv_base, modulus, num_bits);
    tbl.skenc_val_pow2 = PrecomputePow2Table(prep.skenc_value_base, modulus, num_bits);
    tbl.skenc_mask_pow2 = PrecomputePow2Table(prep.skenc_mask_inv_base, modulus, num_bits);
    return tbl;
}

// Fast exponentiation using precomputed table: base^exp = ∏_{i: bit(exp,i)=1} table[i] mod modulus.
// Eliminates squaring cost — only multiplications remain.
// Precondition: NumBits(exponent) <= table.size() (table must cover the full exponent width).
inline void FastPowMod(ZZ &result, const vector<ZZ> &table,
                       const ZZ &exponent, const ZZ &modulus)
{
    const long nbits = NumBits(exponent);
    const long table_size = static_cast<long>(table.size());
    if (nbits > table_size)
    {
        // Safety fallback — should not happen with correct table sizing
        PowerMod(result, table[0], exponent, modulus);
        return;
    }
    result = 1;
    for (long i = 0; i < nbits; ++i)
    {
        if (bit(exponent, i))
        {
            MulMod(result, result, table[static_cast<size_t>(i)], modulus);
        }
    }
}

// Fixed-base variant of VhssElgamalMulPrepared that uses precomputed pow2 tables.
// Replaces 4 PowerMod calls (each with ~1024 squarings + ~512 multiplications)
// with 4 FastPowMod calls (each with ~512 multiplications, no squarings).
// The speedup per call is ~2-3× on the modular exponentiation component.
void VhssElgamalMulPreparedFast(VhssElgamalMv &Mz, const VhssElgamalPk &pk,
                                const FixedBaseTable &tbl,
                                const VhssElgamalMv &My, int &prf_key)
{
    ZZ temp1, temp2;

    FastPowMod(temp1, tbl.enc_val_pow2, My[0], pk.N2);
    FastPowMod(temp2, tbl.enc_mask_pow2, My[1], pk.N2);
    MulMod(Mz[0], temp1, temp2, pk.N2);
    VhssElgamalDdlog(Mz[0], pk, Mz[0]);
    PrfZZ(temp1, prf_key++, pk.N);
    AddMod(Mz[0], Mz[0], temp1, pk.N);

    FastPowMod(temp1, tbl.skenc_val_pow2, My[0], pk.N2);
    FastPowMod(temp2, tbl.skenc_mask_pow2, My[1], pk.N2);
    MulMod(Mz[1], temp1, temp2, pk.N2);
    VhssElgamalDdlog(Mz[1], pk, Mz[1]);
    PrfZZ(temp1, prf_key++, pk.N);
    AddMod(Mz[1], Mz[1], temp1, pk.N);

    FastPowMod(temp1, tbl.enc_val_pow2, My[2], pk.N2);
    FastPowMod(temp2, tbl.enc_mask_pow2, My[3], pk.N2);
    MulMod(Mz[2], temp1, temp2, pk.N2);
    VhssElgamalDdlog(Mz[2], pk, Mz[2]);
    PrfZZ(temp1, prf_key++, pk.N);
    AddMod(Mz[2], Mz[2], temp1, pk.N);

    FastPowMod(temp1, tbl.skenc_val_pow2, My[2], pk.N2);
    FastPowMod(temp2, tbl.skenc_mask_pow2, My[3], pk.N2);
    MulMod(Mz[3], temp1, temp2, pk.N2);
    VhssElgamalDdlog(Mz[3], pk, Mz[3]);
    PrfZZ(temp1, prf_key++, pk.N);
    AddMod(Mz[3], Mz[3], temp1, pk.N);
}
}

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

void VhssElgamalSubMemory(VhssElgamalMv &Mz, const VhssElgamalPk &pk, const VhssElgamalMv &Mx, const VhssElgamalMv &My)
{
    SubMod(Mz[0], Mx[0], My[0], pk.N);
    SubMod(Mz[1], Mx[1], My[1], pk.N);
    SubMod(Mz[2], Mx[2], My[2], pk.N);
    SubMod(Mz[3], Mx[3], My[3], pk.N);
}

void VhssElgamalScaleMemory(VhssElgamalMv &Mz, const VhssElgamalPk &pk, const VhssElgamalMv &Mx, const ZZ &scalar)
{
    ZZ c = scalar % pk.N;
    if (c < 0)
    {
        c += pk.N;
    }

    MulMod(Mz[0], Mx[0], c, pk.N);
    MulMod(Mz[1], Mx[1], c, pk.N);
    MulMod(Mz[2], Mx[2], c, pk.N);
    MulMod(Mz[3], Mx[3], c, pk.N);
}

// RMS-optimized recurrence: H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]
// Reduces VhssElgamalMul calls from O(k*d^3) to O(k*d).
void VhssElgamalEvaluateMPE(VhssElgamalMv &y_b_res, int b, const vector<VhssElgamalCt> &Ix, const VhssElgamalPk &pk, const VhssElgamalEk &ekb, int &prf_key, int degree_f)
{
    VhssElgamalMv tmp;
    
    int k = Ix.size(); 

    Vec<VhssElgamalMv> dp_prev, dp_curr;
    dp_prev.SetLength(1+degree_f);
    dp_curr.SetLength(1+degree_f);
    
    dp_prev[0][0] = b;
    dp_prev[0][1] = ekb[0];
    dp_prev[0][2] = ekb[1];
    dp_prev[0][3] = ekb[2]; 
    
    for (int i = 1; i <= k; i++) {
        // dp_curr[0] = constant 1 (inherited from dp_prev[0])
        dp_curr[0][0] = 0;
        dp_curr[0][1] = 0;
        dp_curr[0][2] = 0;
        dp_curr[0][3] = 0;
        VhssElgamalAddMemory(dp_curr[0], pk, dp_curr[0], dp_prev[0]);

        // H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]  for s = 1..degree_f
        for (int s = 1; s <= degree_f; s++) {
            dp_curr[s][0] = 0;
            dp_curr[s][1] = 0;
            dp_curr[s][2] = 0;
            dp_curr[s][3] = 0;
            // Start with H_{i-1}[s]
            VhssElgamalAddMemory(dp_curr[s], pk, dp_curr[s], dp_prev[s]);
            // Add x_i * H_i[s-1]
            VhssElgamalMul(tmp, b, pk, Ix[i - 1], dp_curr[s - 1], prf_key);
            VhssElgamalAddMemory(dp_curr[s], pk, dp_curr[s], tmp);
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

void VhssElgamalEvaluatePirSelection(VhssElgamalMv &y_b_res, int b,
                                     const vector<VhssElgamalCt> &IxBits,
                                     const vector<ZZ> &database,
                                     const VhssElgamalPk &pk,
                                     const VhssElgamalEk &ekb,
                                     int &prf_key)
{
    const size_t logn = IxBits.size();
    if (logn >= sizeof(size_t) * 8)
    {
        throw invalid_argument("PIR input is too large for size_t indexing");
    }

    const size_t expected_size = size_t{1} << logn;
    if (database.size() != expected_size)
    {
        throw invalid_argument("PIR database size must be exactly 2^logn");
    }

    // ── Phase 0: determine maximum exponent bit-width for table sizing ──
    // The MAC components (My[1], My[3]) start as ekb keys and stay mod pk.N.
    // ekb[2] = skLen+vkLen bits may exceed pk.N, so take the max.
    VhssElgamalMv root;
    VhssElgamalOneMemory(root, b, ekb);
    long max_exp_bits = NumBits(pk.N);
    max_exp_bits = max(max_exp_bits, NumBits(root[1]));
    max_exp_bits = max(max_exp_bits, NumBits(root[3]));

    // ── Phase 1: prepare inputs and precompute fixed-base pow2 tables ──
    // Each depth reuses the same 4 bases for all 2^depth nodes.
    // Precomputing base^(2^i) once per depth eliminates the squaring cost
    // from every subsequent PowerMod, saving ~2× on the exponentiation.
    vector<FixedBaseTable> depth_tables;
    depth_tables.reserve(logn);
    for (size_t d = 0; d < logn; ++d)
    {
        VhssElgamalPreparedInput prep = VhssElgamalPrepareInput(IxBits[d], pk);
        depth_tables.push_back(BuildFixedBaseTable(prep, pk.N2, max_exp_bits));
    }

    // ── Phase 2: build selection tree using fast fixed-base multiplication ──
    vector<VhssElgamalMv> level(1);
    level[0] = root;

    for (size_t depth = 0; depth < logn; ++depth)
    {
        vector<VhssElgamalMv> next(level.size() * 2);
        for (size_t node = 0; node < level.size(); ++node)
        {
            VhssElgamalMv right;
            VhssElgamalMulPreparedFast(right, pk, depth_tables[depth],
                                       level[node], prf_key);

            VhssElgamalSubMemory(next[2 * node], pk, level[node], right);
            next[2 * node + 1] = right;
        }
        level.swap(next);
    }

    // ── Phase 3: weighted sum over database (unchanged) ──
    VhssElgamalZeroMemory(y_b_res);
    for (size_t j = 0; j < database.size(); ++j)
    {
        VhssElgamalMv weighted;
        VhssElgamalScaleMemory(weighted, pk, level[j], database[j]);
        VhssElgamalAddMemory(y_b_res, pk, y_b_res, weighted);
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
