# PVHSS-RH Refactoring Task Plan

## 0. Background

The current repository structure is functional for experiments, but it has become hard to maintain because protocol logic, backend logic, program logic, proof logic, and benchmark logic are mixed together.

The main problem is that the repository currently expands implementations along two axes:

```text
backend  ×  protocol
```

For example:

```text
group/hss
group/vhss
group/ot
group/dc
group/tcc25

rlwe/hss
rlwe/vhss
rlwe/ot
rlwe/dc

cz
```

This causes repeated implementations of:

```text
Setup
ProbGen / Input
Compute / Eval
Verify
Decode
Benchmark driver
PD2 polynomial evaluation
```

The target refactoring is to separate the repository into the following conceptual layers:

```text
Program layer:
    PD2 / RMS programs / plaintext reference evaluators

Backend layer:
    group HSS / group VHSS / RLWE HSS / RLWE VHSS / CZ backend components if needed

Proof layer:
    none / VHSS linear verification / Pedersen / DecPed / TCC25 / CZ verification

Protocol layer:
    HSS / VHSS / OT / DC / TCC25 / CZ

Benchmark layer:
    shared benchmark runner
```

Important correction:

> `cz` must be treated as a protocol, not as a backend and not as common utility code.

---

## 1. High-level goal

Refactor the repository so that all protocols share:

1. one program representation;
2. one PD2 program builder;
3. one RMS-compatible evaluator interface;
4. one benchmark runner;
5. explicit protocol adapters.

The immediate purpose is not to make the code beautiful.  
The immediate purpose is to make benchmarks fair and make future protocol changes easier.

---

## 2. Non-goals

Do not attempt a full rewrite.

Do not change the cryptographic algorithms unless explicitly assigned.

Do not optimize parameters.

Do not replace the current group or RLWE primitives.

Do not attempt to fully fix TCC25 hidden-input verification in this refactoring pass.

Do not merge CZ into OT/DC/VHSS. CZ should remain a separate protocol adapter.

---

## 3. Key design rule

Every protocol should be written as:

```text
ProgramBuilder + Backend + ProofLayer + ProtocolDriver
```

For example:

```text
group-hss-pd2
    = Pd2Program + GroupHssBackend + NoProofLayer + TwoServerProtocolDriver

group-ot-pd2
    = Pd2Program + GroupVhssBackend + PedProofLayer + TwoServerProtocolDriver

group-dc-pd2
    = Pd2Program + GroupVhssBackend + DecPedProofLayer + TwoServerProtocolDriver

group-tcc25-pd2
    = Pd2Program + GroupHssBackend + Tcc25ProofLayer + TwoServerProtocolDriver

rlwe-ot-pd2
    = Pd2Program + RlweVhssBackend + PedProofLayer + TwoServerProtocolDriver

rlwe-dc-pd2
    = Pd2Program + RlweVhssBackend + DecPedProofLayer + TwoServerProtocolDriver

cz-pd2
    = Pd2Program + CzBackend/CzPkeLayer + CzProofLayer + CzProtocolDriver
```

The exact class names may be adjusted, but this separation must be preserved.

---

## 4. Repository layout target

Create the following new directories gradually.

```text
common/
  include/
    timing.h
    random_utils.h
    ntl_utils.h
    relic_utils.h
    bench_config.h
  src/

programs/
  include/
    rms_program.h
    pd2_program.h
    plaintext_pd2.h
  src/
    rms_program.cpp
    pd2_program.cpp
    plaintext_pd2.cpp

backends/
  group_hss/
    include/
      group_hss_backend.h
    src/
      group_hss_backend.cpp

  group_vhss/
    include/
      group_vhss_backend.h
    src/
      group_vhss_backend.cpp

  rlwe_hss/
    include/
      rlwe_hss_backend.h
    src/
      rlwe_hss_backend.cpp

  rlwe_vhss/
    include/
      rlwe_vhss_backend.h
    src/
      rlwe_vhss_backend.cpp

  cz/
    include/
      cz_backend.h
      cz_pke.h
    src/
      cz_backend.cpp
      cz_pke.cpp

proofs/
  none/
  vhss_check/
  ped/
  decped/
  tcc25/
  cz/

protocols/
  hss/
  vhss/
  ot/
  dc/
  tcc25/
  cz/

benchmarks/
  common/
    protocol_bench_runner.h
  protocols/
```

This is the target structure.  
It can be introduced gradually. Existing directories do not need to be deleted in the first pass.

---

## 5. Phase 1: add a common RMS program representation

### 5.1 Files to add

Create:

```text
programs/include/rms_program.h
programs/src/rms_program.cpp
```

### 5.2 Data structures

Implement a minimal intermediate representation:

```cpp
#pragma once

#include <NTL/ZZ.h>
#include <vector>

namespace pvhss::programs {

struct InputTerm {
    int index;        // 0 means public constant 1, i >= 1 means private input x_i
    NTL::ZZ coeff;
};

struct MemTerm {
    int index;        // memory/witness index
    NTL::ZZ coeff;
};

struct InputLinComb {
    std::vector<InputTerm> terms;
};

struct MemLinComb {
    std::vector<MemTerm> terms;
};

struct RmsGate {
    int output_mem;
    InputLinComb a_in;
    MemLinComb b_mem;
};

struct RmsProgram {
    int num_inputs;       // number of private inputs
    int num_memory;       // total memory slots
    int const_mem;        // memory slot containing public constant 1
    std::vector<RmsGate> mul_gates;
    MemLinComb output_lc;
};

} // namespace pvhss::programs
```

### 5.3 Invariant

Every multiplication gate must mean:

\[
M_{\mathsf{out}}
=
L_{\mathsf{in}}(\mathbf{x})\cdot L_{\mathsf{mem}}(\mathbf{M}).
\]

Allowed:

\[
x_i \cdot M,
\]

\[
(x_i+x_j+3)\cdot(M_1+2M_2).
\]

Forbidden:

\[
M_1\cdot M_2.
\]

Forbidden:

\[
\mathsf{ct}(x_i)\cdot\mathsf{ct}(x_j).
\]

---

## 6. Phase 2: add the shared PD2 program builder

### 6.1 Files to add

Create:

```text
programs/include/pd2_program.h
programs/src/pd2_program.cpp
programs/include/plaintext_pd2.h
programs/src/plaintext_pd2.cpp
```

### 6.2 Function signatures

```cpp
namespace pvhss::programs {

RmsProgram BuildPd2RmsProgram(int msg_num, int degree_f);

NTL::ZZ PlaintextPd2(
    const std::vector<NTL::ZZ>& x,
    int degree_f,
    const NTL::ZZ& modulus
);

} // namespace pvhss::programs
```

### 6.3 Required recurrence

Use the optimized complete-homogeneous recurrence:

\[
H_i[s]=H_{i-1}[s]+x_iH_i[s-1].
\]

Initialization:

\[
H_0[0]=1,
\]

\[
H_0[s]=0 \quad \text{for }s>0.
\]

For each input \(x_i\):

\[
H_i[0]=1.
\]

For each degree \(s=1,\ldots,d\), create one RMS multiplication gate:

\[
P_{i,s}=x_i\cdot H_i[s-1].
\]

Then define the new memory value:

\[
H_i[s]=H_{i-1}[s]+P_{i,s}.
\]

The output is:

\[
y=\sum_{s=1}^{d}H_m[s].
\]

### 6.4 Multiplication count

The program builder must create exactly:

\[
m d
\]

RMS multiplication gates.

Add an assertion:

```cpp
assert(program.mul_gates.size() == msg_num * degree_f);
```

### 6.5 Optional prefix version

If a protocol only needs the sum of all degrees and not individual \(H_m[s]\), a prefix recurrence may be added later:

\[
P_i[s]=P_{i-1}[s]+x_iP_i[s-1].
\]

This also costs:

\[
m d
\]

multiplications.

Do not implement this in Phase 2 unless needed.

---

## 7. Phase 3: add backend adapters

### 7.1 Purpose

Backends should expose the same logical operations:

```text
input encoding
input-to-memory conversion
input-linear-combination evaluation
memory-linear-combination evaluation
input × memory multiplication
memory addition
memory scalar multiplication
decode/reconstruct
```

The common RMS evaluator must call backend methods, not protocol-specific methods.

---

### 7.2 Group HSS backend

Create:

```text
backends/group_hss/include/group_hss_backend.h
backends/group_hss/src/group_hss_backend.cpp
```

The adapter should wrap the existing group HSS functions.

Suggested interface:

```cpp
namespace pvhss::backend {

struct GroupHssBackend {
    using PublicKey = /* existing HssPublicKey */;
    using EvalKey = /* existing HssEvalKey */;
    using Ciphertext = /* existing HssCiphertext */;
    using Memory = /* existing HssMemoryValue */;

    static Ciphertext EncodeInput(
        const PublicKey& pk,
        const NTL::ZZ& x
    );

    static Memory ConstantMemory(
        int server_id,
        const EvalKey& ek,
        const NTL::ZZ& c
    );

    static Memory EvalInputLinComb(
        const PublicKey& pk,
        const std::vector<Ciphertext>& inputs,
        const pvhss::programs::InputLinComb& lc
    );

    static Memory EvalMemLinComb(
        const PublicKey& pk,
        const std::vector<Memory>& memory,
        const pvhss::programs::MemLinComb& lc
    );

    static Memory MulInputMemory(
        int server_id,
        const PublicKey& pk,
        const EvalKey& ek,
        const Ciphertext& a,
        const Memory& b,
        int& prf_key
    );

    static void AddInPlace(Memory& acc, const Memory& rhs);
    static void ScalarMulInPlace(Memory& acc, const NTL::ZZ& c);
};

} // namespace pvhss::backend
```

Adjust names to match existing types.

---

### 7.3 Group VHSS backend

Create:

```text
backends/group_vhss/include/group_vhss_backend.h
backends/group_vhss/src/group_vhss_backend.cpp
```

The goal is to move existing `VhssElgamal...` functions out of `common`.

Important:

> `common` should not contain concrete VHSS protocol logic.  
> It should contain only utilities.

---

### 7.4 RLWE backends

Create:

```text
backends/rlwe_hss/include/rlwe_hss_backend.h
backends/rlwe_hss/src/rlwe_hss_backend.cpp

backends/rlwe_vhss/include/rlwe_vhss_backend.h
backends/rlwe_vhss/src/rlwe_vhss_backend.cpp
```

These should wrap the existing RLWE HSS/VHSS logic.

Do not change parameters in this phase.

---

### 7.5 CZ backend components

Create:

```text
backends/cz/include/cz_backend.h
backends/cz/include/cz_pke.h
backends/cz/src/cz_backend.cpp
backends/cz/src/cz_pke.cpp
```

Move low-level PKE and encoding utilities from current CZ headers into this backend layer.

However:

> CZ itself remains a protocol under `protocols/cz`.

The backend layer should contain only low-level reusable operations, such as:

```text
PKE generation
PKE encryption
distributed decryption / multiplication primitive
input encoding
memory representation
pairing utility wrappers if they are reusable
```

The protocol flow remains in `protocols/cz`.

---

## 8. Phase 4: add a common RMS evaluator

### 8.1 Files to add

Create:

```text
programs/include/rms_evaluator.h
```

This can initially be header-only.

### 8.2 Generic evaluator

```cpp
namespace pvhss::programs {

template <class Backend>
typename Backend::Memory EvalRmsProgram(
    const RmsProgram& program,
    int server_id,
    const typename Backend::PublicKey& pk,
    const typename Backend::EvalKey& ek,
    const std::vector<typename Backend::Ciphertext>& inputs,
    int& prf_key
) {
    std::vector<typename Backend::Memory> memory(program.num_memory);

    // initialize constant memory slot
    memory[program.const_mem] = Backend::ConstantMemory(server_id, ek, NTL::ZZ(1));

    for (const auto& gate : program.mul_gates) {
        auto a = Backend::EvalInputLinComb(pk, inputs, gate.a_in);
        auto b = Backend::EvalMemLinComb(pk, memory, gate.b_mem);
        memory[gate.output_mem] =
            Backend::MulInputMemory(server_id, pk, ek, a, b, prf_key);
    }

    return Backend::EvalMemLinComb(pk, memory, program.output_lc);
}

} // namespace pvhss::programs
```

This is the core abstraction.

All direct PD2 evaluators should eventually call this.

---

## 9. Phase 5: convert direct HSS and VHSS PD2 evaluators

### 9.1 Replace duplicated PD2 logic

Replace old duplicated PD2 evaluation code with:

```cpp
auto program = BuildPd2RmsProgram(msg_num, degree_f);
auto y_share = EvalRmsProgram<GroupHssBackend>(
    program, server_id, pk, ek, inputs, prf_key
);
```

and similarly for:

```text
GroupVhssBackend
RlweHssBackend
RlweVhssBackend
CzBackend if compatible
```

### 9.2 Required test

For \(m\) inputs and degree \(d\), count HSS/PVHSS multiplication calls.

Expected:

\[
m d
\]

for direct witness/output evaluation.

For example:

```text
m = 5
d = 15
expected witness multiplications = 75
```

The old implementation had:

\[
m\cdot \frac{d(d+1)(d+2)}{6}.
\]

For \(m=5,d=15\), this was:

\[
3400.
\]

After Phase 5, this should drop to 75.

---

## 10. Phase 6: protocol adapters

Each protocol should expose a small uniform interface.

### 10.1 Generic protocol interface

Use the following conceptual interface:

```cpp
struct Protocol {
    static SetupOutput Setup(const BenchConfig& cfg);

    static ProbGenOutput ProbGen(
        const SetupOutput& pp,
        const std::vector<NTL::ZZ>& x
    );

    static ServerOutput Compute(
        const SetupOutput& pp,
        const ProbGenOutput& task,
        int server_id
    );

    static VerifyOutput Verify(
        const SetupOutput& pp,
        const ProbGenOutput& task,
        const ServerOutput& out0,
        const ServerOutput& out1
    );

    static NTL::ZZ Decode(
        const SetupOutput& pp,
        const ServerOutput& out0,
        const ServerOutput& out1
    );
};
```

The concrete type names may be protocol-specific.  
The logical shape should be the same.

---

### 10.2 HSS protocol

Location:

```text
protocols/hss/
```

Responsibility:

```text
Setup backend keys
Encode private input
Run RMS evaluator on both servers
Decode output shares
No public verification layer
```

---

### 10.3 VHSS protocol

Location:

```text
protocols/vhss/
```

Responsibility:

```text
Setup VHSS backend keys
Encode private input
Run RMS evaluator
Generate/check VHSS verification material
Decode output
```

---

### 10.4 OT protocol

Location:

```text
protocols/ot/
```

Responsibility:

```text
Use backend evaluator
Add Pedersen-style proof/verification layer
Keep OT-specific proof logic in proofs/ped or protocols/ot
```

The duplicated `Setup/ProbGen/Compute/Verify/Decode` logic should be removed gradually.

---

### 10.5 DC protocol

Location:

```text
protocols/dc/
```

Responsibility:

```text
Use backend evaluator
Add DecPed proof/verification layer
Keep DC-specific proof logic in proofs/decped or protocols/dc
```

Do not duplicate OT logic except where the proof layer genuinely differs.

---

### 10.6 TCC25 protocol

Location:

```text
protocols/tcc25/
```

Responsibility:

```text
Build RMS-compatible R1CS/QAP
Use backend evaluator for witness computation
Use proof layer for TCC/Groth-style proof generation
Verify proof
```

Important caveat:

Current TCC25 implementation has two modes conceptually:

```text
scalar-debug mode:
    may use trapdoor scalars
    acceptable only for correctness testing

real-crs mode:
    servers must not know tau, alpha, beta, delta
    proof generation must use CRS group elements and MSMs
    required for real benchmark claims
```

In this refactoring pass, keep scalar-debug mode if needed, but mark it clearly.

---

### 10.7 CZ protocol

Location:

```text
protocols/cz/
```

Responsibility:

```text
Keep the CZ protocol flow:
    setup
    input generation
    two-server evaluation
    verification

Move low-level PKE and utility code into backends/cz or common utilities.
Keep CZ-specific verification/protocol logic here.
```

CZ should be a first-class protocol target, for example:

```text
protocol_cz_bench
```

or:

```text
pvhss_protocol_cz
```

Do not treat CZ as:

```text
common utility
backend only
old standalone executable only
```

It should participate in the same benchmark and testing structure as HSS, VHSS, OT, DC, and TCC25.

---

## 11. Phase 7: benchmark refactoring

### 11.1 Shared benchmark runner

Create:

```text
benchmarks/common/protocol_bench_runner.h
```

Implement a generic runner:

```cpp
template <class Protocol>
void RunProtocolBench(const BenchConfig& cfg) {
    auto setup = Measure("setup", [&] {
        return Protocol::Setup(cfg);
    });

    auto x = SampleInputs(cfg);

    auto task = Measure("probgen", [&] {
        return Protocol::ProbGen(setup.value, x);
    });

    auto out0 = Measure("compute_server_0", [&] {
        return Protocol::Compute(setup.value, task.value, 0);
    });

    auto out1 = Measure("compute_server_1", [&] {
        return Protocol::Compute(setup.value, task.value, 1);
    });

    auto verify = Measure("verify", [&] {
        return Protocol::Verify(setup.value, task.value, out0.value, out1.value);
    });

    auto decoded = Protocol::Decode(setup.value, out0.value, out1.value);

    auto reference = PlaintextPd2(x, cfg.degree, cfg.modulus);

    assert(decoded == reference);
    assert(verify.value.accepted);
}
```

The exact return types may differ.  
The key point is to avoid duplicating benchmark flow.

---

### 11.2 Required counters

Every benchmark must print:

```text
setup_ms
probgen_ms
compute0_ms
compute1_ms
verify_ms
decode_ms

witness_mul_count
proof_mul_count
total_mul_count

memory_add_count
memory_scalar_mul_count
input_lincomb_count

group_scalar_mul_count
msm_count
pairing_count
```

Not every protocol will use all counters.  
Unused counters should print zero.

---

### 11.3 Why counters are required

Wall-clock timing alone is not enough.

The old direct HSS PD2 evaluator and TCC25 evaluator were not comparable because they used different recurrences and different numbers of expensive multiplications.

For PD2, the expected witness multiplication count after refactoring is:

\[
m d.
\]

Any benchmark that reports a different count must explain why.

---

## 12. Phase 8: CMake cleanup

### 12.1 Problem

The current build system often builds executables by listing implementation `.cpp` files directly in each target.

This makes dependencies fragile and causes duplicated compilation.

### 12.2 Target structure

Use library targets:

```cmake
add_library(pvhss_programs ...)
add_library(pvhss_group_hss_backend ...)
add_library(pvhss_group_vhss_backend ...)
add_library(pvhss_rlwe_hss_backend ...)
add_library(pvhss_rlwe_vhss_backend ...)
add_library(pvhss_cz_backend ...)

add_library(pvhss_protocol_hss ...)
add_library(pvhss_protocol_vhss ...)
add_library(pvhss_protocol_ot ...)
add_library(pvhss_protocol_dc ...)
add_library(pvhss_protocol_tcc25 ...)
add_library(pvhss_protocol_cz ...)
```

Benchmark executables should link libraries:

```cmake
target_link_libraries(protocol_group_hss_bench
    PRIVATE
        pvhss_protocol_hss
        pvhss_group_hss_backend
        pvhss_programs
        pvhss_external_deps
)
```

Do not repeatedly list the same `.cpp` files in each benchmark executable.

### 12.3 CZ build target

Add CZ to the main build structure.

At minimum:

```cmake
add_subdirectory(protocols/cz)
```

or, during transition:

```cmake
add_subdirectory(schemes/cz)
```

But the final target should be:

```text
pvhss_protocol_cz
protocol_cz_bench
```

---

## 13. Phase 9: tests

### 13.1 Plaintext PD2 tests

Add tests for:

```text
m = 1, d = 1
m = 2, d = 3
m = 5, d = 5
m = 5, d = 15
```

Check:

```text
PlaintextPd2 == direct expanded reference
```

---

### 13.2 RMS program tests

For `BuildPd2RmsProgram(m,d)` check:

```text
program.mul_gates.size() == m*d
program.output_lc is non-empty
every gate has input-side × memory-side form
no memory × memory gate exists
```

---

### 13.3 Backend evaluator tests

For each backend:

```text
GroupHssBackend
GroupVhssBackend
RlweHssBackend
RlweVhssBackend
CzBackend if supported
```

Run:

```text
encode inputs
evaluate RMS program on server 0
evaluate RMS program on server 1
reconstruct/decode
compare to PlaintextPd2
```

---

### 13.4 Protocol tests

For each protocol:

```text
hss
vhss
ot
dc
tcc25
cz
```

Run:

```text
Setup
ProbGen
Compute server 0
Compute server 1
Verify
Decode
Compare to PlaintextPd2
```

---

## 14. Migration strategy

Do not delete old code immediately.

Use this sequence:

### Step 1

Add `programs/` and implement `RmsProgram`, `BuildPd2RmsProgram`, and `PlaintextPd2`.

No existing protocol changes yet.

### Step 2

Add counters around existing HSS/PVHSS multiplication calls.

Print counts in existing benchmarks.

### Step 3

Add one backend adapter, preferably `GroupHssBackend`.

Port direct group HSS PD2 to the new evaluator.

Confirm multiplication count drops to:

\[
m d.
\]

### Step 4

Port `GroupVhssBackend`.

Confirm group VHSS produces the same output as before.

### Step 5

Port RLWE backends.

Do not change parameters.

### Step 6

Move CZ low-level functions into `backends/cz`, but keep current CZ executable working.

### Step 7

Create `protocols/cz` and make CZ use the shared benchmark runner.

### Step 8

Port OT/DC to the generic protocol driver.

### Step 9

Port TCC25 last, because TCC25 has the extra complication of scalar-debug mode versus real-CRS mode.

---

## 15. Acceptance criteria

The refactoring is acceptable only if all of the following hold.

### Correctness

For all migrated protocols:

```text
decoded output == PlaintextPd2(input, degree)
```

### Multiplication count

For direct PD2 witness/output evaluation:

\[
\text{witness_mul_count}=m d.
\]

### No forbidden multiplication

Every RMS multiplication gate must be:

\[
\text{input-linear} \times \text{memory-linear}.
\]

### CZ included

CZ must appear as a protocol in:

```text
protocols/cz
```

and must have a benchmark target.

### Old benchmarks still comparable

During transition, old and new benchmark results should be printed side-by-side for at least one backend.

Example:

```text
old_group_hss_pd2
new_group_hss_pd2_rms
```

### Build

The following should work:

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build
```

---

## 16. Notes on CZ

CZ is currently structurally different from the other schemes.

It is currently closer to a standalone protocol implementation with:

```text
PKE generation
input generation
PVHSS generation
two-server evaluation
pairing-based verification
benchmark loop
```

The refactor should not erase this identity.

The correct decomposition is:

```text
backends/cz:
    low-level PKE and encoding/decryption primitives

proofs/cz:
    pairing verification helpers if they are CZ-specific

protocols/cz:
    actual CZ protocol flow

benchmarks/protocols:
    CZ benchmark target using shared benchmark runner
```

Do not move CZ into:

```text
common/
```

except for generic helper functions.

Do not move CZ into:

```text
backends/
```

as a whole.

Only low-level reusable CZ components belong to `backends/cz`.

---

## 17. Immediate TODO list for intern

### TODO 1

Create:

```text
programs/include/rms_program.h
programs/src/rms_program.cpp
programs/include/pd2_program.h
programs/src/pd2_program.cpp
programs/include/plaintext_pd2.h
programs/src/plaintext_pd2.cpp
```

### TODO 2

Implement:

```cpp
RmsProgram BuildPd2RmsProgram(int msg_num, int degree_f);
```

using:

\[
H_i[s]=H_{i-1}[s]+x_iH_i[s-1].
\]

### TODO 3

Add a plaintext test for \(m=2,d=3\):

\[
F_{2,3}
=
x_1+x_2+x_1^2+x_1x_2+x_2^2+x_1^3+x_1^2x_2+x_1x_2^2+x_2^3.
\]

### TODO 4

Add a multiplication counter around the expensive HSS/PVHSS multiplication primitive.

### TODO 5

Add `GroupHssBackend`.

### TODO 6

Port group HSS PD2 evaluation to:

```cpp
BuildPd2RmsProgram + EvalRmsProgram<GroupHssBackend>
```

### TODO 7

Show benchmark before/after:

```text
old HSS PD2 multiplication count:
    m*d*(d+1)*(d+2)/6

new HSS PD2 multiplication count:
    m*d
```

For \(m=5,d=15\):

```text
old: 3400
new: 75
```

### TODO 8

Add CZ to the refactoring plan as a protocol:

```text
protocols/cz
```

and do not classify it as common or backend.

---

## 18. Final principle

The repository should not contain separate handwritten PD2 evaluators for each protocol.

The repository should contain one PD2-to-RMS compiler:

\[
\texttt{BuildPd2RmsProgram}
\]

and every protocol should consume the same RMS program through its backend adapter.

The protocol list should explicitly include:

```text
hss
vhss
ot
dc
tcc25
cz
```

CZ is a protocol.
