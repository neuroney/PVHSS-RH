#include "helper.h"

#include <NTL/ZZ.h>
#include <vector>
#include <iostream>

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

// Test MPE against direct computation
static void test_mpe()
{
    cout << "\n--- Plaintext MPE Tests ---\n";

    ZZ modulus(0); // exact integer

    // m=1, d=1: F_{1,1} = x_1
    {
        vector<ZZ> x = { ZZ(5) };
        ZZ result = MPE(x, 1, modulus);
        check(result == ZZ(5), "MPE m=1,d=1: x_1 = 5");
    }

    // m=1, d=3: F_{1,3} = x_1 + x_1^2 + x_1^3
    {
        vector<ZZ> x = { ZZ(2) };
        ZZ result = MPE(x, 3, modulus);
        // 2 + 4 + 8 = 14
        check(result == ZZ(14), "MPE m=1,d=3: x_1=2 -> 14");
    }

    // m=2, d=3: F_{2,3} = x_1+x_2 + x_1^2+x_1x_2+x_2^2 + x_1^3+x_1^2x_2+x_1x_2^2+x_2^3
    {
        vector<ZZ> x = { ZZ(2), ZZ(3) };
        ZZ result = MPE(x, 3, modulus);
        // x1+x2 = 5, x1^2+x1x2+x2^2 = 4+6+9=19, x1^3+x1^2x2+x1x2^2+x2^3 = 8+12+18+27=65
        // Total: 5+19+65 = 89
        check(result == ZZ(89), "MPE m=2,d=3: [2,3] -> 89");
    }

    // m=5, d=5: spot check with small values
    {
        vector<ZZ> x = { ZZ(1), ZZ(1), ZZ(1), ZZ(1), ZZ(1) };
        ZZ result = MPE(x, 5, modulus);
        // For all inputs = 1, H_i[s] = C(i+s-1, s) binomial coefficients
        // The sum for m=5,d=5 should be a known value
        check(result > 0, "MPE m=5,d=5 all-1 produces positive result");
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

        ZZ result = MPE(x, 3, modulus);
        check(result == direct, "MPE matches expanded sum for m=2,d=3");
    }

    // m=5, d=15: verify result is not trivially zero
    {
        vector<ZZ> x;
        for (int i = 0; i < 5; ++i) x.push_back(ZZ(i + 1));
        ZZ result = MPE(x, 15, modulus);
        check(result > 0, "MPE m=5,d=15 produces non-zero result");
    }

    {
        vector<ZZ> x = { ZZ(2), ZZ(3) };
        ZZ result = MPE(x, 3, ZZ(17));
        check(result == ZZ(4), "MPE supports modular reduction");
    }
}

int main()
{
    cout << "=== PVHSS-RH MPE Reference Tests ===\n";

    test_mpe();

    cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===\n";
    return (pass_count == test_count) ? 0 : 1;
}
