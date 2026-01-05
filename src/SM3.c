#include "tool.h"
#include "SM3.h"
#include<string.h>

#define FF1(X,Y,Z) ((X)^(Y)^(Z))
#define FF2(X,Y,Z) (((X)&(Y))|((X)&(Z))|((Y)&(Z)))
#define GG1(X,Y,Z) ((X)^(Y)^(Z))
#define GG2(X,Y,Z) (((X)&(Y))|((~X)&(Z)))
#define P0(X)	   ((X)^(ROL32(X,9))^(ROL32(X,17)))
#define P1(X)	   ((X)^(ROL32(X,15))^(ROL32(X,23)))


uint8_t padding[64] = 
{
	0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


void MessageExt(TTSM3Contex* SM3Contex,uint32_t* W, uint32_t* WP)
{
	uint8_t j = 0;
	uint32_t Temp = 0;
	for (j = 0; j < 16; j++)
	{
		W[j] = betoh32(SM3Contex->Data.W[j]);
	}
	for (j = 16; j < 68; j++)
	{
		Temp = W[j - 16] ^ W[j - 9] ^ ROL32(W[j - 3], 15);
		W[j] = P1(Temp) ^ ROL32(W[j - 13], 7) ^ W[j - 6];
	}
	for (j = 0; j < 64; j++)
	{
		WP[j] = W[j] ^ W[j + 4];
	}
}


void ProcessBlock(TTSM3Contex* SM3Contex)
{
	uint32_t i = 0;
	uint32_t W[68] = { 0 };
	uint32_t WP[64] = { 0 };
	uint32_t SS1, SS2, TT1, TT2, T, Temp;
	uint32_t A, B, C, D, E, F, G, H;
	A = SM3Contex->Digest.D[0];
	B = SM3Contex->Digest.D[1];
	C = SM3Contex->Digest.D[2];
	D = SM3Contex->Digest.D[3];
	E = SM3Contex->Digest.D[4];
	F = SM3Contex->Digest.D[5];
	G = SM3Contex->Digest.D[6];
	H = SM3Contex->Digest.D[7];

	MessageExt(SM3Contex, W, WP);
	for (i = 0; i < 64; i++)
	{
		if (i <= 15)
		{
			T = 0x79cc4519;
			Temp = ROL32(A, 12) + E + ROL32(T, i);
			SS1 = ROL32(Temp, 7);
			SS2 = SS1 ^ ROL32(A, 12);
			TT1 = FF1(A, B, C) + D + SS2 + WP[i];
			TT2 = GG1(E, F, G) + H + SS1 + W[i];
		}
		else
		{
			T = 0x7a879d8a;
			Temp = ROL32(A, 12) + E + ROL32(T, i);
			SS1 = ROL32(Temp, 7);
			SS2 = SS1 ^ ROL32(A, 12);
			TT1 = FF2(A, B, C) + D + SS2 + WP[i];
			TT2 = GG2(E, F, G) + H + SS1 + W[i];
		}

		D = C;
		C = ROL32(B, 9);
		B = A;
		A = TT1;
		H = G;
		G = ROL32(F, 19);
		F = E;
		E = P0(TT2);

	}
	SM3Contex->Digest.D[0] ^= A;
	SM3Contex->Digest.D[1] ^= B;
	SM3Contex->Digest.D[2] ^= C;
	SM3Contex->Digest.D[3] ^= D;
	SM3Contex->Digest.D[4] ^= E;
	SM3Contex->Digest.D[5] ^= F;
	SM3Contex->Digest.D[6] ^= G;
	SM3Contex->Digest.D[7] ^= H;


}

void Sm3Init(TTSM3Contex* SM3Contex)
{
	SM3Contex->Digest.D[0] = 0x7380166f;
	SM3Contex->Digest.D[1] = 0x4914b2b9;
	SM3Contex->Digest.D[2] = 0x172442d7;
	SM3Contex->Digest.D[3] = 0xda8a0600;
	SM3Contex->Digest.D[4] = 0xa96f30bc;
	SM3Contex->Digest.D[5] = 0x163138aa;
	SM3Contex->Digest.D[6] = 0xe38dee4d;
	SM3Contex->Digest.D[7] = 0xb0fb0e4e;
	SM3Contex->Size = 0;
	SM3Contex->TotalSize = 0;
}
void Sm3Updata(TTSM3Contex* SM3Contex, uint8_t* Data, uint64_t DataLen)
{
	uint64_t n = 0;
	while (DataLen>0)
	{
		n = min(DataLen, 64 - SM3Contex->Size);
		memcpy(SM3Contex->Data.buffer + SM3Contex->Size, Data, n);
		DataLen -= n;
		SM3Contex->Size += n;
		SM3Contex->TotalSize += n;
		Data = Data + n;
		if (SM3Contex->Size == 64)
		{
			ProcessBlock(SM3Contex);
			SM3Contex->Size = 0;
		}
	}
}
void Sm3Final(TTSM3Contex* SM3Contex, uint8_t* Digest)
{
	uint8_t PadSize = 0;
	uint64_t allLen = SM3Contex->TotalSize * 8;
	if (SM3Contex->Size < 56)
	{
		PadSize = 56 - SM3Contex->Size;
	}
	else
	{
		PadSize = 64 - SM3Contex->Size + 56;
	}
	Sm3Updata(SM3Contex, padding, PadSize);
	//memcpy(SM3Contex->Data.buffer + SM3Contex->Size, padding, PadSize);
	SM3Contex->Data.W[14] = htobe32(allLen >> 32);
	SM3Contex->Data.W[15] = htobe32(allLen);
	ProcessBlock(SM3Contex);

	for (int i = 0; i < 8; i++)
	{
		SM3Contex->Digest.D[i] = htobe32(SM3Contex->Digest.D[i]);
	}
	memcpy(Digest, SM3Contex->Digest.Digest, 32);

}




