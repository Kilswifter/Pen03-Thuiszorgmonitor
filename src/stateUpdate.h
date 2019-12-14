#include "AEGIS.hpp"

//  g++ stateUpdate.cpp AESround.cpp subBytesAes.cpp shiftRowsAes.cpp mixColumnsAes.cpp -o stateUpdate
//  ./stateUpdate

void stateUpdate (int S0[4][4],int S1[4][4], int S2[4][4], int S3[4][4], int S4[4][4], int M[4][4]){
    transposeMatrix(S0);
    transposeMatrix(S1);
    transposeMatrix(S2);
    transposeMatrix(S3);
    transposeMatrix(S4);

    AESround(S4, Z, sBox, B, C, E, L, CX,AESroundResult, S0);
    copyMatrix(S0updateResult, AESroundResult);
    
    AESround(S0, Z, sBox, B, C, E, L, CX,AESroundResult, S1);
    copyMatrix(S1updateResult, AESroundResult);
 
    AESround(S1, Z, sBox, B, C, E, L, CX,AESroundResult, S2);
    copyMatrix(S2updateResult, AESroundResult);

    AESround(S2, Z, sBox, B, C, E, L, CX,AESroundResult, S3);
    copyMatrix(S3updateResult, AESroundResult);

    AESround(S3, Z, sBox, B, C, E, L, CX,AESroundResult, S4);
    copyMatrix(S4updateResult, AESroundResult);

    copyMatrix(S0, S0updateResult);
    
    XORMatrix(m, S0);
    copyMatrix(S0, xorResult);
    XORMatrix(m, m2);
    copyMatrix(m, xorResult);

    XORMatrix(S0, M);
    copyMatrix(S0, xorResult);
    
    copyMatrix(S1, S1updateResult);
    copyMatrix(S2, S2updateResult);
    copyMatrix(S3, S3updateResult);
    copyMatrix(S4, S4updateResult);

}
