#include "PiOTGroup.h"
#include "scheme_bench_runner.h"
#include "scheme_group_ot.h"
#include "helper.h"

#include <NTL/ZZ.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;
using namespace pvhss;

namespace
{
struct PirConfig
{
    size_t db_size = 16;
    int cyctimes = 1;
    int record_bits = 16;
    int sk_len = 1024;
    int vk_len = 256;
    bool has_index = false;
    size_t index = 0;
    bool tamper = false;
    bool verbose = false;
    string seed;
};

bool IsPowerOfTwo(size_t x)
{
    return x != 0 && (x & (x - 1)) == 0;
}

int Log2Exact(size_t x)
{
    int logn = 0;
    while (x > 1)
    {
        x >>= 1;
        ++logn;
    }
    return logn;
}

vector<NTL::ZZ> BuildIndexBits(size_t index, int logn)
{
    vector<NTL::ZZ> bits(static_cast<size_t>(logn));
    for (int depth = 0; depth < logn; ++depth)
    {
        const int shift = logn - 1 - depth;
        bits[static_cast<size_t>(depth)] = NTL::ZZ((index >> shift) & size_t{1});
    }
    return bits;
}

vector<NTL::ZZ> RandomDatabase(size_t db_size, int record_bits)
{
    vector<NTL::ZZ> database(db_size);
    for (size_t i = 0; i < db_size; ++i)
    {
        NTL::RandomBits(database[i], record_bits);
    }
    return database;
}

size_t RandomIndex(size_t db_size)
{
    NTL::ZZ sampled;
    NTL::RandomBnd(sampled, NTL::conv<NTL::ZZ>(db_size));
    return static_cast<size_t>(NTL::conv<long>(sampled));
}

size_t EpCompressedBytes(const ep_t point)
{
    ep_t normalized;
    ep_null(normalized);
    ep_new(normalized);
    ep_norm(normalized, point);
    const int len = ep_size_bin(normalized, 1);
    ep_free(normalized);
    return static_cast<size_t>(len);
}

size_t ProofCompressedBits(const pvhss::group::ot::PROOF &proof,
                           const NTL::ZZ &group_order)
{
    return 8 * (static_cast<size_t>(NTL::NumBytes(group_order)) +
                EpCompressedBytes(proof.D));
}

void CopyServerProofKey(const scheme::SchemeGroupOt::SetupOutput &pp, int sid,
                        bn_t ekpb)
{
    bn_new(ekpb);
    if (sid == 0)
    {
        bn_copy(ekpb, pp.ekp0);
    }
    else
    {
        bn_copy(ekpb, pp.ekp1);
    }
}

scheme::SchemeGroupOt::ServerOutput AnswerPIR(
    const scheme::SchemeGroupOt::SetupOutput &pp,
    const scheme::SchemeGroupOt::ProbGenOutput &query,
    const vector<NTL::ZZ> &database,
    int sid)
{
    scheme::SchemeGroupOt::ServerOutput out;
    bn_t ekpb;
    CopyServerProofKey(pp, sid, ekpb);

    int prf_key = 0;
    VhssElgamalMv y_b_res;
    const VhssElgamalEk &ekb = (sid == 0) ? pp.ek0 : pp.ek1;

    const auto eval = bench::MeasureOnce([&]() {
        VhssElgamalEvaluatePirSelection(y_b_res, sid, query.Ix, database,
                                        pp.param.pk, ekb, prf_key);
    });
    const auto proof = bench::MeasureOnce([&]() {
        VhssElgamalMv sk_b;
        VhssElgamalConvertInput(sk_b, sid, pp.param.pk, ekb,
                                pp.param.pk_f, prf_key);
        y_b_res[0] = y_b_res[0] + sk_b[0];
        y_b_res[2] = y_b_res[2] + sk_b[2];
        pvhss::group::ot::Ped_Prove(out.proof, sid, y_b_res[0],
                                    y_b_res[2], pp.param.ck, prf_key, ekpb);
    });

    out.profile.push_back({"Answer" + to_string(sid) + "Eval", eval});
    out.profile.push_back({"Answer" + to_string(sid) + "Proof", proof});
    return out;
}

void TamperProof(pvhss::group::ot::PROOF &proof)
{
    proof.y += 1;
}

void PrintUsage(const char *argv0)
{
    cout << "Usage: " << argv0 << " [options]\n"
         << "  --db-size N       power-of-two database size (default 16)\n"
         << "  --index I         zero-based queried index (default random)\n"
         << "  --record-bits B   random DB record bit length, 1..32 (default 16)\n"
         << "  --cyctimes C      timing samples per phase (default 1)\n"
         << "  --seed S          deterministic benchmark seed\n"
         << "  --sk-len B        HSS secret-key bit parameter (default 1024)\n"
         << "  --vk-len B        HSS verification-key bit parameter (default 256)\n"
         << "  --tamper          also check that a modified answer is rejected\n"
         << "  --verbose         print the queried DB value\n";
}

PirConfig ParseArgs(int argc, char **argv)
{
    PirConfig cfg;
    for (int i = 1; i < argc; ++i)
    {
        const string arg(argv[i]);
        if (arg == "--db-size" && i + 1 < argc)
        {
            cfg.db_size = static_cast<size_t>(stoull(argv[++i]));
        }
        else if (arg == "--index" && i + 1 < argc)
        {
            cfg.index = static_cast<size_t>(stoull(argv[++i]));
            cfg.has_index = true;
        }
        else if (arg == "--record-bits" && i + 1 < argc)
        {
            cfg.record_bits = atoi(argv[++i]);
        }
        else if (arg == "--cyctimes" && i + 1 < argc)
        {
            cfg.cyctimes = atoi(argv[++i]);
        }
        else if (arg == "--seed" && i + 1 < argc)
        {
            cfg.seed = argv[++i];
        }
        else if (arg == "--sk-len" && i + 1 < argc)
        {
            cfg.sk_len = atoi(argv[++i]);
        }
        else if (arg == "--vk-len" && i + 1 < argc)
        {
            cfg.vk_len = atoi(argv[++i]);
        }
        else if (arg == "--tamper")
        {
            cfg.tamper = true;
        }
        else if (arg == "--verbose")
        {
            cfg.verbose = true;
        }
        else if (arg == "--help" || arg == "-h")
        {
            PrintUsage(argv[0]);
            exit(0);
        }
        else
        {
            throw invalid_argument("unknown or incomplete option: " + arg);
        }
    }
    return cfg;
}
}

int main(int argc, char **argv)
{
    setbuf(stdout, NULL);

    try
    {
        PirConfig pcfg = ParseArgs(argc, argv);
        if (!IsPowerOfTwo(pcfg.db_size))
        {
            throw invalid_argument("--db-size must be a non-zero power of two");
        }
        if (pcfg.record_bits < 1 || pcfg.record_bits > 32)
        {
            throw invalid_argument("--record-bits must be in the range 1..32");
        }
        if (pcfg.cyctimes < 1)
        {
            pcfg.cyctimes = 1;
        }
        if (pcfg.has_index && pcfg.index >= pcfg.db_size)
        {
            throw invalid_argument("--index must be smaller than --db-size");
        }

        const char *env_seed = getenv("PVHSS_BENCH_SEED");
        const string configured_seed =
            !pcfg.seed.empty() ? pcfg.seed : (env_seed ? string(env_seed) : string());
        ConfigureBenchmarkRandomness(configured_seed);
        SeedBenchmarkRandomness("PIRInputs");

        const int logn = Log2Exact(pcfg.db_size);
        vector<NTL::ZZ> database = RandomDatabase(pcfg.db_size, pcfg.record_bits);
        const size_t index = pcfg.has_index ? pcfg.index : RandomIndex(pcfg.db_size);
        vector<NTL::ZZ> index_bits = BuildIndexBits(index, logn);

        bench::BenchConfig bcfg;
        bcfg.msg_num = logn;
        bcfg.degree_f = logn;
        bcfg.msg_bits = 1;
        bcfg.sk_len = pcfg.sk_len;
        bcfg.vk_len = pcfg.vk_len;
        bcfg.cyctimes = pcfg.cyctimes;
        bcfg.random_seed = configured_seed;

        cout << "=======================================================\n";
        cout << "  Group-OT PIR Benchmark\n";
        cout << "  db_size = " << pcfg.db_size
             << "  logn = " << logn
             << "  record_bits = " << pcfg.record_bits
             << "  index = " << index
             << "  cyctimes = " << pcfg.cyctimes << "\n";
        if (BenchmarkRandomnessConfigured())
        {
            cout << "  random_seed = " << configured_seed << "\n";
        }
        cout << "-------------------------------------------------------\n";

        scheme::SchemeGroupOt::SetupOutput setup_val;
        bench::MeasureWithRecordedProfile("Setup", setup_val, [&]() {
            setup_val = scheme::SchemeGroupOt::Setup(bcfg);
        }, pcfg.cyctimes, "Setup");

        bench::Measure("Gen", [&]() {
            scheme::SchemeGroupOt::KeyGen(setup_val);
        }, pcfg.cyctimes, "Gen");

        scheme::SchemeGroupOt::ProbGenOutput query_val;
        bench::Measure("Query", [&]() {
            query_val = scheme::SchemeGroupOt::ProbGen(setup_val, index_bits);
        }, pcfg.cyctimes, "Query");

        scheme::SchemeGroupOt::ServerOutput out0_val;
        bench::MeasureWithRecordedProfile("Answer0", out0_val, [&]() {
            out0_val = AnswerPIR(setup_val, query_val, database, 0);
        }, pcfg.cyctimes, "Answer0");

        scheme::SchemeGroupOt::ServerOutput out1_val;
        bench::MeasureWithRecordedProfile("Answer1", out1_val, [&]() {
            out1_val = AnswerPIR(setup_val, query_val, database, 1);
        }, pcfg.cyctimes, "Answer1");

        scheme::SchemeGroupOt::VerifyOutput verify_val;
        bench::Measure("Verify", [&]() {
            verify_val = scheme::SchemeGroupOt::Verify(setup_val, query_val,
                                                       out0_val, out1_val);
        }, pcfg.cyctimes, "Verify");

        NTL::ZZ decoded;
        bench::Measure("Decode", [&]() {
            decoded = scheme::SchemeGroupOt::Decode(setup_val, out0_val, out1_val);
        }, pcfg.cyctimes, "Decode");

        const NTL::ZZ reference = database[index];
        const bool correct = verify_val.accepted && decoded == reference;

        if (pcfg.verbose)
        {
            cout << "  Reference value: " << reference << "\n";
            cout << "  Decoded value:   " << decoded << "\n";
        }

        bool tamper_rejected = false;
        if (pcfg.tamper)
        {
            TamperProof(out1_val.proof);
            const bool tamper_accepted =
                scheme::SchemeGroupOt::Verify(setup_val, query_val,
                                              out0_val, out1_val).accepted;
            tamper_rejected = !tamper_accepted;
        }

        const long logN_bits = NTL::NumBits(setup_val.param.pk.N);
        const long logN2_bits = NTL::NumBits(setup_val.param.pk.N2);
        const size_t query_bits_per_server =
            static_cast<size_t>(logn) * 4ULL * static_cast<size_t>(logN2_bits);
        const size_t answer0_bits =
            ProofCompressedBits(out0_val.proof, setup_val.param.ck.g1_order_ZZ);
        const size_t answer1_bits =
            ProofCompressedBits(out1_val.proof, setup_val.param.ck.g1_order_ZZ);
        const size_t total_online_bits =
            2ULL * query_bits_per_server + answer0_bits + answer1_bits;
        const size_t formula_query_bits_per_server =
            static_cast<size_t>(logn) * 8ULL * static_cast<size_t>(logN_bits);
        const size_t formula_answer_bits_per_server = answer0_bits;
        const size_t formula_total_bits =
            2ULL * formula_query_bits_per_server +
            2ULL * formula_answer_bits_per_server;

        cout << "-------------------------------------------------------\n";
        cout << "Correctness: " << (correct ? "PASS" : "FAIL") << "\n";
        if (pcfg.tamper)
        {
            cout << "TamperRejected: " << (tamper_rejected ? "PASS" : "FAIL") << "\n";
        }
        else
        {
            cout << "TamperRejected: SKIP\n";
        }
        cout << "DbSize: " << pcfg.db_size << "\n";
        cout << "LogN: " << logn << "\n";
        cout << "RecordBits: " << pcfg.record_bits << "\n";
        cout << "Index: " << index << "\n";
        cout << "QueryBitsPerServer: " << query_bits_per_server << "\n";
        cout << "Answer0Bits: " << answer0_bits << "\n";
        cout << "Answer1Bits: " << answer1_bits << "\n";
        cout << "TotalOnlineBits: " << total_online_bits << "\n";
        cout << "FormulaQueryBitsPerServer: " << formula_query_bits_per_server << "\n";
        cout << "FormulaAnswerBitsPerServer: " << formula_answer_bits_per_server << "\n";
        cout << "FormulaTotalBits: " << formula_total_bits << "\n";
        cout << "=======================================================\n";

        return correct && (!pcfg.tamper || tamper_rejected) ? 0 : 2;
    }
    catch (const exception &ex)
    {
        cerr << "pir_group_ot: " << ex.what() << "\n";
        return 1;
    }
}
