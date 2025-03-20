#include <limits.h>
#include "helper.h"

int cp_bgn_gen(bgn_t pub, bgn_t prv);
int cp_bgn_enc1(g1_t out[2], const dig_t in, const bgn_t pub);
int cp_bgn_enc3(g1_t out[2], bn_t r, bn_t in, const bgn_t pub);
int cp_bgn_enc2(g2_t out[2], const dig_t in, const bgn_t pub);
int cp_bgn_dec(dig_t *out, const gt_t in[4], const bgn_t prv);
int cp_bgn_add(gt_t e[4], const gt_t c[4], const gt_t d[4]);
int cp_bgn_mul(gt_t e[4], const g1_t c[2], const g2_t d[2]);
int cp_bgn_dec2(dig_t *out, const g2_t in[2], const bgn_t prv);
int cp_bgn_dec1(dig_t &out, const g1_t in[2], const bgn_t prv);
