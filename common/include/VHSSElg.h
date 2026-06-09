#pragma once
#include "elgamal.h"

using VhssElgamalPk = ElgamalPublicKey;
using VhssElgamalEk = std::array<NTL::ZZ, 3>;
using VhssElgamalCt = std::array<ElgamalCiphertext, 2>;
using VhssElgamalMv = std::array<NTL::ZZ, 4>;
using VhssElgamalVk = NTL::ZZ;

void VhssElgamalGen(VhssElgamalPk &pk, VhssElgamalVk &vk, VhssElgamalEk &ek0,
                    VhssElgamalEk &ek1, int sk_len, int vk_len);
void VhssElgamalGen(VhssElgamalPk &pk, VhssElgamalEk &ek0, VhssElgamalEk &ek1,
                    int sk_len, int vk_len);
void VhssElgamalInput(VhssElgamalCt &I, const VhssElgamalPk &pk, const NTL::ZZ &x);
void VhssElgamalConvertInput(VhssElgamalMv &Mx, int idx, const VhssElgamalPk &pk,
                             const VhssElgamalEk &ek, const VhssElgamalCt &Ix,
                             int &prf_key);
void VhssElgamalMul(VhssElgamalMv &Mz, int idx, const VhssElgamalPk &pk,
                    const VhssElgamalCt &Ix, const VhssElgamalMv &My, int &prf_key);
void VhssElgamalDdlog(NTL::ZZ &z, const VhssElgamalPk &pk, const NTL::ZZ &g);
void VhssElgamalAddMemory(VhssElgamalMv &Mz, const VhssElgamalPk &pk,
                          const VhssElgamalMv &Mx, const VhssElgamalMv &My);
void VhssElgamalEvaluate(VhssElgamalMv &y_b_res, int b,
                         const std::vector<VhssElgamalCt> &Ix,
                         const VhssElgamalPk &pk, const VhssElgamalEk &ekb,
                         int &prf_key, std::vector<std::vector<int>> F_TEST);
void VhssElgamalEvaluatePD(VhssElgamalMv &y_b_res, int b,
                           const std::vector<VhssElgamalCt> &Ix,
                           const VhssElgamalPk &pk, const VhssElgamalEk &ekb,
                           int &prf_key, int degree_f);
void VhssElgamalEvaluatePD2(VhssElgamalMv &y_b_res, int b,
                            const std::vector<VhssElgamalCt> &Ix,
                            const VhssElgamalPk &pk, const VhssElgamalEk &ekb,
                            int &prf_key, int degree_f);

bool VhssElgamalVerify(const VhssElgamalMv &y_0_res, const VhssElgamalMv &y_1_res,
                       const VhssElgamalVk &vk);
