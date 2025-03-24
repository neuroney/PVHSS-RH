#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include <NTL/ZZ_pX.h>
#include <NTL/matrix.h>
#include <NTL/vec_vec_ZZ.h>



using namespace std;
using namespace NTL;
struct Data {
    vec_ZZ X_decimal;
    Vec<ZZ_pX> X;
    Vec<vec_ZZ_pX> C_X;
    Vec<vec_ZZ_pX> PRF;
    vector<vector<int>> F_TEST;
    vec_ZZ_pX C_1;
};

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
        // srand(0);
        tmp = Combinations[rand() % len];
        // random_shuffle(tmp.begin(), tmp.end());
        F_TEST.push_back(tmp);
        //for (int j = 0; j < len; ++j) {
            // F_TEST.push_back(Combinations[0]);
        //}
    }
}

void GenData(Data &data, PKE_Para pkePara,ZZ_pXModulus &modulus, vec_ZZ_pX pkePk) {
    data.X.SetLength(pkePara.num_data);
    vec_ZZ_pX C_x, prf;
    C_x.SetLength(4);
    prf.SetLength(2);
    data.X_decimal.SetLength(pkePara.num_data);
    double time;
    for (int i = 0; i < pkePara.num_data; i++) {
        data.X_decimal[i]= RandomBits_ZZ(pkePara.msg_bit);
        time = GetTime();
        Decimal2Bin(data.X[i],data.X_decimal[i],pkePara.msg_bit);
        time = GetTime() - time;
        //cout << "decimal2Bin time: " << time * 1000 <<"ms\n";
        PVHSS_Enc(C_x, pkePara, modulus, pkePk, data.X[i]);
        data.C_X.append(C_x);
    }
    for (int i = 0; i < 10; i++) {
        Random_ZZ_pX(prf[0], pkePara.N, pkePara.q_bit);
        Random_ZZ_pX(prf[1], pkePara.N, pkePara.q_bit);
        data.PRF.append(prf);
    }
    Random_Func(data.F_TEST, pkePara.num_data, pkePara.d);

    // C1 Gen
    data.C_1.SetLength(4);
    ZZ M_1(1);
    ZZ_pX M1_px;
    Decimal2Bin(M1_px,M_1,1);
    PVHSS_Enc(data.C_1, pkePara, modulus, pkePk, M1_px);
}