#include "rms_program.h"
#include "mpe_program.h"
#include "plaintext_mpe.h"

#include <NTL/ZZ.h>
#include <vector>
#include <iostream>
#include <cassert>

using namespace pvhss::programs;
using namespace NTL;
using namespace std;

static int test_count = 0;
static int pass_count = 0;

static void check(bool cond, const string& msg)
{
    ++test_count;
    if (cond) { ++pass_count; cout << "  PASS: " << msg << "\n"; }
    else      { cout << "  FAIL: " << msg << "\n"; }
}

// Test RMS program validation
static void test_rms_program_validation()
{
    cout << "\n--- RMS Program Validation Tests ---\n";

    // Valid MPE program
    auto prog = BuildMpeRmsProgram(2, 3);
    check(ValidateRmsProgram(prog), "BuildMpeRmsProgram(2,3) is valid");

    // Multiplication count
    check(static_cast<int>(prog.mul_gates.size()) == 2 * 3,
          "mul_gates.size() == msg_num * degree_f (2*3=6)");

    // Output LC non-empty
    check(!prog.output_lc.terms.empty(), "output_lc is non-empty");

    // Every gate has input-side × memory-side
    bool all_input_mem = true;
    for (const auto& g : prog.mul_gates)
    {
        if (g.a_in.terms.empty() || g.b_mem.terms.empty())
        {
            all_input_mem = false;
            break;
        }
    }
    check(all_input_mem, "every gate has input-side × memory-side form");

    // Verify multiplication count for various parameters
    for (int m : {1, 2, 5})
    {
        for (int d : {1, 3, 5, 15})
        {
            auto p = BuildMpeRmsProgram(m, d);
            check(static_cast<int>(p.mul_gates.size()) == m * d,
                  "mul_gates.size() == " + to_string(m) + "*" + to_string(d));
        }
    }
}

// Test PlaintextMpe against direct computation
static void test_plaintext_mpe()
{
    cout << "\n--- Plaintext MPE Tests ---\n";

    ZZ modulus(0); // exact integer

    // m=1, d=1: F_{1,1} = x_1
    {
        vector<ZZ> x = { ZZ(5) };
        ZZ result = PlaintextMpe(x, 1, modulus);
        check(result == ZZ(5), "PlaintextMpe m=1,d=1: x_1 = 5");
    }

    // m=1, d=3: F_{1,3} = x_1 + x_1^2 + x_1^3
    {
        vector<ZZ> x = { ZZ(2) };
        ZZ result = PlaintextMpe(x, 3, modulus);
        // 2 + 4 + 8 = 14
        check(result == ZZ(14), "PlaintextMpe m=1,d=3: x_1=2 -> 14");
    }

    // m=2, d=3: F_{2,3} = x_1+x_2 + x_1^2+x_1x_2+x_2^2 + x_1^3+x_1^2x_2+x_1x_2^2+x_2^3
    {
        vector<ZZ> x = { ZZ(2), ZZ(3) };
        ZZ result = PlaintextMpe(x, 3, modulus);
        // x1+x2 = 5, x1^2+x1x2+x2^2 = 4+6+9=19, x1^3+x1^2x2+x1x2^2+x2^3 = 8+12+18+27=65
        // Total: 5+19+65 = 89
        check(result == ZZ(89), "PlaintextMpe m=2,d=3: [2,3] -> 89");
    }

    // m=5, d=5: spot check with small values
    {
        vector<ZZ> x = { ZZ(1), ZZ(1), ZZ(1), ZZ(1), ZZ(1) };
        ZZ result = PlaintextMpe(x, 5, modulus);
        // For all inputs = 1, H_i[s] = C(i+s-1, s) binomial coefficients
        // The sum for m=5,d=5 should be a known value
        check(result > 0, "PlaintextMpe m=5,d=5 all-1 produces positive result");
    }

    // m=2, d=3 against direct expanded sum (from spec)
    {
        vector<ZZ> x = { ZZ(2), ZZ(3) };
        // Direct: sum of all monomials up to degree 3
        ZZ direct(0);
        // degree 1: x1 + x2
        direct += x[0] + x[1];
        // degree 2: x1^2 + x1*x2 + x2^2
        direct += x[0]*x[0] + x[0]*x[1] + x[1]*x[1];
        // degree 3: x1^3 + x1^2*x2 + x1*x2^2 + x2^3
        direct += x[0]*x[0]*x[0] + x[0]*x[0]*x[1] + x[0]*x[1]*x[1] + x[1]*x[1]*x[1];

        ZZ result = PlaintextMpe(x, 3, modulus);
        check(result == direct, "PlaintextMpe matches expanded sum for m=2,d=3");
    }

    // m=5, d=15: verify result is not trivially zero
    {
        vector<ZZ> x;
        for (int i = 0; i < 5; ++i) x.push_back(ZZ(i + 1));
        ZZ result = PlaintextMpe(x, 15, modulus);
        check(result > 0, "PlaintextMpe m=5,d=15 produces non-zero result");
    }
}

int main()
{
    cout << "=== PVHSS-RH Program Layer Tests ===\n";

    test_rms_program_validation();
    test_plaintext_mpe();

    cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===\n";
    return (pass_count == test_count) ? 0 : 1;
}
