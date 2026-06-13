/// DC Group scheme benchmark using unified interface.

#include "scheme_bench_runner.h"
#include "scheme_dc_group.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace pvhss;

int main(int argc, char** argv)
{
    setbuf(stdout, NULL);
    bench::BenchConfig cfg;
    cfg.msg_num  = 5; cfg.cyctimes = 1; cfg.degree_f = 5;
    cfg.modulus  = NTL::ZZ(0);

    for (int i=1;i<argc;++i){string arg(argv[i]);
        if(arg=="--msg-num"&&i+1<argc)cfg.msg_num=atoi(argv[++i]);
        else if(arg=="--cyctimes"&&i+1<argc)cfg.cyctimes=atoi(argv[++i]);
        else if(arg=="--degree"&&i+1<argc)cfg.degree_f=atoi(argv[++i]);
        else if(arg=="--msg-bits"&&i+1<argc)cfg.msg_bits=atoi(argv[++i]);
        else if(arg=="--verbose")cfg.verbose=true;
    }
    bench::RunSchemeBench<scheme::SchemeDcGroup>(cfg);
    return 0;
}
