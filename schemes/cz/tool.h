#include <gmp.h>
extern "C"{
#include <relic/relic.h>
}
#include <sstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <functional>

using namespace std;
using namespace NTL;
#define PI 3.141592654

void Random_ZZ_pX(ZZ_pX &a, int N, int q_bit) {
    ZZ_p coeff;
    for (int i = 0; i < N; i++) {
        conv(coeff, RandomBits_ZZ(q_bit));
        SetCoeff(a, i, coeff);
    }
}

void SecretKey(ZZ_pX &sk, int N, int hsk) {
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

void NativeEval(ZZ &y, int d, int num_data, vec_ZZ X, ZZ mmod, vector<vector<int>> F_TEST)
{
    // ZZ temp;
    // for (int i = 1; i < d + 1; i++)
    // {
    //     int *ind_var = (int *)malloc(sizeof(int) * i);
    //     NativeEval_f(temp, i, num_data, 0, 0, ind_var, X, mmod);
    //     AddMod(y, y, temp, mmod);
    //     temp = 0;
    // }

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

void Decimal2Bin(ZZ_pX &out, ZZ in, int bits)
{
    ZZ rem;
    ZZX out_ZZ_pX;
    for(int i=0;i<bits;i++)
    {
        rem=in%2;
        SetCoeff(out_ZZ_pX,i, rem);
        in=(in-rem)/2;
    }
    conv(out,out_ZZ_pX);
}

void ZZ_pX_ScaleMul_ZZ(ZZ_pX &out, ZZ_pX in1,ZZ in2)
{
    ZZ_pX a;
    conv(a,in2);
    out=in1*a;
}


void ZZ2bn(bn_t out, ZZ in)
{
    if(in==0)
    {
        bn_read_str(out,"0",1,10);
    }else
    {
        std::stringstream buffer;
        buffer << in;
        const char* str=strdup(buffer.str().c_str());
        bn_read_str(out,str,floor(log(in)/log(10))+1,10);
    }

}

void eval_ZZX(ZZ &y,ZZX f)
{
    ZZ coeff,pow2;
    pow2=1;
    for(int i=0;i<deg(f)+1;i++)
    {
        GetCoeff(coeff,f,i);
        y=y+coeff*pow2;
        pow2= pow2*2;
    }

}

void DataProcess(double &mean, double &stdev,  double *Time,int cyctimes)
{
    if (cyctimes <= 0 || Time == nullptr)
    {
        mean = 0.0;
        stdev = 0.0;
        return;
    }

    double temp;
    double sum=0.0;
    for(int i=0;i<cyctimes;i++)
    {
        sum=sum+Time[i];
    }
    mean=sum/cyctimes;
    double temp_sum=0.0;
    for(int i=0;i<cyctimes;i++)
    {
        temp=mean-Time[i];
        temp=temp*temp;
        temp_sum=temp_sum+temp;
    }
    stdev=sqrt(temp_sum/cyctimes);
    stdev=(mean == 0.0) ? 0.0 : stdev/mean;
}

static double CZSteadyTimeSeconds()
{
    using Clock = std::chrono::steady_clock;
    static const Clock::time_point base = Clock::now();
    const Clock::time_point now = Clock::now();
    return std::chrono::duration<double>(now - base).count();
}

struct CZTimingResult
{
    int samples;
    int iterations_per_sample;
    bool adaptive;
    double mean_ms;
    double median_ms;
    double min_ms;
    double rsd;
};

static CZTimingResult CZMeasureTimeMs(const function<void()> &fn, int samples,
                                      int iterations_per_sample = 1,
                                      bool adaptive = false,
                                      double min_sample_ms = 25.0,
                                      int max_adaptive_iters = 10000000)
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

    CZTimingResult result;
    result.samples = samples;
    result.iterations_per_sample = measured_iters;
    result.adaptive = adaptive;
    result.mean_ms = mean;
    result.median_ms = median;
    result.min_ms = sorted.front();
    result.rsd = (mean == 0.0) ? 0.0 : sqrt(variance) / mean;
    return result;
}

static CZTimingResult CZMeasureTimeMsAdaptive(const function<void()> &fn, int samples,
                                              double min_sample_ms = 25.0,
                                              int max_adaptive_iters = 10000000)
{
    return CZMeasureTimeMs(fn, samples, 1, true, min_sample_ms, max_adaptive_iters);
}

static void CZPrintTimeMs(const string &label, const CZTimingResult &result)
{
    cout << label << ": " << result.mean_ms << " ms  RSD: " << result.rsd * 100 << "%";
    if (result.adaptive)
    {
        cout << "  iters/sample: " << result.iterations_per_sample;
    }
    cout << "\n";
}
