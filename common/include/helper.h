#pragma once

#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cstdlib>
#include <algorithm>
#include <cstdint>
#include <ctime>
#include <functional>
#include <NTL/vector.h>
#include <NTL/matrix.h>
#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <NTL/ZZ_pX.h>
#include <NTL/BasicThreadPool.h>
#include <gmp.h>
extern "C" {
#include <relic/relic.h>
}

// NTL and std are used pervasively in this codebase;
// keeping these using-directives in the central helper header is pragmatic.
using namespace std;
using namespace NTL;

const int PVHSS_NUM_THREADS = 8;
#define PVHSS_PI 3.141592654
#define PVHSS_M_MAX (UINT64_C(1) << 32)

// === Integer conversion utilities ===

void ZZtoBn(bn_t out, const NTL::ZZ &in);

void BnToZZ(NTL::ZZ &out, const bn_t in);

// === Timing infrastructure ===

struct TimingResult {
    int samples;
    int iterations_per_sample;
    bool adaptive;
    double mean_ms;
    double median_ms;
    double min_ms;
    double rsd;
};

double SteadyTimeSeconds();

TimingResult MeasureTimeMs(const std::function<void()> &fn, int samples,
                           int iterations_per_sample = 1,
                           bool adaptive = false,
                           double min_sample_ms = 25.0,
                           int max_adaptive_iters = 10000000);

TimingResult MeasureTimeMsAdaptive(const std::function<void()> &fn, int samples,
                                   double min_sample_ms = 25.0,
                                   int max_adaptive_iters = 10000000);

void PrintTimeMs(const std::string &label, const TimingResult &result);

// === Cryptographic helpers ===

NTL::ZZ PrfZZ(int prf_key, const NTL::ZZ &mmod);
void PrfZZ(NTL::ZZ &res, int prf_key, const NTL::ZZ &mmod);

void PrfBn(bn_t res, int prf_key, const NTL::ZZ &mmod);

void GenerateRandomFunc(std::vector<std::vector<int>> &F_TEST, int msg_num, int degree_f);

void GeneratePartitions(int &cnt, std::vector<std::vector<int>> &Res, int sum, int k,
                        std::vector<int> &lst, int minn);

void RandomZZpx(ZZ_pX &a, int N, int q_bit);
void RlweSecretKey(ZZ_pX &sk, int N, int hsk);
void GaussRandom(ZZ_pX &e, int N);
void ZZpxScaleMul(ZZ_pX &out, const ZZ_pX &in1, const ZZ &in2);

ZZ MPE(const vec_ZZ &x, int degree_f);
ZZ MPE(const std::vector<ZZ> &x, int degree_f, const ZZ &modulus);
