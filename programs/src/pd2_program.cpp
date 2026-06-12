#include "pd2_program.h"
#include <cassert>

namespace pvhss { namespace programs {

RmsProgram BuildPd2RmsProgram(int msg_num, int degree_f)
{
    assert(msg_num >= 1);
    assert(degree_f >= 1);

    RmsProgram program;
    program.num_inputs = msg_num;
    // Memory layout:
    //   slot 0: constant 1
    //   slots 1 .. degree_f:       prev[1..degree_f]  (H_{i-1})
    //   slots degree_f+1 .. 2*degree_f: curr[1..degree_f]  (H_i)
    program.num_memory = 1 + 2 * degree_f;
    program.const_mem  = 0;

    const int prev_base = 1;             // prev[s] at slot prev_base + (s-1)
    const int curr_base = 1 + degree_f;  // curr[s] at slot curr_base + (s-1)

    // H_0[0] = 1 is handled by constant slot 0.
    // H_0[s] = 0 for s > 0: prev slots start at 0 (default), which is correct.

    // For each input x_i, we process s = 1..degree_f.
    // Before processing input i, copy curr[s] = prev[s] for all s = 1..degree_f.
    // Then for each s: curr[s] += x_i * curr[s-1]   (where curr[0] = constant 1).
    //
    // Note: curr[s-1] for s=1 reads from slot 0 (constant 1), which is correct.

    for (int i = 1; i <= msg_num; ++i)
    {
        // Step 0: copy prev[*] -> curr[*] to start accumulation
        for (int s = 1; s <= degree_f; ++s)
        {
            MemCopy cp;
            cp.dst = curr_base + (s - 1);
            cp.src = prev_base + (s - 1);
            program.init_copies.push_back(cp);
        }

        // Step 1: create multiplication gates for s = 1..degree_f
        for (int s = 1; s <= degree_f; ++s)
        {
            RmsGate gate;
            gate.output_mem = curr_base + (s - 1);  // accumulate into curr[s]

            // a_in = x_i  (input index i, coefficient 1)
            InputTerm it;
            it.index = i;
            it.coeff = 1;
            gate.a_in.terms.push_back(it);

            // b_mem = curr[s-1]  (for s=1 this is slot 0 = constant 1)
            MemTerm mt;
            if (s == 1)
            {
                mt.index = 0;  // constant 1 slot
            }
            else
            {
                mt.index = curr_base + (s - 2);  // curr[s-1] from previous gate
            }
            mt.coeff = 1;
            gate.b_mem.terms.push_back(mt);

            program.mul_gates.push_back(gate);
        }

        // Step 2: after computing curr[*] = H_i[*], swap roles for next iteration
        // We do this by swapping prev_base and curr_base for the NEXT iteration,
        // but since we're generating the program statically, we just flip which
        // slots are "prev" and which are "curr" for the next i.
        //
        // Actually, we don't need to swap at the program level. The evaluator
        // handles memory in order. For iteration i:
        //   - Read prev[s] (which has H_{i-1}[s])
        //   - Copy to curr[s]
        //   - Accumulate into curr[s]
        // Next iteration i+1:
        //   - Read prev[s] which is the OLD prev[s] = H_{i-1}[s], but we need H_i[s]
        //   - We need curr[s] to become prev[s] for the next iteration
        //
        // So we need to SWAP the roles after each iteration. This can be done by
        // alternating the prev/curr assignment for odd/even i.

        // We use a different approach: for odd i, prev is in slots 1..degree_f,
        // curr is in slots degree_f+1..2*degree_f. For even i, prev is in
        // curr slots and curr is in prev slots.
    }

    // Re-implement with alternating buffer approach:
    // We'll rebuild with a cleaner alternating-buffer design.
    // Rather than using init_copies which the evaluator must handle,
    // we encode the swap inside the gate structure itself.
    //
    // The cleanest approach that works with a pure accumulate evaluator:
    // Use 2*(d+1) memory slots alternating between "read" and "write" buffers.
    // But since the evaluator template only reads b_mem and writes output_mem,
    // and output_mem accumulates (adds mul result to existing value),
    // we need to pre-initialize the write buffer from the read buffer.
    //
    // Let me use a completely different approach that avoids init_copies:
    // Represent H_i[s] = H_{i-1}[s] + x_i * H_i[s-1] as:
    //   temp = x_i * H_i[s-1]     (RMS gate: a_in=x_i, b_mem=H_i[s-1])
    //   H_i[s] = H_{i-1}[s] + temp  (another gate or accumulation)
    //
    // With accumulation in the evaluator:
    //   memory[dest] = H_{i-1}[s]  (pre-set)
    //   gate: a_in=x_i, b_mem=H_i[s-1] -> mul -> add to memory[dest]
    //   memory[dest] = H_{i-1}[s] + x_i * H_i[s-1] = H_i[s]
    //
    // The challenge is pre-setting memory[dest] to H_{i-1}[s].
    // This IS what init_copies does.

    // Let me go with a cleaner single-buffer approach:
    // Use d+1 slots for H[0..d], updated in-place.
    // H[0] = constant 1, initialized once.
    // For each input x_i:
    //   For s = d down to 1:  (reverse order to avoid overwriting needed values!)
    //     H[s] += x_i * H[s-1]
    //
    // This is the REVERSE recurrence, equivalent to the forward recurrence
    // because x_i * H[s-1] reads the UPDATED H[s-1] from the same iteration,
    // and accumulating into H[s] works because H[s-1] has already been updated
    // (since we go from high s to low s, and H[s] only depends on H[s-1] which
    // is at a LOWER index, which is processed LATER in the reverse order).
    //
    // Wait: reverse order means s = d, d-1, ..., 1:
    //   H[d] += x_i * H[d-1]   // H[d-1] is NOT yet updated (since d-1 < d, processed later)
    //   H[d-1] += x_i * H[d-2] // H[d-2] is NOT yet updated
    //   ...
    //   H[1] += x_i * H[0]     // H[0] is constant 1
    //
    // So reverse order reads the OLD H[s-1] from iteration i-1, which is correct!
    // And writes to H[s], which doesn't affect any subsequent read since we go
    // descending (H[d] doesn't affect H[d-1], etc.)
    //
    // With accumulation in the evaluator (memory[out] += mul_result):
    //   For s = d down to 1:
    //     gate: a_in = x_i, b_mem = slot[s-1], output = slot[s]
    //     memory[slot[s]] += x_i * memory[slot[s-1]]
    //
    // This is PERFECT! No init_copies needed! Single buffer!

    // Rebuild with this cleaner approach.
    program = RmsProgram();
    program.num_inputs = msg_num;
    // Slot 0: constant 1
    // Slots 1..degree_f: H[1..degree_f]
    program.num_memory = 1 + degree_f;
    program.const_mem  = 0;

    for (int i = 1; i <= msg_num; ++i)
    {
        // Forward order: s = 1, 2, ..., degree_f
        // H_i[s] = H_{i-1}[s] + x_i * H_i[s-1]
        // Accumulation handles H_{i-1}[s] part; gate computes x_i * H_i[s-1] and adds.
        // Forward order ensures H_i[s-1] is already computed when we reach s.
        for (int s = 1; s <= degree_f; ++s)
        {
            RmsGate gate;
            gate.output_mem = s;  // H[s] slot (accumulate)

            // a_in = x_i
            InputTerm it;
            it.index = i;
            it.coeff = 1;
            gate.a_in.terms.push_back(it);

            // b_mem = H[s-1]
            MemTerm mt;
            mt.index = s - 1;  // slot for H[s-1]
            mt.coeff = 1;
            gate.b_mem.terms.push_back(mt);

            program.mul_gates.push_back(gate);
        }
    }

    // Output: sum_{s=1}^{degree_f} H_m[s]
    for (int s = 1; s <= degree_f; ++s)
    {
        MemTerm mt;
        mt.index = s;
        mt.coeff = 1;
        program.output_lc.terms.push_back(mt);
    }

    // Verify multiplication count
    assert(static_cast<int>(program.mul_gates.size()) == msg_num * degree_f);

    return program;
}

}} // namespace pvhss::programs
