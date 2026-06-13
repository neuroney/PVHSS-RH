#pragma once

#include <NTL/ZZ.h>
#include <vector>
#include <cassert>

namespace pvhss { namespace programs {

/// A single term in an input-linear-combination: coeff * x_index.
/// index == 0 means the public constant 1; index >= 1 means private input x_index.
struct InputTerm {
    int index;        // 0 = constant 1; i >= 1  =>  x_i
    NTL::ZZ coeff;
};

/// A single term in a memory-linear-combination: coeff * M_mem_index.
struct MemTerm {
    int index;        // memory/witness slot index
    NTL::ZZ coeff;
};

/// Linear combination over inputs (and constant 1).
struct InputLinComb {
    std::vector<InputTerm> terms;
};

/// Linear combination over memory slots.
struct MemLinComb {
    std::vector<MemTerm> terms;
};

/// One RMS multiplication gate:
///   M_{output_mem} = a_in(x) * b_mem(M)
///
/// Invariant: a_in is an input-linear combination,
///            b_mem is a memory-linear combination.
/// Never: memory × memory or ciphertext × ciphertext.
struct RmsGate {
    int output_mem;         // destination memory slot
    InputLinComb a_in;      // L_in(x)
    MemLinComb b_mem;       // L_mem(M)
};

/// Copy initialisation: before gate evaluation, memory[dst] = memory[src].
/// Used by MPE to copy H_{i-1}[*] into H_i[*] slots before accumulation.
struct MemCopy {
    int dst;
    int src;
};

/// A complete RMS (Restricted Multiplication Straight-line) program.
struct RmsProgram {
    int num_inputs  = 0;    // number of private inputs  x_1 ... x_n
    int num_memory  = 0;    // total number of memory slots
    int const_mem   = 0;    // memory slot that holds constant 1
    std::vector<MemCopy> init_copies;  // copy memory[dst] = memory[src] before gate eval
    std::vector<RmsGate> mul_gates;
    MemLinComb output_lc;   // output = this linear combination of memory values
};

/// Validate invariants:
/// - Every gate output_mem is in [0, num_memory).
/// - Every gate's a_in uses only valid input indices.
/// - Every gate's b_mem uses only valid memory indices.
/// - Output LC uses only valid memory indices.
/// - No gate is memory × memory (always input-linear × memory-linear).
bool ValidateRmsProgram(const RmsProgram& program);

}} // namespace pvhss::programs
