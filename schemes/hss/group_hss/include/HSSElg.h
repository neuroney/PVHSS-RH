#pragma once
#include "helper.h"
#include "elgamal.h"

namespace pvhss { namespace group { namespace hss {

using HssPublicKey = ElgamalPublicKey;
using HssEvalKey = NTL::ZZ;
using HssCiphertext = std::array<ElgamalCiphertext, 2>;
using HssMemoryValue = std::array<NTL::ZZ, 2>;

void ElgamalGen(ElgamalPublicKey &pk, ElgamalSecretKey &sk, int sk_len);
void ElgamalEnc(ElgamalCiphertext &ct, const ElgamalPublicKey &pk, const NTL::ZZ &x);
void ElgamalSkEnc(ElgamalCiphertext &ct, const ElgamalPublicKey &pk, const NTL::ZZ &x);
void ElgamalDec(NTL::ZZ &x, const ElgamalPublicKey &pk, const ElgamalSecretKey &sk,
                const ElgamalCiphertext &ct);

void HssGen(HssPublicKey &pk, HssEvalKey &ek0, HssEvalKey &ek1, int sk_len);
void HssInput(HssCiphertext &I, const HssPublicKey &pk, const NTL::ZZ &x);
void HssConvertInput(HssMemoryValue &Mx, int idx, const HssPublicKey &pk,
                     const HssEvalKey &ek, const HssCiphertext &Ix, int &prf_key);
void HssMul(HssMemoryValue &Mz, int idx, const HssPublicKey &pk,
            const HssCiphertext &Ix, const HssMemoryValue &My, int &prf_key);
void HssDdlog(NTL::ZZ &z, const HssPublicKey &pk, const NTL::ZZ &g);
void HssAddMemory(HssMemoryValue &Mz, const HssPublicKey &pk,
                  const HssMemoryValue &Mx, const HssMemoryValue &My);
void HssEvaluate(HssMemoryValue &y_b_res, int b,
                 const std::vector<HssCiphertext> &Ix, const HssPublicKey &pk,
                 const HssEvalKey &ekb, int &prf_key,
                 std::vector<std::vector<int>> F_TEST);
void HssEvaluateMPE(HssMemoryValue &y_b_res, int b,
                       const std::vector<HssCiphertext> &Ix,
                       const HssPublicKey &pk, const HssEvalKey &ekb,
                       int &prf_key, int degree_f);
void HssDec(NTL::ZZ &z, const HssMemoryValue &Mx, const HssMemoryValue &My);

}}} // namespace pvhss::group::hss
