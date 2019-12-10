#include "AEGIS.hpp"

#include <iostream>
using namespace std;

//  g++ subBytesAes.cpp shiftRowsAes.cpp mixColumnsAes.cpp AESround.cpp -o AESround
//  ./AESround

void XORMatrix(int matrix1[4][4], int matrix2[4][4]){
  for (int i = 0; i<4; i++){
    for (int j = 0; j<4; j++){
        xorResult[i][j] = matrix1[i][j] ^ matrix2[i][j];
    }
  } 
}

void copyMatrix (int extra[4][4], int original[4][4]){
    for (int i = 0; i<4; i++){
        for (int j = 0; j<4; j++){
            extra[i][j] = original[i][j];
        }
    }
}

void transposeMatrix (int matrix[4][4]){
    for (int j = 0; j<4; j++){
        for (int i = 0; i<4; i++){
            transpose[j][i] = matrix[i][j];
        }
    }
    copyMatrix(matrix, transpose);
}

void setToZero (int matrix[4][4]){
  for (int i = 0; i<4; i++){
    for (int j = 0; j<4; j++){
      matrix[i][j] = 0;
    }
  }
}

void AESround (int A[4][4],int Z[4][4], int sBox[16][16],int B[4][4], int C[4][4],int E[16][16], int L[16][16], int CX[4][4],int AESroundResult [4][4], int Y[4][4]){
  subBytes(A, Z, sBox);
  shiftRows(Z,B);
  mixColumns(B, C, E, L, CX);
  XORMatrix(C,Y);

  transposeMatrix(xorResult);
  copyMatrix(AESroundResult, xorResult);
  }
