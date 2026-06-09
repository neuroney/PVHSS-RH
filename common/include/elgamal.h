#pragma once
#include "helper.h"

// Elgamal cryptosystem types (Paillier-Elgamal hybrid over Z_{N^2}).
struct ElgamalPublicKey {
    NTL::ZZ f;
    NTL::ZZ g;
    NTL::ZZ h;
    NTL::ZZ N;
    NTL::ZZ N2;
};

using ElgamalCiphertext = std::array<NTL::ZZ, 2>;
using ElgamalSecretKey = NTL::ZZ;

void ElgamalGen(ElgamalPublicKey &pk, ElgamalSecretKey &sk, int sk_len);
void ElgamalEnc(ElgamalCiphertext &ct, const ElgamalPublicKey &pk, const NTL::ZZ &x);
void ElgamalSkEnc(ElgamalCiphertext &ct, const ElgamalPublicKey &pk, const NTL::ZZ &x);
void ElgamalDec(NTL::ZZ &x, const ElgamalPublicKey &pk, const ElgamalSecretKey &sk,
                const ElgamalCiphertext &ct);