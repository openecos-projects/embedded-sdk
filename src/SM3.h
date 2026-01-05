#ifndef __SM3_H__
#define __SM3_H__
#include "tool.h"

typedef union
{
	uint32_t D[8];
	uint8_t Digest[32];

}TDigest;

typedef union
{
	uint8_t buffer[64];
	uint32_t W[16];
}TData;


typedef struct TSM3Contex
{
	TData Data;
	TDigest Digest;
	uint64_t Size;
	uint64_t TotalSize;
}TTSM3Contex;

#define SM3_DIGEST_SIZE 32

void Sm3Init(TTSM3Contex* SM3Contex);
void Sm3Updata(TTSM3Contex* SM3Contex, uint8_t* Data, uint64_t DataLen);
void Sm3Final(TTSM3Contex* SM3Contex, uint8_t* Digest);


#endif	
