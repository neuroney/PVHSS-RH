#pragma once

#include "HSSElg.h"

namespace pvhss { namespace group { namespace tcc25 {

using Tcc25EvalKey = pvhss::group::hss::HssEvalKey;
using Tcc25Ciphertext = pvhss::group::hss::HssCiphertext;
using Tcc25Memory = pvhss::group::hss::HssMemoryValue;

struct Tcc25Term
{
    int index;
    NTL::ZZ coeff;
};

struct Tcc25Constraint
{
    int index;
    std::vector<Tcc25Term> a_in;
    std::vector<Tcc25Term> b_wit;
    int output_witness;
};

struct Tcc25RmsR1cs
{
    int num_inputs;
    int num_witnesses;
    int output_witness;
    std::vector<Tcc25Constraint> constraints;
    std::vector<int> execution_order;
};

struct Tcc25SparseQap
{
    std::vector<NTL::ZZ> base_points;
    std::vector<NTL::ZZ> h_points;
    std::vector<std::vector<std::pair<int, NTL::ZZ>>> a_in_columns;
    std::vector<std::vector<std::pair<int, NTL::ZZ>>> b_wit_columns;
    std::vector<int> output_witness_by_row;
};

struct Tcc25Server
{
    int id;
    Tcc25EvalKey ek;
};

struct Tcc25ProofShare
{
    std::vector<NTL::ZZ> y_share;
    ep_t a_g1;
    ep2_t b_g2;
    ep_t c_g1;
};

struct Tcc25Proof
{
    std::vector<NTL::ZZ> y;
    ep_t a_g1;
    ep2_t b_g2;
    ep_t c_g1;
};

struct Tcc25G1Point
{
    ep_t value;
};

struct Tcc25G2Point
{
    ep2_t value;
};

struct Tcc25Param
{
    pvhss::group::hss::HssPublicKey pk;
    Tcc25RmsR1cs r1cs;
    Tcc25SparseQap qap;

    int skLen;
    int msg_bits;
    int degree_f;
    int msg_num;

    bn_t order;
    NTL::ZZ order_ZZ;

    ep_t alpha_g1;
    ep_t beta_g1;
    ep_t delta_g1;
    ep2_t beta_g2;
    ep2_t delta_g2;
    ep2_t g2;

    std::vector<Tcc25G1Point> public_input_g1;
    std::vector<Tcc25G1Point> output_g1;
    std::vector<Tcc25G1Point> a_query_g1;
    std::vector<Tcc25G1Point> b_query_g1;
    std::vector<Tcc25G2Point> b_query_g2;
    std::vector<Tcc25G1Point> hidden_query_g1;
    std::vector<bool> hidden_present;
    std::vector<Tcc25G1Point> h_query_g1;
};

void Tcc25_Setup(Tcc25Param &param, Tcc25Server &server0, Tcc25Server &server1);
void Tcc25_ProbGen(std::vector<Tcc25Ciphertext> &Ix, const Tcc25Param &param,
                   const NTL::vec_ZZ &x);
void Tcc25_Compute(Tcc25ProofShare &proof, int server_id, const Tcc25Param &param,
                   const Tcc25Server &server, const std::vector<Tcc25Ciphertext> &Ix);
bool Tcc25_Verify(Tcc25Proof &proof, const Tcc25ProofShare &pi0,
                  const Tcc25ProofShare &pi1, const Tcc25Param &param,
                  const NTL::vec_ZZ &x);
void Tcc25_Decode(NTL::ZZ &y, const Tcc25ProofShare &pi0,
                  const Tcc25ProofShare &pi1, const Tcc25Param &param);

bool Tcc25_ACC_TEST(int msg_num, int degree_f);

}}} // namespace pvhss::group::tcc25
