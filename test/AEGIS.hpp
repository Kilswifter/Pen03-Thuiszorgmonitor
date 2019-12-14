//subBytes
extern int Z[4][4];
extern int sBox [16][16];
void subBytes( int A[4][4], int Z[4][4], int sBox[16][16]);

//shiftRows
extern int B[4][4];
void shiftRows (int Z[4][4], int B[4][4]);
int newPos (int currentPos, int add, int mod);

//mixColumns
extern int C[4][4];
extern int CX[4][4];
extern int E[16][16];
extern int L[16][16];
void mixColumns(int B[4][4], int C[4][4], int E[16][16], int L[16][16], int CX[4][4]);
int multiply(int a, int b, int E[16][16], int L[16][16]);

//extras
extern int xorResult[4][4];
void XORMatrix(int matrix1[4][4], int matrix2[4][4]);

void copyMatrix (int extra[4][4], int original[4][4]);
void printMatrix(int matrix[4][4]);
void transposeMatrix(int matrix[4][4]);
extern int transpose[4][4];
void setToZero (int matrix[4][4]);

extern int m[4][4];
extern int m2[4][4];

//AESround
extern int AESroundResult [4][4];
void AESround (int A[4][4],int Z[4][4], int sBox[16][16],int B[4][4], int C[4][4],int E[16][16], int L[16][16], int CX[4][4],int AESroundResult [4][4], int Y[4][4]);

// StateUpdate
void stateUpdate (int S0[4][4],int S1[4][4], int S2[4][4], int S3[4][4], int S4[4][4], int M[4][4]);
// preparing
extern int S0[4][4];
extern int S1[4][4];
extern int S2[4][4];
extern int S3[4][4];
extern int S4[4][4];
extern int S0updateResult[4][4];
extern int S1updateResult[4][4];
extern int S2updateResult[4][4];
extern int S3updateResult[4][4];
extern int S4updateResult[4][4];

extern int Key[4][4];
extern int IV[4][4];
extern int const0[4][4];
extern int const1[4][4];
extern int M[4][4];
void preparing(int Key[4][4], int IV[4][4], int const0[4][4], int const1[4][4]);

// //encryption
extern uint8_t plaintext[16];
extern int newPlaintext[4][4];
extern short int P;
extern unsigned char lowerByte;
extern unsigned char upperByte;
extern int indexPlaintextConverter[1];
extern int cipherTextBlock[4][4];
extern uint8_t cipherTextBlocksend[16];
//////
// encryption en plaintextconverter eerste argument aangepast voor te testen !
void encryption(uint8_t plaintext[16],int S0[4][4],int S1[4][4], int S2[4][4], int S3[4][4],int S4[4][4]);
void plaintextConverter(uint8_t plaintext[16], int newPlaintext[4][4]);
/////
void encryptBlock(int S0[4][4], int S1[4][4], int S2[4][4], int S3[4][4], int S4[4][4], int newPlaintext[4][4],int cipherTextBlock[4][4]);
void ANDMatrix(int S2[4][4], int S3[4][4]);

// decryption
void decryptBlock(int S0[4][4], int S1[4][4], int S2[4][4], int S3[4][4], int S4[4][4], int newPlaintext[4][4],int cipherTextBlock[4][4]);
void decryption(uint8_t cipherTextBlock[4][4],int S0[4][4],int S1[4][4], int S2[4][4], int S3[4][4],int S4[4][4]);
extern int result[4][4];
extern uint8_t resultSend[16];
// //AND fucntie
extern int ANDresult[4][4];

// //tag
int checkTag(uint8_t tag1[16], uint8_t tag2[16]);
extern unsigned long long int msglen;
extern unsigned long long int adlen;
extern int temp[4][4];
extern int tag[4][4];
extern uint8_t tagsend[16];
void createTag (int S0[4][4], int S1[4][4], int S2[4][4], int S3[4][4], int S4[4][4],unsigned long long int msglen, unsigned long long int adlen);
void maketemp(int S3[4][4], unsigned long long int adlen, unsigned long long int msglen, int temp[4][4]);

////////////////////////
extern int testPlaintext[4][4];

extern int m [4][4];
