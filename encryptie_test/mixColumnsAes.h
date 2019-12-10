#include "AEGIS.hpp"

//   g++ mixColumnsAes.cpp -o mixColumnsAes
//  ./mixColumnsAes

int multiply(int a, int b, int E[16][16], int L[16][16]){
    //  This function multiplies a given a and b
    //
    //  E and L are used as look-up tables for the multiplication for AES algorithm
    if(a==0 or b == 0){
        return 0;
    }

    int la = L[a/16][a % 16];
    int lb = L[b/16][b % 16];
    int lResult = la + lb;
    lResult = (lResult%255);
    int eResult = E[lResult/16][lResult % 16];
    return eResult;
}

void mixColumns(int B[4][4], int C[4][4], int E[16][16], int L[16][16], int CX[4][4]){
    //  This function swaps elements from each column following the AES algorithm
    //
    //  The input matrix is B, the result is stored in C 
    //
    //  CX is used to store tempory results that eventually will be stored in C
    //
    //  E and L are used as look-up tables for the multiplication for AES algorithm

    for (int j = 0; j<4; j++){
        for (int i = 0; i<4; i++){
            CX[j][i] = B[i][j];
        }
    }

    for (int k = 0; k<4; k++){
        C[0][k] = (multiply(0x02, CX[k][0], E, L)) ^ (multiply(0x03, CX[k][1], E,L)) ^ (CX[k][2]) ^ (CX[k][3]);
        C[1][k] = (CX[k][0]) ^ (multiply(0x02, CX[k][1], E, L)) ^ (multiply(0x03, CX[k][2], E, L)) ^ (CX[k][3]);
        C[2][k] = (CX[k][0]) ^ (CX[k][1]) ^ (multiply(0x02, CX[k][2], E, L)) ^ (multiply(0x03, CX[k][3], E, L));
        C[3][k] = (multiply(0x03, CX[k][0], E, L)) ^ (CX[k][1]) ^ (CX[k][2]) ^ (multiply(0x02, CX[k][3], E, L));
    }
}

