#include "Tcc25Group.h"

#include <map>

using namespace NTL;
using namespace std;

namespace pvhss { namespace group { namespace tcc25 {

namespace
{

using pvhss::group::hss::HssAddMemory;
using pvhss::group::hss::HssConvertInput;
using pvhss::group::hss::HssInput;
using pvhss::group::hss::HssMul;

struct WitnessLinComb
{
    map<int, ZZ> terms;

    static WitnessLinComb Zero()
    {
        return WitnessLinComb();
    }

    static WitnessLinComb FromWitness(int index, const ZZ &coeff)
    {
        WitnessLinComb lc;
        lc.AddTerm(index, coeff);
        return lc;
    }

    void AddTerm(int index, const ZZ &coeff)
    {
        if (coeff == 0)
        {
            return;
        }
        terms[index] += coeff;
        if (terms[index] == 0)
        {
            terms.erase(index);
        }
    }

    void AddAssign(const WitnessLinComb &other)
    {
        for (map<int, ZZ>::const_iterator it = other.terms.begin(); it != other.terms.end(); ++it)
        {
            AddTerm(it->first, it->second);
        }
    }

    bool IsZero() const
    {
        return terms.empty();
    }

    vector<Tcc25Term> ToTerms() const
    {
        vector<Tcc25Term> out;
        out.reserve(terms.size());
        for (map<int, ZZ>::const_iterator it = terms.begin(); it != terms.end(); ++it)
        {
            out.push_back(Tcc25Term{it->first, it->second});
        }
        return out;
    }
};

ZZ ModQ(const ZZ &x, const ZZ &q)
{
    ZZ r = x % q;
    if (r < 0)
    {
        r += q;
    }
    return r;
}

ZZ AddQ(const ZZ &a, const ZZ &b, const ZZ &q)
{
    return ModQ(a + b, q);
}

ZZ SubQ(const ZZ &a, const ZZ &b, const ZZ &q)
{
    return ModQ(a - b, q);
}

ZZ MulQ(const ZZ &a, const ZZ &b, const ZZ &q)
{
    return ModQ(a * b, q);
}

ZZ InvQ(const ZZ &a, const ZZ &q)
{
    ZZ inv;
    InvMod(inv, ModQ(a, q), q);
    return inv;
}

ZZ CenteredQ(const ZZ &x, const ZZ &q)
{
    ZZ r = ModQ(x, q);
    if (r > q / 2)
    {
        r -= q;
    }
    return r;
}

ZZ RandomNonZeroScalar(const ZZ &q)
{
    ZZ value;
    do
    {
        RandomBnd(value, q);
    } while (value == 0);
    return value;
}

void InitPairing()
{
    core_init();
    ep_curve_init();
    ep_param_set(B12_P381);
    ep2_curve_init();
    ep2_curve_set_twist(RLC_EP_MTYPE);
}

void G1MulGenZZ(ep_t out, const ZZ &scalar, const ZZ &q)
{
    const ZZ s = ModQ(scalar, q);
    if (s == 0)
    {
        ep_set_infty(out);
        return;
    }
    bn_t s_bn;
    ZZtoBn(s_bn, s);
    ep_mul_gen(out, s_bn);
    ep_norm(out, out);
}

void G2MulGenZZ(ep2_t out, const ZZ &scalar, const ZZ &q)
{
    const ZZ s = ModQ(scalar, q);
    if (s == 0)
    {
        ep2_set_infty(out);
        return;
    }
    bn_t s_bn;
    ZZtoBn(s_bn, s);
    ep2_mul_gen(out, s_bn);
    ep2_norm(out, out);
}

void InitProofShare(Tcc25ProofShare &proof)
{
    ep_null(proof.a_g1);
    ep_new(proof.a_g1);
    ep2_null(proof.b_g2);
    ep2_new(proof.b_g2);
    ep_null(proof.c_g1);
    ep_new(proof.c_g1);
}

void InitProof(Tcc25Proof &proof)
{
    ep_null(proof.a_g1);
    ep_new(proof.a_g1);
    ep2_null(proof.b_g2);
    ep2_new(proof.b_g2);
    ep_null(proof.c_g1);
    ep_new(proof.c_g1);
}

Tcc25Term Term(int index, long coeff)
{
    return Tcc25Term{index, ZZ(coeff)};
}

int PushConstraint(Tcc25RmsR1cs &r1cs, int &next_witness,
                   const vector<Tcc25Term> &a_in, const vector<Tcc25Term> &b_wit)
{
    const int out = next_witness++;
    const int index = static_cast<int>(r1cs.constraints.size());
    r1cs.constraints.push_back(Tcc25Constraint{index, a_in, b_wit, out});
    r1cs.execution_order.push_back(index);
    return out;
}

int MaterializeLinComb(Tcc25RmsR1cs &r1cs, int &next_witness, const WitnessLinComb &lc)
{
    if (lc.IsZero())
    {
        return PushConstraint(r1cs, next_witness, vector<Tcc25Term>{Term(0, 0)},
                              vector<Tcc25Term>{Term(1, 1)});
    }
    if (lc.terms.size() == 1)
    {
        map<int, ZZ>::const_iterator it = lc.terms.begin();
        if (it->second == 1)
        {
            return it->first;
        }
    }
    return PushConstraint(r1cs, next_witness, vector<Tcc25Term>{Term(0, 1)}, lc.ToTerms());
}

Tcc25RmsR1cs BuildPdR1cs(int msg_num, int degree_f)
{
    Tcc25RmsR1cs r1cs;
    r1cs.num_inputs = msg_num + 1; // x0 is public 1; x1..x_msg_num are private.
    r1cs.num_witnesses = 1;
    r1cs.output_witness = 1;

    int next_witness = 2;
    vector<WitnessLinComb> dp_prev(degree_f + 1), dp_curr(degree_f + 1);
    dp_prev[0] = WitnessLinComb::FromWitness(1, ZZ(1));
    for (int s = 1; s <= degree_f; ++s)
    {
        dp_prev[s] = WitnessLinComb::Zero();
    }

    for (int input_idx = 1; input_idx <= msg_num; ++input_idx)
    {
        dp_curr[0] = WitnessLinComb::FromWitness(1, ZZ(1));
        for (int s = 1; s <= degree_f; ++s)
        {
            const int product_witness = PushConstraint(
                r1cs, next_witness, vector<Tcc25Term>{Term(input_idx, 1)},
                dp_curr[s - 1].ToTerms());
            dp_curr[s] = dp_prev[s];
            dp_curr[s].AddTerm(product_witness, ZZ(1));
        }
        dp_prev.swap(dp_curr);
        for (int s = 0; s <= degree_f; ++s)
        {
            dp_curr[s] = WitnessLinComb::Zero();
        }
    }

    WitnessLinComb output_lc = WitnessLinComb::Zero();
    for (int s = 1; s <= degree_f; ++s)
    {
        output_lc.AddAssign(dp_prev[s]);
    }
    r1cs.output_witness = MaterializeLinComb(r1cs, next_witness, output_lc);
    r1cs.num_witnesses = next_witness - 1;
    return r1cs;
}

Tcc25SparseQap BuildSparseQap(const Tcc25RmsR1cs &r1cs, const ZZ &q)
{
    Tcc25SparseQap qap;
    const int m = static_cast<int>(r1cs.constraints.size());
    qap.base_points.reserve(m);
    qap.h_points.reserve(m);
    for (int i = 0; i < m; ++i)
    {
        qap.base_points.push_back(ModQ(ZZ(i + 1), q));
        qap.h_points.push_back(ModQ(ZZ(m + i + 1), q));
    }

    qap.a_in_columns.assign(r1cs.num_inputs, vector<pair<int, ZZ>>());
    qap.b_wit_columns.assign(r1cs.num_witnesses, vector<pair<int, ZZ>>());
    qap.output_witness_by_row.reserve(m);

    for (size_t row = 0; row < r1cs.constraints.size(); ++row)
    {
        const Tcc25Constraint &cons = r1cs.constraints[row];
        for (size_t i = 0; i < cons.a_in.size(); ++i)
        {
            qap.a_in_columns[cons.a_in[i].index].push_back(
                make_pair(static_cast<int>(row), ModQ(cons.a_in[i].coeff, q)));
        }
        for (size_t i = 0; i < cons.b_wit.size(); ++i)
        {
            qap.b_wit_columns[cons.b_wit[i].index - 1].push_back(
                make_pair(static_cast<int>(row), ModQ(cons.b_wit[i].coeff, q)));
        }
        qap.output_witness_by_row.push_back(cons.output_witness - 1);
    }
    return qap;
}

ZZ LagrangeAt(const vector<ZZ> &points, int row, const ZZ &x, const ZZ &q)
{
    ZZ numerator(1);
    ZZ denominator(1);
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (static_cast<int>(i) == row)
        {
            continue;
        }
        numerator = MulQ(numerator, SubQ(x, points[i], q), q);
        denominator = MulQ(denominator, SubQ(points[row], points[i], q), q);
    }
    return MulQ(numerator, InvQ(denominator, q), q);
}

ZZ VanishingAt(const vector<ZZ> &points, const ZZ &x, const ZZ &q)
{
    ZZ out(1);
    for (size_t i = 0; i < points.size(); ++i)
    {
        out = MulQ(out, SubQ(x, points[i], q), q);
    }
    return out;
}

ZZ SparseColumnValueAt(const vector<pair<int, ZZ>> &column, const vector<ZZ> &base_points,
                       const ZZ &point, const ZZ &q)
{
    ZZ acc(0);
    for (size_t i = 0; i < column.size(); ++i)
    {
        const ZZ l = LagrangeAt(base_points, column[i].first, point, q);
        acc = AddQ(acc, MulQ(column[i].second, l, q), q);
    }
    return acc;
}

void EvaluateQapAtTau(const Tcc25SparseQap &qap, const ZZ &tau, const ZZ &q,
                      vector<ZZ> &u_in, vector<ZZ> &v_wit, vector<ZZ> &w_wit)
{
    u_in.assign(qap.a_in_columns.size(), ZZ(0));
    v_wit.assign(qap.b_wit_columns.size(), ZZ(0));
    w_wit.assign(qap.b_wit_columns.size(), ZZ(0));

    for (size_t input_idx = 0; input_idx < qap.a_in_columns.size(); ++input_idx)
    {
        u_in[input_idx] = SparseColumnValueAt(qap.a_in_columns[input_idx], qap.base_points, tau, q);
    }
    for (size_t wit_idx = 0; wit_idx < qap.b_wit_columns.size(); ++wit_idx)
    {
        v_wit[wit_idx] = SparseColumnValueAt(qap.b_wit_columns[wit_idx], qap.base_points, tau, q);
    }
    for (size_t row = 0; row < qap.output_witness_by_row.size(); ++row)
    {
        const int wit_idx = qap.output_witness_by_row[row];
        const ZZ l = LagrangeAt(qap.base_points, static_cast<int>(row), tau, q);
        w_wit[wit_idx] = AddQ(w_wit[wit_idx], l, q);
    }
}

void HssLoadConstant(Tcc25Memory &out, int server_id, const Tcc25EvalKey &ek, const ZZ &value)
{
    out[0] = value * server_id;
    out[1] = value * ek;
}

void HssZeroMemory(Tcc25Memory &out)
{
    out[0] = ZZ(0);
    out[1] = ZZ(0);
}

void HssCmulMemory(Tcc25Memory &out, const Tcc25Memory &in, const ZZ &scalar)
{
    out[0] = in[0] * scalar;
    out[1] = in[1] * scalar;
}

void AddMemory(Tcc25Memory &out, const pvhss::group::hss::HssPublicKey &pk,
               const Tcc25Memory &left, const Tcc25Memory &right)
{
    HssAddMemory(out, pk, left, right);
}

void HssIdentityCt(Tcc25Ciphertext &ct)
{
    ct[0][0] = ZZ(1);
    ct[0][1] = ZZ(1);
    ct[1][0] = ZZ(1);
    ct[1][1] = ZZ(1);
}

void PowerModSigned(ZZ &out, const ZZ &base, const ZZ &exp, const ZZ &modulus)
{
    if (exp < 0)
    {
        ZZ inv;
        InvMod(inv, ModQ(base, modulus), modulus);
        PowerMod(out, inv, -exp, modulus);
    }
    else
    {
        PowerMod(out, base, exp, modulus);
    }
}

void AccumulateScaledCiphertext(const pvhss::group::hss::HssPublicKey &pk,
                                Tcc25Ciphertext &acc, const Tcc25Ciphertext &ct,
                                const ZZ &scalar)
{
    if (scalar == 0)
    {
        return;
    }
    ZZ factor;
    for (int lane = 0; lane < 2; ++lane)
    {
        for (int comp = 0; comp < 2; ++comp)
        {
            PowerModSigned(factor, ct[lane][comp], scalar, pk.N2);
            MulMod(acc[lane][comp], acc[lane][comp], factor, pk.N2);
        }
    }
}

Tcc25Memory WitnessLinearCombo(const vector<Tcc25Memory> &witness,
                               const vector<Tcc25Term> &terms,
                               const pvhss::group::hss::HssPublicKey &pk,
                               const ZZ &q)
{
    Tcc25Memory acc;
    HssZeroMemory(acc);
    for (size_t i = 0; i < terms.size(); ++i)
    {
        const ZZ coeff = CenteredQ(terms[i].coeff, q);
        if (coeff == 0)
        {
            continue;
        }
        Tcc25Memory scaled;
        HssCmulMemory(scaled, witness[terms[i].index], coeff);
        Tcc25Memory next;
        AddMemory(next, pk, acc, scaled);
        acc = next;
    }
    return acc;
}

Tcc25Memory InputComboProduct(const Tcc25Param &param, int server_id,
                              const vector<Tcc25Ciphertext> &Ix,
                              const vector<Tcc25Term> &terms, const Tcc25Memory &mem,
                              int &prf_key)
{
    Tcc25Ciphertext private_combo;
    HssIdentityCt(private_combo);
    bool has_private = false;
    ZZ public_scalar(0);

    for (size_t i = 0; i < terms.size(); ++i)
    {
        const ZZ coeff = CenteredQ(terms[i].coeff, param.order_ZZ);
        if (coeff == 0)
        {
            continue;
        }

        if (terms[i].index == 0)
        {
            public_scalar += coeff;
        }
        else
        {
            AccumulateScaledCiphertext(param.pk, private_combo, Ix[terms[i].index - 1], coeff);
            has_private = true;
        }
    }

    Tcc25Memory acc;
    HssZeroMemory(acc);
    if (has_private)
    {
        HssMul(acc, server_id, param.pk, private_combo, mem, prf_key);
    }
    if (public_scalar != 0)
    {
        Tcc25Memory public_term;
        HssCmulMemory(public_term, mem, public_scalar);
        Tcc25Memory next;
        AddMemory(next, param.pk, acc, public_term);
        acc = next;
    }
    return acc;
}

void EvaluateRmsWitness(vector<Tcc25Memory> &input_mem, vector<Tcc25Memory> &witness_mem,
                        vector<ZZ> &input_shares, vector<ZZ> &witness_shares,
                        const Tcc25Param &param, int server_id,
                        const Tcc25EvalKey &ek, const vector<Tcc25Ciphertext> &Ix,
                        int &prf_key)
{
    input_mem.assign(param.r1cs.num_inputs, Tcc25Memory());
    HssLoadConstant(input_mem[0], server_id, ek, ZZ(1));
    for (int i = 1; i < param.r1cs.num_inputs; ++i)
    {
        HssConvertInput(input_mem[i], server_id, param.pk, ek, Ix[i - 1], prf_key);
    }

    input_shares.assign(param.r1cs.num_inputs, ZZ(0));
    for (int i = 0; i < param.r1cs.num_inputs; ++i)
    {
        input_shares[i] = ModQ(input_mem[i][0], param.order_ZZ);
    }

    witness_mem.assign(param.r1cs.num_witnesses + 1, Tcc25Memory());
    HssLoadConstant(witness_mem[1], server_id, ek, ZZ(1));

    for (size_t i = 0; i < param.r1cs.execution_order.size(); ++i)
    {
        const Tcc25Constraint &cons = param.r1cs.constraints[param.r1cs.execution_order[i]];
        const Tcc25Memory b_mem = WitnessLinearCombo(witness_mem, cons.b_wit, param.pk,
                                                     param.order_ZZ);
        witness_mem[cons.output_witness] = InputComboProduct(
            param, server_id, Ix, cons.a_in, b_mem, prf_key);
    }

    witness_shares.assign(param.r1cs.num_witnesses, ZZ(0));
    for (int i = 1; i <= param.r1cs.num_witnesses; ++i)
    {
        witness_shares[i - 1] = ModQ(witness_mem[i][0], param.order_ZZ);
    }
}

void EvaluateQapH(vector<ZZ> &h_shares, const Tcc25Param &param, int server_id,
                  const vector<Tcc25Ciphertext> &Ix,
                  const vector<Tcc25Memory> &witness_mem,
                  const vector<ZZ> &witness_shares, int &prf_key)
{
    h_shares.assign(param.qap.h_points.size(), ZZ(0));
    for (size_t point_idx = 0; point_idx < param.qap.h_points.size(); ++point_idx)
    {
        const ZZ &point = param.qap.h_points[point_idx];

        Tcc25Memory b_mem;
        HssZeroMemory(b_mem);
        ZZ b_share(0);
        for (size_t wit_idx = 0; wit_idx < param.qap.b_wit_columns.size(); ++wit_idx)
        {
            const ZZ coeff = SparseColumnValueAt(param.qap.b_wit_columns[wit_idx],
                                                 param.qap.base_points, point, param.order_ZZ);
            if (coeff == 0)
            {
                continue;
            }
            Tcc25Memory scaled;
            HssCmulMemory(scaled, witness_mem[wit_idx + 1], CenteredQ(coeff, param.order_ZZ));
            Tcc25Memory next;
            AddMemory(next, param.pk, b_mem, scaled);
            b_mem = next;
            b_share = AddQ(b_share, MulQ(coeff, witness_shares[wit_idx], param.order_ZZ),
                           param.order_ZZ);
        }

        Tcc25Ciphertext private_combo;
        HssIdentityCt(private_combo);
        bool has_private = false;
        ZZ public_a(0);
        for (size_t input_idx = 0; input_idx < param.qap.a_in_columns.size(); ++input_idx)
        {
            const ZZ coeff = SparseColumnValueAt(param.qap.a_in_columns[input_idx],
                                                 param.qap.base_points, point, param.order_ZZ);
            if (coeff == 0)
            {
                continue;
            }
            if (input_idx == 0)
            {
                public_a = AddQ(public_a, coeff, param.order_ZZ);
            }
            else
            {
                AccumulateScaledCiphertext(param.pk, private_combo, Ix[input_idx - 1],
                                           CenteredQ(coeff, param.order_ZZ));
                has_private = true;
            }
        }

        ZZ ab_share(0);
        if (has_private)
        {
            Tcc25Memory ab_mem;
            HssMul(ab_mem, server_id, param.pk, private_combo, b_mem, prf_key);
            ab_share = ModQ(ab_mem[0], param.order_ZZ);
        }
        const ZZ public_ab = MulQ(public_a, b_share, param.order_ZZ);

        ZZ c_share(0);
        for (size_t row = 0; row < param.qap.output_witness_by_row.size(); ++row)
        {
            const int wit_idx = param.qap.output_witness_by_row[row];
            const ZZ l = LagrangeAt(param.qap.base_points, static_cast<int>(row), point,
                                    param.order_ZZ);
            c_share = AddQ(c_share, MulQ(l, witness_shares[wit_idx], param.order_ZZ),
                           param.order_ZZ);
        }

        const ZZ numerator = SubQ(AddQ(ab_share, public_ab, param.order_ZZ), c_share,
                                  param.order_ZZ);
        const ZZ t_point = VanishingAt(param.qap.base_points, point, param.order_ZZ);
        h_shares[point_idx] = MulQ(numerator, InvQ(t_point, param.order_ZZ), param.order_ZZ);
    }
}

ZZ DotScalars(const vector<ZZ> &left, const vector<ZZ> &right, const ZZ &q)
{
    ZZ acc(0);
    const size_t n = min(left.size(), right.size());
    for (size_t i = 0; i < n; ++i)
    {
        acc = AddQ(acc, MulQ(left[i], right[i], q), q);
    }
    return acc;
}

struct Tcc25Blinders
{
    ZZ r_share;
    ZZ s_share;
    ZZ r;
    ZZ s;
    ZZ rs_share;
};

Tcc25Blinders DeriveBlinders(int server_id, const ZZ &q)
{
    const ZZ r0 = PrfZZ(2000001, q);
    const ZZ r1 = PrfZZ(2000002, q);
    const ZZ s0 = PrfZZ(2000003, q);
    const ZZ s1 = PrfZZ(2000004, q);
    Tcc25Blinders blinders;
    blinders.r = SubQ(r1, r0, q);
    blinders.s = SubQ(s1, s0, q);
    blinders.r_share = server_id == 0 ? r0 : r1;
    blinders.s_share = server_id == 0 ? s0 : s1;
    blinders.rs_share = MulQ(blinders.r, blinders.s_share, q);
    return blinders;
}

void FinalizeProofShare(Tcc25ProofShare &proof, int server_id, const Tcc25Param &param,
                        const vector<ZZ> &input_shares,
                        const vector<ZZ> &witness_shares,
                        const vector<ZZ> &h_shares)
{
    const Tcc25Blinders blinders = DeriveBlinders(server_id, param.order_ZZ);

    const ZZ dot_a = DotScalars(input_shares, param.a_query_scalars, param.order_ZZ);
    const ZZ dot_b = DotScalars(witness_shares, param.b_query_scalars, param.order_ZZ);

    ZZ a_scalar = AddQ(dot_a, MulQ(param.delta, blinders.r_share, param.order_ZZ),
                       param.order_ZZ);
    if (server_id == 1)
    {
        a_scalar = AddQ(a_scalar, param.alpha, param.order_ZZ);
    }

    ZZ b_scalar = AddQ(dot_b, MulQ(param.delta, blinders.s_share, param.order_ZZ),
                       param.order_ZZ);
    if (server_id == 1)
    {
        b_scalar = AddQ(b_scalar, param.beta, param.order_ZZ);
    }

    vector<ZZ> hidden_terms = witness_shares;
    for (size_t i = 0; i < hidden_terms.size(); ++i)
    {
        if (!param.hidden_present[i])
        {
            hidden_terms[i] = ZZ(0);
        }
    }
    const ZZ hidden_scalar = DotScalars(hidden_terms, param.hidden_over_delta_scalars,
                                        param.order_ZZ);
    const ZZ h_scalar = DotScalars(h_shares, param.h_lagrange_over_delta_scalars,
                                   param.order_ZZ);

    ZZ c_scalar = AddQ(hidden_scalar, h_scalar, param.order_ZZ);
    c_scalar = AddQ(c_scalar, MulQ(param.alpha, blinders.s_share, param.order_ZZ),
                    param.order_ZZ);
    c_scalar = AddQ(c_scalar, MulQ(param.beta, blinders.r_share, param.order_ZZ),
                    param.order_ZZ);
    c_scalar = AddQ(c_scalar, MulQ(param.delta, blinders.rs_share, param.order_ZZ),
                    param.order_ZZ);
    c_scalar = AddQ(c_scalar, MulQ(blinders.s, dot_a, param.order_ZZ), param.order_ZZ);
    c_scalar = AddQ(c_scalar, MulQ(blinders.r, dot_b, param.order_ZZ), param.order_ZZ);

    InitProofShare(proof);
    G1MulGenZZ(proof.a_g1, a_scalar, param.order_ZZ);
    G2MulGenZZ(proof.b_g2, b_scalar, param.order_ZZ);
    G1MulGenZZ(proof.c_g1, c_scalar, param.order_ZZ);

    proof.y_share.assign(1, witness_shares[param.r1cs.output_witness - 1]);
}

bool PairingCheck(const Tcc25Proof &proof, const Tcc25Param &param,
                  const vector<ZZ> &public_inputs, const vector<ZZ> &claimed_outputs)
{
    ZZ commitment_scalar(0);
    for (size_t i = 0; i < public_inputs.size(); ++i)
    {
        commitment_scalar = AddQ(
            commitment_scalar,
            MulQ(public_inputs[i], param.public_u_scalars[i], param.order_ZZ),
            param.order_ZZ);
    }
    for (size_t i = 0; i < claimed_outputs.size(); ++i)
    {
        commitment_scalar = AddQ(
            commitment_scalar,
            MulQ(claimed_outputs[i], param.output_u_scalars[i], param.order_ZZ),
            param.order_ZZ);
    }

    ep_t commitment, neg_alpha, neg_c, neg_commitment;
    ep_null(commitment);
    ep_null(neg_alpha);
    ep_null(neg_c);
    ep_null(neg_commitment);
    ep_new(commitment);
    ep_new(neg_alpha);
    ep_new(neg_c);
    ep_new(neg_commitment);
    G1MulGenZZ(commitment, commitment_scalar, param.order_ZZ);
    ep_neg(neg_alpha, param.alpha_g1);
    ep_neg(neg_c, proof.c_g1);
    ep_neg(neg_commitment, commitment);

    fp12_t lhs, term;
    fp12_null(lhs);
    fp12_null(term);
    fp12_new(lhs);
    fp12_new(term);
    pp_map_oatep_k12(lhs, proof.a_g1, proof.b_g2);

    pp_map_oatep_k12(term, neg_alpha, param.beta_g2);
    fp12_mul(lhs, lhs, term);
    pp_map_oatep_k12(term, neg_c, param.delta_g2);
    fp12_mul(lhs, lhs, term);
    pp_map_oatep_k12(term, neg_commitment, param.g2);
    fp12_mul(lhs, lhs, term);

    return gt_is_unity(lhs) == 1;
}

} // namespace

void Tcc25_Setup(Tcc25Param &param, Tcc25Server &server0, Tcc25Server &server1)
{
    if (param.skLen <= 0)
    {
        param.skLen = 1024;
    }
    if (param.msg_bits <= 0)
    {
        param.msg_bits = 32;
    }

    InitPairing();
    bn_null(param.order);
    bn_new(param.order);
    ep_curve_get_ord(param.order);
    BnToZZ(param.order_ZZ, param.order);

    param.r1cs = BuildPdR1cs(param.msg_num, param.degree_f);
    param.qap = BuildSparseQap(param.r1cs, param.order_ZZ);

    pvhss::group::hss::HssGen(param.pk, server0.ek, server1.ek, param.skLen);
    server0.id = 0;
    server1.id = 1;

    do
    {
        RandomBnd(param.tau, param.order_ZZ);
    } while (VanishingAt(param.qap.base_points, param.tau, param.order_ZZ) == 0);
    param.alpha = RandomNonZeroScalar(param.order_ZZ);
    param.beta = RandomNonZeroScalar(param.order_ZZ);
    param.delta = RandomNonZeroScalar(param.order_ZZ);
    const ZZ delta_inv = InvQ(param.delta, param.order_ZZ);

    vector<ZZ> u_in, v_wit, w_wit;
    EvaluateQapAtTau(param.qap, param.tau, param.order_ZZ, u_in, v_wit, w_wit);
    const ZZ t_tau = VanishingAt(param.qap.base_points, param.tau, param.order_ZZ);

    param.a_query_scalars = u_in;
    param.b_query_scalars = v_wit;
    param.public_u_scalars.assign(u_in.size(), ZZ(0));
    for (size_t i = 0; i < u_in.size(); ++i)
    {
        param.public_u_scalars[i] = MulQ(param.beta, u_in[i], param.order_ZZ);
    }

    param.output_u_scalars.assign(1, ZZ(0));
    const int output_idx = param.r1cs.output_witness - 1;
    param.output_u_scalars[0] = AddQ(MulQ(param.alpha, v_wit[output_idx], param.order_ZZ),
                                     w_wit[output_idx], param.order_ZZ);

    param.hidden_over_delta_scalars.assign(v_wit.size(), ZZ(0));
    param.hidden_present.assign(v_wit.size(), true);
    param.hidden_present[output_idx] = false;
    for (size_t i = 0; i < v_wit.size(); ++i)
    {
        if (param.hidden_present[i])
        {
            const ZZ scalar = AddQ(MulQ(param.alpha, v_wit[i], param.order_ZZ), w_wit[i],
                                   param.order_ZZ);
            param.hidden_over_delta_scalars[i] = MulQ(scalar, delta_inv, param.order_ZZ);
        }
    }

    param.h_lagrange_over_delta_scalars.assign(param.qap.h_points.size(), ZZ(0));
    for (size_t i = 0; i < param.qap.h_points.size(); ++i)
    {
        const ZZ l_h = LagrangeAt(param.qap.h_points, static_cast<int>(i), param.tau,
                                  param.order_ZZ);
        param.h_lagrange_over_delta_scalars[i] = MulQ(MulQ(l_h, t_tau, param.order_ZZ),
                                                      delta_inv, param.order_ZZ);
    }

    ep_null(param.alpha_g1);
    ep_new(param.alpha_g1);
    ep_null(param.beta_g1);
    ep_new(param.beta_g1);
    ep_null(param.delta_g1);
    ep_new(param.delta_g1);
    ep2_null(param.beta_g2);
    ep2_new(param.beta_g2);
    ep2_null(param.delta_g2);
    ep2_new(param.delta_g2);
    ep2_null(param.g2);
    ep2_new(param.g2);

    G1MulGenZZ(param.alpha_g1, param.alpha, param.order_ZZ);
    G1MulGenZZ(param.beta_g1, param.beta, param.order_ZZ);
    G1MulGenZZ(param.delta_g1, param.delta, param.order_ZZ);
    G2MulGenZZ(param.beta_g2, param.beta, param.order_ZZ);
    G2MulGenZZ(param.delta_g2, param.delta, param.order_ZZ);
    ep2_curve_get_gen(param.g2);
}

void Tcc25_ProbGen(vector<Tcc25Ciphertext> &Ix, const Tcc25Param &param, const vec_ZZ &x)
{
    Ix.clear();
    Tcc25Ciphertext ct;
    for (int i = 0; i < x.length(); ++i)
    {
        HssInput(ct, param.pk, x[i]);
        Ix.push_back(ct);
    }
}

void Tcc25_Compute(Tcc25ProofShare &proof, int server_id, const Tcc25Param &param,
                   const Tcc25Server &server, const vector<Tcc25Ciphertext> &Ix)
{
    vector<Tcc25Memory> input_mem, witness_mem;
    vector<ZZ> input_shares, witness_shares, h_shares;
    int prf_key = 0;
    EvaluateRmsWitness(input_mem, witness_mem, input_shares, witness_shares, param, server_id,
                       server.ek, Ix, prf_key);
    EvaluateQapH(h_shares, param, server_id, Ix, witness_mem, witness_shares, prf_key);
    FinalizeProofShare(proof, server_id, param, input_shares, witness_shares, h_shares);
}

bool Tcc25_Verify(Tcc25Proof &proof, const Tcc25ProofShare &pi0,
                  const Tcc25ProofShare &pi1, const Tcc25Param &param, const vec_ZZ &x)
{
    if (pi0.y_share.size() != pi1.y_share.size())
    {
        return false;
    }

    InitProof(proof);
    proof.y.assign(pi0.y_share.size(), ZZ(0));
    for (size_t i = 0; i < proof.y.size(); ++i)
    {
        proof.y[i] = SubQ(pi1.y_share[i], pi0.y_share[i], param.order_ZZ);
    }

    ep_sub(proof.a_g1, pi1.a_g1, pi0.a_g1);
    ep_norm(proof.a_g1, proof.a_g1);
    ep2_sub(proof.b_g2, pi1.b_g2, pi0.b_g2);
    ep2_norm(proof.b_g2, proof.b_g2);
    ep_sub(proof.c_g1, pi1.c_g1, pi0.c_g1);
    ep_norm(proof.c_g1, proof.c_g1);

    vector<ZZ> full_inputs(param.r1cs.num_inputs, ZZ(0));
    full_inputs[0] = ZZ(1);
    for (int i = 1; i < param.r1cs.num_inputs; ++i)
    {
        full_inputs[i] = ModQ(x[i - 1], param.order_ZZ);
    }

    return PairingCheck(proof, param, full_inputs, proof.y);
}

void Tcc25_Decode(ZZ &y, const Tcc25ProofShare &pi0, const Tcc25ProofShare &pi1,
                  const Tcc25Param &param)
{
    if (pi0.y_share.empty() || pi1.y_share.empty())
    {
        y = ZZ(0);
        return;
    }
    y = SubQ(pi1.y_share[0], pi0.y_share[0], param.order_ZZ);
}

bool Tcc25_ACC_TEST(int msg_num, int degree_f)
{
    Tcc25Param param;
    Tcc25Server server0, server1;
    param.skLen = 1024;
    param.msg_bits = 32;
    param.degree_f = degree_f;
    param.msg_num = msg_num;

    TimingResult timing = MeasureTimeMs([&]() {
        Tcc25_Setup(param, server0, server1);
    }, 1);
    PrintTimeMs("Setup algorithm time", timing);

    vec_ZZ X;
    X.SetLength(msg_num);
    for (int i = 0; i < msg_num; ++i)
    {
        RandomBits(X[i], param.msg_bits);
    }

    vector<Tcc25Ciphertext> Ix;
    timing = MeasureTimeMs([&]() {
        Tcc25_ProbGen(Ix, param, X);
    }, 1);
    PrintTimeMs("Input algorithm time", timing);

    Tcc25ProofShare pi0, pi1;
    timing = MeasureTimeMs([&]() {
        Tcc25_Compute(pi0, 0, param, server0, Ix);
    }, 1);
    PrintTimeMs("Evaluation 0 algorithm time", timing);
    timing = MeasureTimeMs([&]() {
        Tcc25_Compute(pi1, 1, param, server1, Ix);
    }, 1);
    PrintTimeMs("Evaluation 1 algorithm time", timing);

    Tcc25Proof proof;
    bool verified = false;
    timing = MeasureTimeMsAdaptive([&]() {
        verified = Tcc25_Verify(proof, pi0, pi1, param, X);
    }, 1);
    PrintTimeMs("Verification algorithm time", timing);

    ZZ decoded;
    timing = MeasureTimeMsAdaptive([&]() {
        Tcc25_Decode(decoded, pi0, pi1, param);
    }, 1);
    PrintTimeMs("Decryption algorithm time", timing);

    ZZ expected = ModQ(PolyD2(X, degree_f), param.order_ZZ);
    cout << "True result: " << expected << endl;
    cout << "Eval result: " << decoded << endl;
    if (verified)
    {
        printf("****************** Verification Passed ******************\n");
    }
    else
    {
        printf("****************** Verification Failed ********************\n");
    }

    const bool result_matches = (expected == decoded);
    if (!result_matches)
    {
        cout << "P5 accuracy check failed: decoded result differs from native computation." << endl;
    }
    core_clean();
    return verified && result_matches;
}

}}} // namespace pvhss::group::tcc25
