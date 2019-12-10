#include "AEGIS.hpp"

// Om te testen is het eerste argument van de "encryption functie": short int Plaintext[8] verandert
// naar int testPlaintext[4][4]. Dit is ook voor de "plaintextConverter functie"
void encryption(int plaintext[16],int S0[4][4],int S1[4][4], int S2[4][4], int S3[4][4],int S4[4][4]){
    plaintextConverter(plaintext, newPlaintext);
    encryptBlock(S0,S1,S2,S3,S4,newPlaintext,cipherTextBlock);
    }

void plaintextConverter(int plaintext[16], int newPlaintext[4][4]){
  int r = 0;
  for(int i = 0; i<4; i++){
    for(int j = 0; j<4; j++){
      newPlaintext[i][j] = plaintext[r];
      r += 1;
    }
  }            
}

void encryptBlock(int S0[4][4], int S1[4][4], int S2[4][4], int S3[4][4], int S4[4][4], int newPlaintext[4][4],int cipherTextBlock[4][4]){
    XORMatrix(newPlaintext, S1);
    XORMatrix(xorResult, S4);
    ANDMatrix(S2, S3);
    XORMatrix(xorResult, ANDresult);
    copyMatrix(cipherTextBlock, xorResult);
    
    copyMatrix(M, newPlaintext);
    
    stateUpdate(S0, S1, S2, S3, S4,M);

    setToZero(M);
}

void ANDMatrix(int S2[4][4], int S3[4][4]){
  for (int i = 0; i<4; i++){
    for (int j = 0; j<4; j++){
        ANDresult[i][j] = S2[i][j] & S3[i][j];
    }
  } 
}