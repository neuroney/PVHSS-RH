#include "helper.h"
#include <chrono>
#include <cmath>
#include <numeric>

namespace
{
RandomStream &PRFStream()
{
    // Existing callers pass only a counter, so keep a fixed domain key and avoid global RNG reseeding.
    static const unsigned char key[32] = {
        0x50, 0x56, 0x48, 0x53, 0x53, 0x2d, 0x52, 0x48,
        0x20, 0x50, 0x52, 0x46, 0x20, 0x76, 0x31, 0x00,
        0x83, 0x9b, 0x61, 0x4f, 0xd7, 0x2c, 0x0e, 0x5a,
        0xa1, 0x37, 0xc4, 0x98, 0x6e, 0x10, 0xf2, 0xbb};
    thread_local RandomStream stream(key);
    return stream;
}

void PRFBoundedZZ(ZZ &res, int prfkey, const ZZ &mmod)
{
    if (mmod <= 1)
    {
        clear(res);
        return;
    }

    const long nbits = NumBits(mmod - 1);
    const long nbytes = (nbits + 7) / 8;
    const long excess_bits = nbytes * 8 - nbits;
    const unsigned char top_mask =
        (excess_bits == 0) ? 0xff : static_cast<unsigned char>((1u << (8 - excess_bits)) - 1u);

    thread_local vector<unsigned char> buf;
    buf.resize(nbytes);

    RandomStream &stream = PRFStream();
    stream.set_nonce(static_cast<unsigned long>(static_cast<uint32_t>(prfkey)));

    do
    {
        stream.get(buf.data(), nbytes);
        if (excess_bits != 0)
        {
            buf[nbytes - 1] &= top_mask;
        }
        ZZFromBytes(res, buf.data(), nbytes);
    } while (res >= mmod);
}
}

int int2uint8_t(uint8_t *out, int in)
{
    const uint32_t value = static_cast<uint32_t>(in);
    out[0] = static_cast<uint8_t>((value >> 24) & 0xffu);
    out[1] = static_cast<uint8_t>((value >> 16) & 0xffu);
    out[2] = static_cast<uint8_t>((value >> 8) & 0xffu);
    out[3] = static_cast<uint8_t>(value & 0xffu);
    return 0;
}

int uint8_t2int(int *out, uint8_t *in)
{
    const uint32_t value = (static_cast<uint32_t>(in[0]) << 24) |
                           (static_cast<uint32_t>(in[1]) << 16) |
                           (static_cast<uint32_t>(in[2]) << 8) |
                           static_cast<uint32_t>(in[3]);
    *out = static_cast<int>(value);

    return 0;
}

int int2bn(bn_t out, int in)
{
    uint8_t temp[4];
    int2uint8_t(temp, in);
    bn_read_bin(out, temp, 4);

    return 0;
}

int bn2int(int *out, bn_t in)
{
    uint8_t temp[4];
    bn_write_bin(temp, 4, in);
    uint8_t2int(out, temp);

    return 0;
}

int sint2bn(bn_t out, int in, int size)
{
    (void)size;
    const string str = std::to_string(in);
    bn_read_str(out, str.c_str(), static_cast<int>(str.size()), 10);
    return 0;
}

void ZZ2bn(bn_t out, const ZZ &in)
{
    bn_null(out);
    bn_new(out);
    if (in == 0)
    {
        bn_read_str(out, "0", 1, 10);
    }
    else
    {
        std::stringstream buffer;
        buffer << in;
        const string ss(buffer.str());
        bn_read_str(out, ss.c_str(), static_cast<int>(ss.length()), 10);
    }
}

void bn2ZZ(ZZ &out, const bn_t in)
{
    int size = bn_size_str(in, 10);
    char *in_char = new char[size];
    bn_write_str(in_char, size, in, 10);
    out = conv<ZZ>(in_char);
    delete[] in_char;
}

void DataProcess(double &mean, double &stdev, double *Time, int cyctimes)
{
    if (cyctimes <= 0 || Time == nullptr)
    {
        mean = 0.0;
        stdev = 0.0;
        return;
    }

    const double sum = std::accumulate(Time, Time + cyctimes, 0.0);
    mean = sum / cyctimes;
    double temp_sum = 0.0;
    for (int i = 0; i < cyctimes; i++)
    {
        double temp = mean - Time[i];
        temp *= temp;
        temp_sum = temp_sum + temp;
    }
    stdev = sqrt(temp_sum / cyctimes);
    stdev = (mean == 0.0) ? 0.0 : stdev / mean;
}

double SteadyTimeSeconds()
{
    using Clock = std::chrono::steady_clock;
    static const Clock::time_point base = Clock::now();
    const Clock::time_point now = Clock::now();
    return std::chrono::duration<double>(now - base).count();
}

TimingResult MeasureTimeMs(const function<void()> &fn, int samples,
                           int iterations_per_sample,
                           bool adaptive,
                           double min_sample_ms,
                           int max_adaptive_iters)
{
    using Clock = std::chrono::steady_clock;

    if (samples < 1)
    {
        samples = 1;
    }
    if (iterations_per_sample < 1)
    {
        iterations_per_sample = 1;
    }
    if (max_adaptive_iters < iterations_per_sample)
    {
        max_adaptive_iters = iterations_per_sample;
    }

    vector<double> sample_ms(samples);
    int measured_iters = iterations_per_sample;

    for (int i = 0; i < samples; ++i)
    {
        int iters = iterations_per_sample;
        double elapsed_ms = 0.0;

        while (true)
        {
            const Clock::time_point start = Clock::now();
            for (int j = 0; j < iters; ++j)
            {
                fn();
            }
            const Clock::time_point end = Clock::now();
            elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();

            if (!adaptive || elapsed_ms >= min_sample_ms || iters >= max_adaptive_iters)
            {
                break;
            }

            double scale = min_sample_ms / (elapsed_ms > 0.0 ? elapsed_ms : 0.001);
            if (scale < 2.0)
            {
                scale = 2.0;
            }
            if (scale > 16.0)
            {
                scale = 16.0;
            }

            long long next_iters = static_cast<long long>(iters * scale) + 1;
            if (next_iters <= iters)
            {
                next_iters = static_cast<long long>(iters) + 1;
            }
            if (next_iters > max_adaptive_iters)
            {
                next_iters = max_adaptive_iters;
            }
            iters = static_cast<int>(next_iters);
        }

        measured_iters = iters;
        sample_ms[i] = elapsed_ms / iters;
    }

    double mean = 0.0;
    for (int i = 0; i < samples; ++i)
    {
        mean += sample_ms[i];
    }
    mean /= samples;

    double variance = 0.0;
    for (int i = 0; i < samples; ++i)
    {
        const double diff = sample_ms[i] - mean;
        variance += diff * diff;
    }
    variance /= samples;

    vector<double> sorted = sample_ms;
    sort(sorted.begin(), sorted.end());
    double median = sorted[samples / 2];
    if (samples % 2 == 0)
    {
        median = (sorted[samples / 2 - 1] + sorted[samples / 2]) / 2.0;
    }

    TimingResult result;
    result.samples = samples;
    result.iterations_per_sample = measured_iters;
    result.adaptive = adaptive;
    result.mean_ms = mean;
    result.median_ms = median;
    result.min_ms = sorted.front();
    result.rsd = (mean == 0.0) ? 0.0 : sqrt(variance) / mean;
    return result;
}

TimingResult MeasureTimeMsAdaptive(const function<void()> &fn, int samples,
                                   double min_sample_ms,
                                   int max_adaptive_iters)
{
    return MeasureTimeMs(fn, samples, 1, true, min_sample_ms, max_adaptive_iters);
}

void PrintTimeMs(const string &label, const TimingResult &result)
{
    cout << label << ": " << result.mean_ms << " ms  RSD: " << result.rsd * 100 << "%";
    if (result.adaptive)
    {
        cout << "  iters/sample: " << result.iterations_per_sample;
    }
    cout << "\n";
}


void NativeEval(ZZ &y, int d, int num_data, const vec_ZZ &X, const ZZ &mmod, const vector<vector<int>> &F_TEST)
{
    (void)d;
    y = 0;
    for (int i = 0; i < F_TEST.size(); ++i)
    {
        ZZ monotmp(1);
        for (int j = 0; j < num_data; ++j)
        {
            if (F_TEST[i][j] == 0)
            {
                continue;
            }
            ZZ tmp;
            PowerMod(tmp, X[j], F_TEST[i][j], mmod);
            MulMod(monotmp, monotmp, tmp, mmod);
        }
        AddMod(y, y, monotmp, mmod);
    }
}

ZZ PRF_ZZ(int prfkey, const ZZ &mmod)
{
    ZZ res;
    PRFBoundedZZ(res, prfkey, mmod);
    return res;
}

void PRF_ZZ(ZZ& res, int prfkey, const ZZ& mmod)
{
    PRFBoundedZZ(res, prfkey, mmod);
}

void PRF_bn(bn_t res, int prfkey, const ZZ &mmod)
{
    ZZ tmp;
    tmp = PRF_ZZ(prfkey, mmod);
    bn_null(res);
    bn_new(res);
    ZZ2bn(res, tmp);
}

void Random_Func(vector<vector<int>> &F_TEST, int msg_num, int degree_f)
{
    vector<vector<int>> Combinations;
    vector<int> tmp;

    for (int i = 1; i <= degree_f; ++i)
    {
        Combinations.clear();
        tmp.clear();
        int len = 0;
        partitions(len, Combinations, i, msg_num, tmp, 0);
        srand(time(0));
        tmp = Combinations[rand() % len];
        F_TEST.push_back(tmp);
    }
}

void partitions(int &cnt, vector<vector<int>> &Res, int sum, int k, vector<int> &lst, int minn)
{
    if (k == 0)
    {
        if (sum == 0)
        {
            Res.push_back(lst);
            ++cnt;
        }
        return;
    }
    for (int i = minn; i < sum + 1; ++i)
    {
        lst.push_back(i);
        partitions(cnt, Res, sum - i, k - 1, lst, i);
        lst.pop_back();
    }
}

void GENERATE_RANDOM_FUNCTION(int msg_num, int degree_f)
{

    ofstream fout;  
    vector<vector<int>> F_TEST;
    Random_Func(F_TEST, msg_num, degree_f);

    fout.open("function.txt"); // 关联一个文件
    for (int i = 0; i < F_TEST.size(); ++i)
    {
        for (int j = 0; j < F_TEST[i].size(); ++j)
        {
            fout << F_TEST[i][j] << " "; // 写入
        }
        fout << endl;
    }
    fout.close();
}

ZZ Combine(int n, int m)
{
    if (n == m || m == 0)
        return ZZ(1);
    return Combine(n, m - 1) * ZZ(n - m + 1) / ZZ(m);
}


void Random_ZZ_pX(ZZ_pX &a, int N, int q_bit) {
    ZZ_p coeff;
    for (int i = 0; i < N; i++) {
        conv(coeff, RandomBits_ZZ(q_bit));
        SetCoeff(a, i, coeff);
    }
}

void RLWESecretKey(ZZ_pX &sk, int N, int hsk) {
    int interval = 0;
    interval = N / hsk;
    int index = rand() % interval;
    for (int i = 0; i < hsk; i++) {
        SetCoeff(sk, index, 1);
        index = index + rand() % interval;
    }

}

void GaussRand(ZZ_pX &e, int N) {
    double res_standard;
    int deviation = 8;
    int res;
    for (int i = 0; i < N; i++) {
        res_standard = sqrt(-2.0 * log(rand() / (RAND_MAX + 1.0))) * sin(2.0 * PI * rand() / (RAND_MAX + 1.0));
        res = res_standard * deviation;
        SetCoeff(e, i, res);
    }
}

void ZZ_pX_ScaleMul_ZZ(ZZ_pX &out, const ZZ_pX &in1, const ZZ &in2)
{
    ZZ_pX a;
    conv(a,in2);
    out = in1 * a;
}

// 计算 P_d(x1, x2, x3, x4, x5) 的动态规划函数
ZZ P_d(const vec_ZZ& x, int degree_f) {
    double start = SteadyTimeSeconds();
    int k = x.length(); // 变量个数

    // 定义 dp[k+1][d+1]，初始化为 0
    Mat<ZZ> dp;
    dp.SetDims(k + 1, degree_f + 1);
    dp[0][0] = ZZ(1); 
    
    ZZ tmp;
    // 动态规划填表
    for (int i = 1; i <= k; i++) { // 依次加入 x1, x2, ..., x5
        for (int s = 0; s <= degree_f; s++) { // 目标和从 0 到 d
            dp[i][s] = ZZ(0);
            for (int j = 0; j <= s; j++) { 
                power(tmp, x[i - 1], j); 
                mul(tmp, tmp, dp[i - 1][s - j]); 
                add(dp[i][s], dp[i][s], tmp); 
            }
        }
    }

    // 计算最终结果 P_d(x1, ..., x5)
    ZZ result(0);
    for (int s = 1; s <= degree_f; s++) {
        add(result, result, dp[k][s]); 
    }

    start = SteadyTimeSeconds() - start;
    cout << "Time: " << start << " seconds" << endl;
    return result;
}


ZZ P_d2(const vec_ZZ& x, int degree_f) {
    double start = SteadyTimeSeconds();
    int k = x.length();

    // 使用一维数组而非二维矩阵，从而改善缓存命中率
    vector<ZZ> dp_prev(degree_f + 1, ZZ(0));
    vector<ZZ> dp_curr(degree_f + 1, ZZ(0));
    vector<ZZ> powers(degree_f + 1, ZZ(1));
    
    dp_prev[0] = ZZ(1);
    
    for (int i = 0; i < k; i++) {
        for (int j = 1; j <= degree_f; ++j) {
            mul(powers[j], powers[j - 1], x[i]);
        }

        for (int s = 0; s <= degree_f; s++) {
            dp_curr[s] = ZZ(0);
            for (int j = 0; j <= s; j++) {
                ZZ tmp;
                mul(tmp, powers[j], dp_prev[s - j]);
                add(dp_curr[s], dp_curr[s], tmp);
            }
        }
        // Swap current and previous arrays
        dp_prev.swap(dp_curr);
    }
    
    // 计算最终结果
    ZZ result(0);
    for (int s = 1; s <= degree_f; s++) {
        add(result, result, dp_prev[s]);
    }
    
    start = SteadyTimeSeconds() - start;
    cout << "Time: " << start * 1000 << " ms" << endl;
    return result;
}
