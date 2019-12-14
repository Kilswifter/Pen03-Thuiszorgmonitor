#include "AEGIS.hpp"

void decryption(int cipherTextBlock[4][4],int S0[4][4],int S1[4][4], int S2[4][4], int S3[4][4],int S4[4][4]){
  
  XORMatrix(cipherTextBlock, S1);
  XORMatrix(xorResult, S4);
  ANDMatrix(S2, S3);
  XORMatrix(xorResult, ANDresult);
  copyMatrix(result, xorResult);

  stateUpdate(S0, S1, S2, S3, S4, result);
}
