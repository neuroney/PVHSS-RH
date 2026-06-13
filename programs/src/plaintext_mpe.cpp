#include "plaintext_mpe.h"
#include <NTL/ZZ.h>
#include <vector>

namespace pvhss { namespace programs {

NTL::ZZ PlaintextMpe(
    const std::vector<NTL::ZZ>& x,
    int degree_f,
    const NTL::ZZ& modulus)
{
    int m = static_cast<int>(x.size());

    // H[0] = 1, H[s] = 0 for s > 0
    std::vector<NTL::ZZ> H(degree_f + 1, NTL::ZZ(0));
    H[0] = 1;

    bool has_modulus = (modulus > 0);

    for (int i = 0; i < m; ++i)
    {
        // Forward order: H[s] += x_i * H[s-1] for s = 1 .. degree_f
        // H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]
        for (int s = 1; s <= degree_f; ++s)
        {
            NTL::ZZ prod;
            if (has_modulus)
            {
                NTL::MulMod(prod, x[i], H[s - 1], modulus);
                NTL::AddMod(H[s], H[s], prod, modulus);
            }
            else
            {
                mul(prod, x[i], H[s - 1]);
                add(H[s], H[s], prod);
            }
        }
    }

    // Output: sum_{s=1}^{d} H[s]
    NTL::ZZ result(0);
    for (int s = 1; s <= degree_f; ++s)
    {
        if (has_modulus)
        {
            NTL::AddMod(result, result, H[s], modulus);
        }
        else
        {
            add(result, result, H[s]);
        }
    }

    return result;
}

}} // namespace pvhss::programs
