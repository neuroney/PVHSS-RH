#pragma once

#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <NTL/ZZ_pX.h>

// Encoded dataset used by the CZ protocol benchmark.
struct EncodedData {
    NTL::vec_ZZ X_decimal;
    NTL::Vec<NTL::ZZ_pX> X;
    NTL::Vec<NTL::vec_ZZ_pX> C_X;
};

inline void ClearEncodedData(EncodedData &data) {
    data.X_decimal.SetLength(0);
    data.X.SetLength(0);
    data.C_X.SetLength(0);
}

inline void GenerateInputData(EncodedData &data, const PkeParams &params,
                              const NTL::ZZ_pXModulus &modulus,
                              const NTL::vec_ZZ_pX &pk,
                              const NTL::vec_ZZ &x_decimal) {
    ClearEncodedData(data);
    data.X.SetLength(params.num_data);
    data.X_decimal.SetLength(params.num_data);
    NTL::vec_ZZ_pX C_x;
    C_x.SetLength(4);
    for (int i = 0; i < params.num_data; i++) {
        data.X_decimal[i] = x_decimal[i];
        DecimalToBinary(data.X[i], data.X_decimal[i], params.msg_bit);
        PvhssEnc(C_x, params, modulus, pk, data.X[i]);
        data.C_X.append(C_x);
    }
}
