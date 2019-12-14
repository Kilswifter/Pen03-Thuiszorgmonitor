#include "AEGIS.hpp"

//  g++ shiftRowsAes.cpp -o shiftRowsAes
//  ./shiftRowsAes

void shiftRows (int Z[4][4], int B[4][4]){
    //  This functions shifts the elements of each row, accordingly the AES algorithm
    //
    //  The input matrix has is Z, the result is stored in B.
    
    for (int i = 0; i<4; i++){
        if (i < 1){
            for ( int k = 0; k < 4; k++){
                B[i][k] = Z[i][k];
            }
        } 
        else{
        for (int j =0; j<4; j++){
            B[i][newPos(j, (i+(2*(2-i))),4)] = Z[i][j];
        }
        }
    }
}

int newPos (int currentPos, int add, int mod){
    // This function calculates the new position given a current possition for the shiftRowfunction
    
    return (currentPos + add) % mod;
}

