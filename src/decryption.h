#include "AEGIS.hpp"

void decryption(uint8_t cipherTextBlocksend[16],int S0[4][4],int S1[4][4], int S2[4][4], int S3[4][4],int S4[4][4]){
  int r = 0;
  for (int i = 0; i<4; i++){
      for (int j = 0; j<4; j++){
          cipherTextBlock[i][j] = cipherTextBlocksend[r];
          r += 1;
      }
  }
  XORMatrix(cipherTextBlock, S1);
  XORMatrix(xorResult, S4);
  ANDMatrix(S2, S3);
  XORMatrix(xorResult, ANDresult);
  copyMatrix(result, xorResult);

  int q = 0;
  for (int i = 0; i<4; i++){
    for (int j = 0; j<4; j++){
      resultSend[q] = result[i][j];
      q += 1;
    }
  }

  stateUpdate(S0, S1, S2, S3, S4, result);
}
