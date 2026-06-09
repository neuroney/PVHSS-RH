/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2015 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
 */

 #include "relic_cp_decped.h"

 #include <unordered_map>
 #include <cmath>
 
 /*============================================================================*/
/* Helper: serialize a G1 element for hash-map lookup (BSGS baby-step table)  */
/*============================================================================*/

static std::string g1_bsgs_key(const g1_t point) {
    if (g1_is_infty(point) == 1) {
        return "INF";
    }
    g1_t normalized;
    g1_null(normalized);
    g1_new(normalized);
    g1_norm(normalized, point);
    const int len = g1_size_bin(normalized, 1);
    std::vector<uint8_t> bytes(static_cast<size_t>(len));
    g1_write_bin(bytes.data(), len, normalized, 1);
    g1_free(normalized);
    return std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

static std::string g2_bsgs_key(const g2_t point) {
    if (g2_is_infty(point) == 1) {
        return "INF";
    }
    g2_t normalized;
    g2_null(normalized);
    g2_new(normalized);
    g2_norm(normalized, point);
    const int len = g2_size_bin(normalized, 1);
    std::vector<uint8_t> bytes(static_cast<size_t>(len));
    g2_write_bin(bytes.data(), len, normalized, 1);
    g2_free(normalized);
    return std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

static std::string gt_bsgs_key(gt_t point) {
    if (gt_is_unity(point) == 1) {
        return "ONE";
    }
    const int len = gt_size_bin(point, 1);
    std::vector<uint8_t> bytes(static_cast<size_t>(len));
    gt_write_bin(bytes.data(), len, point, 1);
    return std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}
 int cp_decped_gen(bgn_t pub, bgn_t prv) {
     bn_t n;
     int result = RLC_OK;
 
     bn_null(n);
 
     RLC_TRY {
         bn_new(n);
 
         pc_get_ord(n);
 
         bn_rand_mod(prv->x, n);
         bn_rand_mod(prv->y, n);
         bn_rand_mod(prv->z, n);
 
         g1_mul_gen(pub->gx, prv->x);
         g1_mul_gen(pub->gy, prv->y);
         g1_mul_gen(pub->gz, prv->z);
 
         g2_mul_gen(pub->hx, prv->x);
         g2_mul_gen(pub->hy, prv->y);
         g2_mul_gen(pub->hz, prv->z);
     }
     RLC_CATCH_ANY {
         result = RLC_ERR;
     }
     RLC_FINALLY {
         bn_free(n);
     }
 
     return result;
 }
 
 int cp_decped_enc3(g1_t out[2], bn_t r, bn_t in, const bgn_t pub) {
    bn_t n;
    g1_t t;
    int result = RLC_OK;

    bn_null(n);
    g1_null(t);

    RLC_TRY {
        bn_new(n);
        g1_new(t);

        pc_get_ord(n);

        /* Compute c0 = (ym + r)G. */
        g1_mul(out[0], pub->gy, in);

        g1_mul_gen(t, r);
        g1_add(out[0], out[0], t);
        g1_norm(out[0], out[0]);

        /* Compute c1 = (zm + xr)G. */
        g1_mul(out[1], pub->gz, in);
        g1_mul(t, pub->gx, r);
        g1_add(out[1], out[1], t);
        g1_norm(out[1], out[1]);
    }
    RLC_CATCH_ANY {
        result = RLC_ERR;
    }
    RLC_FINALLY {
        bn_free(n);
        g1_free(t);
    }

    return result;
}

 int cp_decped_enc1(g1_t out[2], const dig_t in, const bgn_t pub) {
     bn_t r, n;
     g1_t t;
     int result = RLC_OK;
 
     bn_null(n);
     bn_null(r);
     g1_null(t);
 
     RLC_TRY {
         bn_new(n);
         bn_new(r);
         g1_new(t);
 
         pc_get_ord(n);
         bn_rand_mod(r, n);
 
         /* Compute c0 = (ym + r)G. */
         g1_mul_dig(out[0], pub->gy, in);
 
         g1_mul_gen(t, r);
         g1_add(out[0], out[0], t);
         g1_norm(out[0], out[0]);
 
         /* Compute c1 = (zm + xr)G. */
         g1_mul_dig(out[1], pub->gz, in);
         g1_mul(t, pub->gx, r);
         g1_add(out[1], out[1], t);
         g1_norm(out[1], out[1]);
     }
     RLC_CATCH_ANY {
         result = RLC_ERR;
     }
     RLC_FINALLY {
         bn_free(n);
         bn_free(r);
         g1_free(t);
     }
 
     return result;
 }
 
 int cp_decped_dec1(dig_t& out, const g1_t in[2], const bgn_t prv) {
     bn_t n, r;
     g1_t s, t;
     int result = RLC_OK;

     bn_null(n);
     bn_null(r);
     g1_null(s);
     g1_null(t);

     RLC_TRY {
         bn_new(n);
         bn_new(r);
         g1_new(s);
         g1_new(t);

         pc_get_ord(n);
         /* Compute T = x(ym + r)G - (zm + xr)G = m(xy - z)G. */
         g1_mul(t, in[0], prv->x);
         g1_sub(t, t, in[1]);
         g1_norm(t, t);
         /* Compute base = (xy - z)G. */
         bn_mul(r, prv->x, prv->y);
         bn_sub(r, r, prv->z);
         bn_mod(r, r, n);
         g1_mul_gen(s, r);

         if (g1_is_infty(t) == 1) {
             out = 0;
             result = RLC_OK;
         } else {
             /* ---- Baby-Step Giant-Step (BSGS) ---- */
             const uint64_t M = PVHSS_M_MAX;
             const uint64_t baby = static_cast<uint64_t>(
                 std::ceil(std::sqrt(static_cast<long double>(M))));

             /* Build baby-step table: {0, s, 2s, ..., (baby-1)*s} */
             std::unordered_map<std::string, uint64_t> table;
             table.reserve(static_cast<size_t>(baby * 2));

             g1_t cur;
             g1_null(cur);
             g1_new(cur);
             g1_set_infty(cur);

             for (uint64_t i = 0; i < baby && i < M; ++i) {
                 table[g1_bsgs_key(cur)] = i;
                 g1_add(cur, cur, s);
                 g1_norm(cur, cur);
             }
             g1_free(cur);

             /* Giant step = baby * s (negative direction) */
             g1_t giant;
             g1_null(giant);
             g1_new(giant);

             bn_t step_bn;
             bn_null(step_bn);
             bn_new(step_bn);

             char buf[32];
             std::snprintf(buf, sizeof(buf), "%llu",
                           static_cast<unsigned long long>(baby));
             bn_read_str(step_bn, buf, static_cast<int>(std::strlen(buf)), 10);
             g1_mul(giant, s, step_bn);
             g1_norm(giant, giant);
             bn_free(step_bn);

             /* Search: T - j*giant ∈ baby-steps ? */
             g1_t cur2;
             g1_null(cur2);
             g1_new(cur2);
             g1_copy(cur2, t);

             const uint64_t giants = (M + baby - 1) / baby;
             result = RLC_ERR;
             for (uint64_t j = 0; j <= giants; ++j) {
                 std::unordered_map<std::string, uint64_t>::const_iterator it =
                     table.find(g1_bsgs_key(cur2));
                 if (it != table.end()) {
                     const uint64_t cand = j * baby + it->second;
                     if (cand < M) {
                         out = static_cast<dig_t>(cand);
                         result = RLC_OK;
                         break;
                     }
                 }
                 g1_sub(cur2, cur2, giant);
                 g1_norm(cur2, cur2);
             }
             g1_free(cur2);
             g1_free(giant);
         }
     } RLC_CATCH_ANY {
         result = RLC_ERR;
     }
     RLC_FINALLY {
         bn_free(n);
         bn_free(r);
         g1_free(s);
         g1_free(t);
     }

     return result;
 }
 
 int cp_decped_enc2(g2_t out[2], const dig_t in, const bgn_t pub) {
     bn_t r, n;
     g2_t t;
     int result = RLC_OK;
 
     bn_null(n);
     bn_null(r);
     g2_null(t);
 
     RLC_TRY {
         bn_new(n);
         bn_new(r);
         g2_new(t);
 
         pc_get_ord(n);
         bn_rand_mod(r, n);
 
         /* Compute c0 = (ym + r)G. */
         g2_mul_dig(out[0], pub->hy, in);
         g2_mul_gen(t, r);
         g2_add(out[0], out[0], t);
         g2_norm(out[0], out[0]);
 
         /* Compute c1 = (zm + xr)G. */
         g2_mul_dig(out[1], pub->hz, in);
         g2_mul(t, pub->hx, r);
         g2_add(out[1], out[1], t);
         g2_norm(out[1], out[1]);
     }
     RLC_CATCH_ANY {
         result = RLC_ERR;
     }
     RLC_FINALLY {
         bn_free(n);
         bn_free(r);
         g2_free(t);
     }
 
     return result;
 }
 
 int cp_decped_dec2(dig_t *out, const g2_t in[2], const bgn_t prv) {
     bn_t n, r;
     g2_t s, t;
     int result = RLC_OK;

     bn_null(n);
     bn_null(r);
     g2_null(s);
     g2_null(t);

     RLC_TRY {
         bn_new(n);
         bn_new(r);
         g2_new(s);
         g2_new(t);

         pc_get_ord(n);
         /* Compute T = x(ym + r)G - (zm + xr)G = m(xy - z)G. */
         g2_mul(t, in[0], prv->x);
         g2_sub(t, t, in[1]);
         g2_norm(t, t);
         /* Compute base = (xy - z)G. */
         bn_mul(r, prv->x, prv->y);
         bn_sub(r, r, prv->z);
         bn_mod(r, r, n);
         g2_mul_gen(s, r);

         if (g2_is_infty(t) == 1) {
             *out = 0;
             result = RLC_OK;
         } else {
             /* ---- Baby-Step Giant-Step (BSGS) ---- */
             const uint64_t M = PVHSS_M_MAX;
             const uint64_t baby = static_cast<uint64_t>(
                 std::ceil(std::sqrt(static_cast<long double>(M))));

             /* Build baby-step table */
             std::unordered_map<std::string, uint64_t> table;
             table.reserve(static_cast<size_t>(baby * 2));

             g2_t cur;
             g2_null(cur);
             g2_new(cur);
             g2_set_infty(cur);

             for (uint64_t i = 0; i < baby && i < M; ++i) {
                 table[g2_bsgs_key(cur)] = i;
                 g2_add(cur, cur, s);
                 g2_norm(cur, cur);
             }
             g2_free(cur);

             /* Giant step = baby * s */
             g2_t giant;
             g2_null(giant);
             g2_new(giant);

             bn_t step_bn;
             bn_null(step_bn);
             bn_new(step_bn);

             char buf[32];
             std::snprintf(buf, sizeof(buf), "%llu",
                           static_cast<unsigned long long>(baby));
             bn_read_str(step_bn, buf, static_cast<int>(std::strlen(buf)), 10);
             g2_mul(giant, s, step_bn);
             g2_norm(giant, giant);
             bn_free(step_bn);

             /* Search: T - j*giant ∈ baby-steps ? */
             g2_t cur2;
             g2_null(cur2);
             g2_new(cur2);
             g2_copy(cur2, t);

             const uint64_t giants = (M + baby - 1) / baby;
             result = RLC_ERR;
             for (uint64_t j = 0; j <= giants; ++j) {
                 std::unordered_map<std::string, uint64_t>::const_iterator it =
                     table.find(g2_bsgs_key(cur2));
                 if (it != table.end()) {
                     const uint64_t cand = j * baby + it->second;
                     if (cand < M) {
                         *out = static_cast<dig_t>(cand);
                         result = RLC_OK;
                         break;
                     }
                 }
                 g2_sub(cur2, cur2, giant);
                 g2_norm(cur2, cur2);
             }
             g2_free(cur2);
             g2_free(giant);
         }
     } RLC_CATCH_ANY {
         result = RLC_ERR;
     }
     RLC_FINALLY {
         bn_free(n);
         bn_free(r);
         g2_free(s);
         g2_free(t);
     }

     return result;
 }
 
 int cp_decped_add(gt_t e[4], const gt_t c[4], const gt_t d[4]) {
     for (int i = 0; i < 4; i++) {
         gt_mul(e[i], c[i], d[i]);
     }
     return RLC_OK;
 }
 
 int cp_decped_mul(gt_t e[4], const g1_t c[2], const g2_t d[2]) {
     for (int i = 0; i < 2; i++) {
         for (int j = 0; j < 2; j++) {
             pc_map(e[2*i + j], c[i], d[j]);
         }
     }
     return RLC_OK;
 }
 
 int cp_decped_dec(dig_t *out, const gt_t in[4], const bgn_t prv) {
     int i, result = RLC_OK;
     g1_t g;
     g2_t h;
     gt_t t[4];
     bn_t n, r, s;

     bn_null(n);
     bn_null(r);
     bn_null(s);
     g1_null(g);
     g2_null(h);

     RLC_TRY {
         bn_new(n);
         bn_new(r);
         bn_new(s);
         g1_new(g);
         g2_new(h);
         for (i = 0; i < 4; i++) {
             gt_null(t[i]);
             gt_new(t[i]);
         }

         gt_exp(t[0], in[0], prv->x);
         gt_exp(t[0], t[0], prv->x);

         gt_mul(t[1], in[1], in[2]);
         gt_exp(t[1], t[1], prv->x);
         gt_inv(t[1], t[1]);

         gt_mul(t[3], in[3], t[1]);
         gt_mul(t[3], t[3], t[0]);

         pc_get_ord(n);
         g1_get_gen(g);
         g2_get_gen(h);

         bn_mul(r, prv->x, prv->y);
         bn_sqr(r, r);

         bn_mul(s, prv->x, prv->y);
         bn_mul(s, s, prv->z);
         bn_sub(r, r, s);
         bn_sub(r, r, s);

         bn_sqr(s, prv->z);
         bn_add(r, r, s);
         bn_mod(r, r, n);
         pc_map(t[1], g, h);
         gt_exp(t[1], t[1], r);

         if (gt_is_unity(t[3]) == 1) {
             *out = 0;
             result = RLC_OK;
         } else {
             /* ---- Baby-Step Giant-Step (BSGS) ---- */
             const uint64_t M = PVHSS_M_MAX;
             const uint64_t baby = static_cast<uint64_t>(
                 std::ceil(std::sqrt(static_cast<long double>(M))));

             /* Build baby-step table */
             std::unordered_map<std::string, uint64_t> table;
             table.reserve(static_cast<size_t>(baby * 2));

             gt_t cur;
             gt_null(cur);
             gt_new(cur);
             gt_set_unity(cur);

             for (uint64_t k = 0; k < baby && k < M; ++k) {
                 table[gt_bsgs_key(cur)] = k;
                 gt_mul(cur, cur, t[1]);
             }

             /* Giant step = t[1]^baby, inverted for subtraction walk */
             gt_t giant_inv;
             gt_null(giant_inv);
             gt_new(giant_inv);

             bn_t step_bn;
             bn_null(step_bn);
             bn_new(step_bn);

             char buf[32];
             std::snprintf(buf, sizeof(buf), "%llu",
                           static_cast<unsigned long long>(baby));
             bn_read_str(step_bn, buf, static_cast<int>(std::strlen(buf)), 10);
             gt_exp(giant_inv, t[1], step_bn);
             gt_inv(giant_inv, giant_inv);
             bn_free(step_bn);

             /* Search: target * (step^{-baby})^j ∈ baby-steps ? */
             gt_copy(cur, t[3]);

             const uint64_t giants = (M + baby - 1) / baby;
             result = RLC_ERR;
             for (uint64_t j = 0; j <= giants; ++j) {
                 std::unordered_map<std::string, uint64_t>::const_iterator it =
                     table.find(gt_bsgs_key(cur));
                 if (it != table.end()) {
                     const uint64_t cand = j * baby + it->second;
                     if (cand < M) {
                         *out = static_cast<dig_t>(cand);
                         result = RLC_OK;
                         break;
                     }
                 }
                 gt_mul(cur, cur, giant_inv);
             }
             gt_free(cur);
             gt_free(giant_inv);
         }
     } RLC_CATCH_ANY {
         result = RLC_ERR;
     } RLC_FINALLY {
         bn_free(n);
         bn_free(r);
         bn_free(s);
         g1_free(g);
         g2_free(h);
         for (i = 0; i < 4; i++) {
             gt_free(t[i]);
         }
     }

     return result;
 } /*============================================================================*/
 /* End of file                                                                */
 /*============================================================================*/