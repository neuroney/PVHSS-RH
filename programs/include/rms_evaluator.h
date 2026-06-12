#pragma once

#include "rms_program.h"

#include <vector>
#include <cassert>

namespace pvhss { namespace programs {

/// Generic RMS program evaluator.
///
/// Evaluates an RmsProgram using a Backend that provides:
///   - PublicKey, EvalKey, Ciphertext, Memory
///   - EncodeInput(pk, x) -> Ciphertext
///   - ConstantMemory(server_id, ek, c) -> Memory
///   - EvalInputLinComb(pk, inputs, lc) -> Ciphertext
///   - EvalMemLinComb(pk, memory, lc) -> Memory
///   - MulInputMemory(server_id, pk, ek, ct, mem, prf_key) -> Memory
///   - AddInPlace(acc, rhs, pk) -> void
///
/// Multiplication results are accumulated into the destination memory slot
/// (i.e. memory[out] += mul_result). This supports the PD2 recurrence:
///   H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]
/// where slots are updated in reverse-s order in-place.
///
/// @param program   the RMS program to evaluate
/// @param server_id 0 or 1
/// @param pk        public key
/// @param ek        evaluation key for this server
/// @param inputs    encoded ciphertext inputs (index 0 = x_1, index 1 = x_2, ...)
/// @param prf_key   PRF counter (modified in-place)
/// @return the output memory value (linear combination of final memory slots)
template <class Backend>
typename Backend::Memory EvalRmsProgram(
    const RmsProgram& program,
    int server_id,
    const typename Backend::PublicKey& pk,
    const typename Backend::EvalKey& ek,
    const std::vector<typename Backend::Ciphertext>& inputs,
    int& prf_key)
{
    assert(program.num_memory > 0);
    std::vector<typename Backend::Memory> memory(program.num_memory);

    // Initialize constant memory slot
    memory[program.const_mem] = Backend::ConstantMemory(server_id, ek, NTL::ZZ(1));

    // Process memory initialisation copies (if any)
    for (const auto& cp : program.init_copies)
    {
        memory[cp.dst] = memory[cp.src];
    }

    // Evaluate each multiplication gate with accumulation:
    //   memory[out] += L_in(x) * L_mem(M)
    for (const auto& gate : program.mul_gates)
    {
        auto a_ct  = Backend::EvalInputLinComb(pk, inputs, gate.a_in);
        auto b_mem = Backend::EvalMemLinComb(pk, memory, gate.b_mem);
        auto mul_result = Backend::MulInputMemory(server_id, pk, ek, a_ct, b_mem, prf_key);
        Backend::AddInPlace(memory[gate.output_mem], mul_result, pk);
    }

    // Compute output as linear combination of memory slots
    return Backend::EvalMemLinComb(pk, memory, program.output_lc);
}

}} // namespace pvhss::programs
