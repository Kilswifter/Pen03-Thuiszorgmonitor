#include "AEGIS.hpp"

//  g++ subBytesAes.cpp -o subBytesAes
//  ./subBytesAes

void subBytes( int A[4][4], int Z[4][4], int sBox[16][16]){
  //  This function swaps the elemenst with the adequate element from the sBox
  //  
  //  the input matrix is A, the result is stored in Z

      for (int i = 0; i<4; i++){
        for ( int j = 0; j< 4; j++){
          Z[i][j] = sBox[A[i][j]/16][A[i][j]%16];
        }
      }
    }



