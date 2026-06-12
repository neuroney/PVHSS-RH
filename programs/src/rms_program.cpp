#include "rms_program.h"

namespace pvhss { namespace programs {

bool ValidateRmsProgram(const RmsProgram& program)
{
    // Basic sanity
    if (program.num_inputs < 0)  return false;
    if (program.num_memory <= 0) return false;
    if (program.const_mem < 0 || program.const_mem >= program.num_memory) return false;

    // Validate each gate
    for (const auto& gate : program.mul_gates)
    {
        // Output memory slot must be valid
        if (gate.output_mem < 0 || gate.output_mem >= program.num_memory) return false;

        // a_in: input-linear-combination — must refer only to valid input indices
        for (const auto& t : gate.a_in.terms)
        {
            if (t.index < 0 || t.index > program.num_inputs) return false;
        }

        // b_mem: memory-linear-combination — must refer only to valid memory indices
        for (const auto& t : gate.b_mem.terms)
        {
            if (t.index < 0 || t.index >= program.num_memory) return false;
        }

        // Gate must not be empty on both sides
        if (gate.a_in.terms.empty() || gate.b_mem.terms.empty()) return false;
    }

    // Validate output linear combination
    for (const auto& t : program.output_lc.terms)
    {
        if (t.index < 0 || t.index >= program.num_memory) return false;
    }

    return true;
}

}} // namespace pvhss::programs
