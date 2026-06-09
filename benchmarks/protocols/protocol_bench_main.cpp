#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include PVHSS_PROTOCOL_HEADER

using namespace std;

static vector<int> ParseDegrees(const string &value)
{
    vector<int> degrees;
    string token;
    stringstream input(value);
    while (getline(input, token, ','))
    {
        if (!token.empty())
        {
            degrees.push_back(atoi(token.c_str()));
        }
    }
    if (degrees.empty())
    {
        degrees.push_back(5);
    }
    return degrees;
}

static void PrintUsage(const char *program)
{
    cerr << "Usage: " << program << " [--msg-num N] [--cyctimes N] [--degrees 5,10,15]\n";
}

int main(int argc, char **argv)
{
    // Disable stdout buffering so logs appear in real time
    setbuf(stdout, NULL);

    int msg_num = 5;
    int cyctimes = 3;
    vector<int> degrees;
    degrees.push_back(5);

    for (int i = 1; i < argc; ++i)
    {
        const string arg(argv[i]);
        if (arg == "--msg-num" && i + 1 < argc)
        {
            msg_num = atoi(argv[++i]);
        }
        else if (arg == "--cyctimes" && i + 1 < argc)
        {
            cyctimes = atoi(argv[++i]);
        }
        else if (arg == "--samples" && i + 1 < argc)
        {
            // backward compat: --samples is alias for --cyctimes
            cyctimes = atoi(argv[++i]);
        }
        else if (arg == "--degrees" && i + 1 < argc)
        {
            degrees = ParseDegrees(argv[++i]);
        }
        else if (arg == "--help")
        {
            PrintUsage(argv[0]);
            return 0;
        }
        else
        {
            PrintUsage(argv[0]);
            return 2;
        }
    }

    if (msg_num < 1)
    {
        msg_num = 1;
    }
    if (cyctimes < 1)
    {
        cyctimes = 1;
    }

    cout << "protocol_benchmark"
         << ",backend=" << PVHSS_PROTOCOL_BACKEND
         << ",scheme=" << PVHSS_PROTOCOL_SCHEME
         << ",msg_num=" << msg_num
         << ",cyctimes=" << cyctimes << "\n";

    for (size_t i = 0; i < degrees.size(); ++i)
    {
        cout << "degree_f: " << degrees[i] << "\n";
        PVHSS_PROTOCOL_TIME_TEST(msg_num, degrees[i], cyctimes);
    }

    return 0;
}
