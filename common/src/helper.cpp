#include "helper.h"

int int2uint8_t(uint8_t *out, int in)
{
    int num = 1;
    // Output has to be big endian
    if (*(char *)&num == 1) // Little endian
    {
        out[3] = (uint8_t)(in >> 0);
        out[2] = (uint8_t)(in >> 8);
        out[1] = (uint8_t)(in >> 16);
        out[0] = (uint8_t)(in >> 24);
    }
    else // Big endian
    {
        out[3] = (uint8_t)(in << 0);
        out[2] = (uint8_t)(in << 8);
        out[1] = (uint8_t)(in << 16);
        out[0] = (uint8_t)(in << 24);
    }
    return 0;
}

int uint8_t2int(int *out, uint8_t *in)
{
    // The byte array is in big endian
    int num = 1;
    if (*(char *)&num == 1) // Little endian
    {
        *out = (int)in[0];
        *out = (*out << 8);

        *out += (int)in[1];
        *out = (*out << 8);

        *out += (int)in[2];
        *out = (*out << 8);

        *out += (int)in[3];
    }
    else // Big endian
    {
        *out = (int)in[0];
        *out = (*out >> 8);

        *out += (int)in[1];
        *out = (*out >> 8);

        *out += (int)in[2];
        *out = (*out >> 8);

        *out += (int)in[3];
    }

    return 0;
}

int int2bn(bn_t out, int in)
{
    uint8_t *temp = (uint8_t *)malloc(sizeof(uint8_t) * 4);
    int2uint8_t(temp, in);
    bn_read_bin(out, temp, 4);
    free(temp);

    return 0;
}

int bn2int(int *out, bn_t in)
{
    uint8_t *temp = (uint8_t *)malloc(sizeof(uint8_t) * 4);
    bn_write_bin(temp, 4, in);
    uint8_t2int(out, temp);
    free(temp);

    return 0;
}

int sint2bn(bn_t out, int in, int size)
{
    char *str = (char *)malloc(sizeof(char) * size);
    snprintf(str, size, "%d", in);
    bn_read_str(out, str, size, 10);
    free(str);
    return 0;
}

void ZZ2bn(bn_t out, ZZ in)
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
        string ss(buffer.str());
        const char *str = strdup(ss.c_str());
        bn_read_str(out, str, ss.length(), 10);
        // bn_read_str(out, str, floor(log(in) / log(10)) + 1, 10);
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
    double temp;
    double sum = 0;
    for (int i = 0; i < cyctimes; i++)
    {
        sum = sum + Time[i];
    }
    mean = sum / cyctimes;
    double temp_sum = 0;
    for (int i = 0; i < cyctimes; i++)
    {
        temp = mean - Time[i];
        temp = temp * temp;
        temp_sum = temp_sum + temp;
    }
    stdev = sqrt(temp_sum / cyctimes);
    stdev = stdev / mean;
}


void NativeEval(ZZ &y, int d, int num_data, vec_ZZ X, ZZ mmod, vector<vector<int>> F_TEST)
{
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

ZZ PRF_ZZ(int prfkey, ZZ mmod)
{
    ZZ res;
    SetSeed(ZZ(prfkey) % mmod);
    RandomBnd(res, mmod);
    return res;
}

void PRF_bn(bn_t res, int prfkey, ZZ mmod)
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