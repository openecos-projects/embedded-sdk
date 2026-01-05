#include "SM3.h"
#include "SM3Test.h"
#include <tinysh.h>
#include <tinyprintf.h>
#include <tinyuart.h>

static int print_digest(unsigned char *digest)
{
	uint32_t i;

	for (i = 0; i < SM3_DIGEST_SIZE; i++)
	{
		printf("%02x", digest[i]);
	}

	return 0;
}



unsigned char *SM3(const unsigned char *d, size_t n, unsigned char *md)
{
	TTSM3Contex c;

	if ((NULL == d) || (NULL == md))
	{
		return NULL;
	}

	Sm3Init(&c);
	Sm3Updata(&c, (uint8_t *)d, n);
	Sm3Final(&c, md);

	return md;
}



static void StandDataTest()
{
	char TestData[]="abc";
	unsigned char digest[SM3_DIGEST_SIZE];

	printf("(\"%s\") = ", TestData);

	SM3(TestData, 3, digest);

	print_digest(digest);
	printf("\n");

}

static int GetString(char* Str)
{
	int DataLen=0;
	char c;
    while ((c=getchar_prompt(0)) != '\r')
    {
		printf("%c",c);
		Str[DataLen++]=c;
    }
	printf("Str= %s,DataLen=%d\n",Str,DataLen);
	return DataLen;
}

static void CustomDatatest()
{
	printf("	Input you data,end with Enter key\n");
	char str[1024 * 10] = { 0 };
	unsigned char digest[SM3_DIGEST_SIZE];
	//scanf("%s", str, 10240);
	int DataLen=GetString(str);
	printf("(\"%s\") = ", str);

	SM3(str, DataLen,digest);

	print_digest(digest);
	printf("\n");
}

void SM3Test(void)
{


    while (1)
    {
        printf("\nSelect an action:\n");
		printf("Return to menu by sending '!'\n");
        printf("   [s] Run standard data test\n");
        printf("   [i] Run input custom data test\n");
        // printf("   [h] print housekeeping spi info\n");
        // printf("   [a] run archinfo test\n");
        // printf("   [r] run rng test\n");
        // printf("   [u] run uart test\n");
        // printf("   [p] run pwm test\n");
        // printf("   [s] run ps2 test\n");
        // printf("   [i] run i2c test\n");
        // printf("   [l] run lcd test\n");
        // printf("   [b] run tiny benchmark\n");
        // printf("   [m] run Memtest\n");
        // printf("   [e] echo uart\n");
        // printf("   [j] run SM3 test\n\n");

        for (int rep = 10; rep > 0; rep--)
        {
			printf("tinysh> ");
            char cmd = getchar_prompt(0);
            if (cmd > 32 && cmd < 127)
                putch(cmd);
            printf("\n");

            switch (cmd)
            {
            case 's':
                StandDataTest();
                break;
			case 'i':
				CustomDatatest();
				break;
			case '!':
				return;
			default:
				break;
			}
		}

	}






}

