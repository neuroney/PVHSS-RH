#pragma once
#include "helper.h"

typedef struct
{
    ZZ f;
    ZZ g;
    ZZ h;
    ZZ N;
    ZZ N2;
} Elgamal_PK;

typedef array<ZZ, 2> Elgamal_CT;
typedef ZZ Elgamal_SK;

void Elgamal_Gen(Elgamal_PK &pk, Elgamal_SK &d, int skLen);
void Elgamal_Enc(Elgamal_CT &ct, const Elgamal_PK &pk, const ZZ &x);
void Elgamal_skEnc(Elgamal_CT &ct, const Elgamal_PK &pk, const ZZ &x);
void Elgamal_Dec(ZZ &x, const Elgamal_PK &pk, const Elgamal_SK &sk, const Elgamal_CT &ct);