#pragma once

#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <NTL/ZZ_pX.h>
#include <NTL/matrix.h>
#include <NTL/vec_vec_ZZ.h>

// Encoded dataset: plaintext values, ciphertexts, PRF shares, and evaluation map.
struct EncodedData {
    NTL::vec_ZZ X_decimal;
    NTL::Vec<NTL::ZZ_pX> X;
    NTL::Vec<NTL::vec_ZZ_pX> C_X;
    NTL::Vec<NTL::vec_ZZ_pX> PRF;
    std::vector<std::vector<int>> F_TEST;
    NTL::vec_ZZ_pX C_1;
};

inline void GeneratePartitions(int &cnt, std::vector<std::vector<int>> &Res,
                               int sum, int k, std::vector<int> &lst, int minn) {
    if (k == 0) {
        if (sum == 0) {
            Res.push_back(lst);
            ++cnt;
        }
        return;
    }
    for (int i = minn; i < sum + 1; ++i) {
        lst.push_back(i);
        GeneratePartitions(cnt, Res, sum - i, k - 1, lst, i);
        lst.pop_back();
    }
}

inline void GenerateRandomFunc(std::vector<std::vector<int>> &F_TEST,
                               int msg_num, int degree_f) {
    std::vector<std::vector<int>> Combinations;
    std::vector<int> tmp;

    for (int i = 1; i <= degree_f; ++i) {
        Combinations.clear();
        tmp.clear();
        int len = 0;
        GeneratePartitions(len, Combinations, i, msg_num, tmp, 0);
        srand(static_cast<unsigned>(time(0)));
        tmp = Combinations[rand() % len];
        F_TEST.push_back(tmp);
    }
}

inline void GenerateData(EncodedData &data, PkeParams params,
                         NTL::ZZ_pXModulus &modulus, NTL::vec_ZZ_pX pk) {
    data.X.SetLength(params.num_data);
    NTL::vec_ZZ_pX C_x, prf;
    C_x.SetLength(4);
    prf.SetLength(2);
    data.X_decimal.SetLength(params.num_data);
    for (int i = 0; i < params.num_data; i++) {
        data.X_decimal[i] = NTL::RandomBits_ZZ(params.msg_bit);
        DecimalToBinary(data.X[i], data.X_decimal[i], params.msg_bit);
        PvhssEnc(C_x, params, modulus, pk, data.X[i]);
        data.C_X.append(C_x);
    }
    for (int i = 0; i < 10; i++) {
        RandomZZpx(prf[0], params.N, params.q_bit);
        RandomZZpx(prf[1], params.N, params.q_bit);
        data.PRF.append(prf);
    }
    GenerateRandomFunc(data.F_TEST, params.num_data, params.d);

    // C1: encryption of constant 1
    data.C_1.SetLength(4);
    NTL::ZZ M_1(1);
    NTL::ZZ_pX M1_px;
    DecimalToBinary(M1_px, M_1, 1);
    PvhssEnc(data.C_1, params, modulus, pk, M1_px);
}
