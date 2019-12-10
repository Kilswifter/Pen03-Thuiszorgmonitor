#include "AEGIS.hpp"
#include <iostream>
using namespace std;

void createTag (int S0[4][4], int S1[4][4], int S2[4][4], int S3[4][4], int S4[4][4],unsigned long long int msglen, unsigned long long int adlen){
    maketemp(S3, adlen, msglen, temp);

    for(int i = 0; i <7; i++){
        stateUpdate(S0, S1, S2, S3, S4, temp);
    }
    
    XORMatrix(S0, S1);
    XORMatrix(xorResult, S2);
    XORMatrix(xorResult, S3);
    XORMatrix(xorResult, S4);

    copyMatrix(tag, xorResult);

    int r = 0;
    for (int i=0; i<4; i++){
      for (int j=0; j<4; j++){
        tagsend[r] = tag[i][j];
        r+=1;
      }
    }
    
    setToZero(M);
}
    
void maketemp(int S3[4][4], unsigned long long int adlen, unsigned long long int msglen ,int temp[4][4]){
    setToZero(temp);
    temp[0][0] = ((adlen*8) % 256);
    temp[2][0] = ((msglen*8) % 256);

    XORMatrix(S3, temp);
    copyMatrix(temp, xorResult); 
}
