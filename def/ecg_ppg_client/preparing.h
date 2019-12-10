#include "AEGIS.hpp"
#include <iostream>
using namespace std;

void preparing(int Key[4][4], int IV[4][4], int const0[4][4], int const1[4][4]){    
    XORMatrix(IV, Key);
    copyMatrix(S0,xorResult);
    copyMatrix(S1,const1);
    copyMatrix(S2,const0);
    XORMatrix(const0, Key);
    copyMatrix(S3,xorResult);
    XORMatrix(const1, Key);
    copyMatrix(S4,xorResult);

    copyMatrix(m, Key);
    copyMatrix(m2, IV);

    for (int i=0; i<10; i++){
        stateUpdate(S0,S1,S2,S3,S4, M);
    }

    setToZero(m);
    setToZero(m2);
}