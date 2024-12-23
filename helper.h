#pragma once
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <NTL/BasicThreadPool.h> 
#include <gmp.h>
extern "C"
{
#include <relic/relic.h>
}

using namespace std;
using namespace NTL;

const int NUMTHREADS = 8;

/**
 * Converts an unsigned integer to an array of 4 unsigned bytes.
 * @param[out] out 			Pointer to the output array.
 * @param[in] in 	 		Unsigned integer
 */
int int2uint8_t(uint8_t *out, int in);

/**
 * Converts a string of 4 unsigned bytes to an integer.
 * @param[out] out 			Output integer.
 * @param[in] in 	 		Pointer to the byte array to convert.
 */
int uint8_t2int(int *out, uint8_t *in);

/**
 * Converts an unsigned integer to a bn integer.
 * @param[out] out 			Output bn integer.
 * @param[in] in 	 		Unsigned integer
 */
int int2bn(bn_t out, int in);

/**
 * Converts a signed integer to a bn integer.
 * @param[out] out 			Output bn integer.
 * @param[in] in 	 		Signed integer
 * @param[in] size 	 		The size of the integer in base 10.
 */
int sint2bn(bn_t out, int in, int size);

/**
 * Converts a bn integer to an int.
 * @param[out] out 			Output bn integer.
 * @param[in] in 	 		Signed integer
 * @param[in] size 	 		The size of the integer in base 10.
 */
int bn2int(int *out, bn_t in);

void ZZ2bn(bn_t out, ZZ in);

void bn2ZZ(ZZ &out, const bn_t in);

void DataProcess(double &mean, double &stdev, double *Time, int cyctimes);

// void NativeEval_f(ZZ &y, int d, int num_data, int loop, int beg_ind, int *ind_var, vec_ZZ X, ZZ mmod);

void NativeEval(ZZ &y, int d, int num_data, vec_ZZ X, ZZ mmod, vector<vector<int>> F_TEST);

ZZ PRF_ZZ(int prfkey, ZZ mmod);

void PRF_bn(bn_t res, int prfkey, ZZ mmod);

void Random_Func(vector<vector<int>> &F_TEST, int msg_num, int degree_f);

void partitions(int &cnt, vector<vector<int>> &Res, int sum, int k, vector<int> &lst, int minn);

void GENERATE_RANDOM_FUNCTION(int msg_num, int degree_f);

ZZ Combine(int n, int m);