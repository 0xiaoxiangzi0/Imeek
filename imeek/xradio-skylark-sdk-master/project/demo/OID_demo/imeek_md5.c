#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel/os/os.h"
#include "fs/fatfs/ff.h"
#include "common/framework/platform_init.h"
#include "common/framework/fs_ctrl.h"
#include "imeek_md5.h"

/* forward declaration */
static void ImeekTransform(); 
static unsigned char ImeekPADDING[64] = 
{	
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* F, G and H are basic MD5 functions: selection, majority, parity */
#define IMEEK_F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define IMEEK_G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define IMEEK_H(x, y, z) ((x) ^ (y) ^ (z))
#define IMEEK_I(x, y, z) ((y) ^ ((x) | (~z)))  

/* ROTATE_LEFT rotates x left n bits */
#define IMEEK_ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n)))) 

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define IMEEK_FF(a, b, c, d, x, s, ac) \
	{(a) += IMEEK_F ((b), (c), (d)) + (x) + (UINT4)(ac); \
	 (a) = IMEEK_ROTATE_LEFT ((a), (s)); \
	 (a) += (b); \
	}

#define IMEEK_GG(a, b, c, d, x, s, ac) \
	{(a) += IMEEK_G ((b), (c), (d)) + (x) + (UINT4)(ac); \
	 (a) = IMEEK_ROTATE_LEFT ((a), (s)); \
	 (a) += (b); \
	}

#define IMEEK_HH(a, b, c, d, x, s, ac) \
	{(a) += IMEEK_H ((b), (c), (d)) + (x) + (UINT4)(ac); \
	 (a) = IMEEK_ROTATE_LEFT ((a), (s)); \
	 (a) += (b); \
	}

#define IMEEK_II(a, b, c, d, x, s, ac) \
	{(a) += IMEEK_I ((b), (c), (d)) + (x) + (UINT4)(ac); \
	 (a) = IMEEK_ROTATE_LEFT ((a), (s)); \
	 (a) += (b); \
	}

void ImeekMD5Init(IMEEK_MD5_CTX *mdContext)
{	
    mdContext->i[0] = mdContext->i[1] = (UINT4)0;
	/* Load magic initialization constants.	*/
	mdContext->buf[0] = (UINT4)0x67452301;
	mdContext->buf[1] = (UINT4)0xefcdab89;
	mdContext->buf[2] = (UINT4)0x98badcfe;
	mdContext->buf[3] = (UINT4)0x10325476;
}

void ImeekMD5Update(IMEEK_MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen)
{
    UINT4 in[16];
	int mdi;
	unsigned int i, ii;
	/* compute number of bytes mod 64 */
	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);
	/* update number of bits */
	if ((mdContext->i[0] + ((UINT4)inLen << 3)) < mdContext->i[0])
		mdContext->i[1]++;	mdContext->i[0] += ((UINT4)inLen << 3);
	mdContext->i[1] += ((UINT4)inLen >> 29);
	while (inLen--) {
		/* add new character to buffer, increment mdi */
		mdContext->in[mdi++] = *inBuf++;
		/* transform if necessary */
		if (mdi == 0x40) {
			for (i = 0, ii = 0; i < 16; i++, ii += 4)
				in[i] = (((UINT4)mdContext->in[ii + 3]) << 24) |
				        (((UINT4)mdContext->in[ii + 2]) << 16) |
				        (((UINT4)mdContext->in[ii + 1]) << 8) |
				         ((UINT4)mdContext->in[ii]);
			ImeekTransform(mdContext->buf, in);
			mdi = 0;
		}
	}
}

void ImeekMD5Final(IMEEK_MD5_CTX *mdContext)
{
	UINT4 in[16];
	int mdi;
	unsigned int i, ii;
	unsigned int padLen;
 
	/* save number of bits */
	in[14] = mdContext->i[0];
	in[15] = mdContext->i[1];
 
	/* compute number of bytes mod 64 */
	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);
 
	/* pad out to 56 mod 64 */
	padLen = (mdi < 56) ? (56 - mdi) : (120 - mdi);
	ImeekMD5Update(mdContext, ImeekPADDING, padLen);
	/* append length in bits and transform */
	for (i = 0, ii = 0; i < 14; i++, ii += 4)
		in[i] = (((UINT4)mdContext->in[ii + 3]) << 24) |
		(((UINT4)mdContext->in[ii + 2]) << 16) |
		(((UINT4)mdContext->in[ii + 1]) << 8) |
		((UINT4)mdContext->in[ii]);
	ImeekTransform(mdContext->buf, in);
 
	/* store buffer in digest */
	for (i = 0, ii = 0; i < 4; i++, ii += 4) {
		mdContext->digest[ii] = (unsigned char)(mdContext->buf[i] & 0xFF);
		mdContext->digest[ii + 1] =
			(unsigned char)((mdContext->buf[i] >> 8) & 0xFF);
		mdContext->digest[ii + 2] =
			(unsigned char)((mdContext->buf[i] >> 16) & 0xFF);
		mdContext->digest[ii + 3] =
			(unsigned char)((mdContext->buf[i] >> 24) & 0xFF);
	}
}

/* Basic MD5 step. Transform buf based on in.*/
static void ImeekTransform(UINT4 *buf, UINT4 *in)
{	
    UINT4 a = buf[0], b = buf[1], c = buf[2], d = buf[3];
/* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22	
	IMEEK_FF(a, b, c, d, in[0], S11, 3614090360); /* 1 */
	IMEEK_FF(d, a, b, c, in[1], S12, 3905402710); /* 2 */
	IMEEK_FF(c, d, a, b, in[2], S13, 606105819); /* 3 */
	IMEEK_FF(b, c, d, a, in[3], S14, 3250441966); /* 4 */
	IMEEK_FF(a, b, c, d, in[4], S11, 4118548399); /* 5 */
	IMEEK_FF(d, a, b, c, in[5], S12, 1200080426); /* 6 */
	IMEEK_FF(c, d, a, b, in[6], S13, 2821735955); /* 7 */
	IMEEK_FF(b, c, d, a, in[7], S14, 4249261313); /* 8 */
	IMEEK_FF(a, b, c, d, in[8], S11, 1770035416); /* 9 */
	IMEEK_FF(d, a, b, c, in[9], S12, 2336552879); /* 10 */
	IMEEK_FF(c, d, a, b, in[10], S13, 4294925233); /* 11 */
	IMEEK_FF(b, c, d, a, in[11], S14, 2304563134); /* 12 */
	IMEEK_FF(a, b, c, d, in[12], S11, 1804603682); /* 13 */
	IMEEK_FF(d, a, b, c, in[13], S12, 4254626195); /* 14 */
	IMEEK_FF(c, d, a, b, in[14], S13, 2792965006); /* 15 */
	IMEEK_FF(b, c, d, a, in[15], S14, 1236535329); /* 16 */
/* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20	
	IMEEK_GG(a, b, c, d, in[1], S21, 4129170786); /* 17 */
	IMEEK_GG(d, a, b, c, in[6], S22, 3225465664); /* 18 */
	IMEEK_GG(c, d, a, b, in[11], S23, 643717713); /* 19 */
	IMEEK_GG(b, c, d, a, in[0], S24, 3921069994); /* 20 */
	IMEEK_GG(a, b, c, d, in[5], S21, 3593408605); /* 21 */
	IMEEK_GG(d, a, b, c, in[10], S22, 38016083); /* 22 */
	IMEEK_GG(c, d, a, b, in[15], S23, 3634488961); /* 23 */
	IMEEK_GG(b, c, d, a, in[4], S24, 3889429448); /* 24 */
	IMEEK_GG(a, b, c, d, in[9], S21, 568446438); /* 25 */
	IMEEK_GG(d, a, b, c, in[14], S22, 3275163606); /* 26 */
	IMEEK_GG(c, d, a, b, in[3], S23, 4107603335); /* 27 */
	IMEEK_GG(b, c, d, a, in[8], S24, 1163531501); /* 28 */
	IMEEK_GG(a, b, c, d, in[13], S21, 2850285829); /* 29 */
	IMEEK_GG(d, a, b, c, in[2], S22, 4243563512); /* 30 */
	IMEEK_GG(c, d, a, b, in[7], S23, 1735328473); /* 31 */
	IMEEK_GG(b, c, d, a, in[12], S24, 2368359562); /* 32 */
/* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23	
	IMEEK_HH(a, b, c, d, in[5], S31, 4294588738); /* 33 */
	IMEEK_HH(d, a, b, c, in[8], S32, 2272392833); /* 34 */
	IMEEK_HH(c, d, a, b, in[11], S33, 1839030562); /* 35 */
	IMEEK_HH(b, c, d, a, in[14], S34, 4259657740); /* 36 */
	IMEEK_HH(a, b, c, d, in[1], S31, 2763975236); /* 37 */
	IMEEK_HH(d, a, b, c, in[4], S32, 1272893353); /* 38 */
	IMEEK_HH(c, d, a, b, in[7], S33, 4139469664); /* 39 */
	IMEEK_HH(b, c, d, a, in[10], S34, 3200236656); /* 40 */
	IMEEK_HH(a, b, c, d, in[13], S31, 681279174); /* 41 */
	IMEEK_HH(d, a, b, c, in[0], S32, 3936430074); /* 42 */
	IMEEK_HH(c, d, a, b, in[3], S33, 3572445317); /* 43 */
	IMEEK_HH(b, c, d, a, in[6], S34, 76029189); /* 44 */
	IMEEK_HH(a, b, c, d, in[9], S31, 3654602809); /* 45 */
	IMEEK_HH(d, a, b, c, in[12], S32, 3873151461); /* 46 */
	IMEEK_HH(c, d, a, b, in[15], S33, 530742520); /* 47 */
	IMEEK_HH(b, c, d, a, in[2], S34, 3299628645); /* 48 */
/* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21	
	IMEEK_II(a, b, c, d, in[0], S41, 4096336452); /* 49 */
	IMEEK_II(d, a, b, c, in[7], S42, 1126891415); /* 50 */
	IMEEK_II(c, d, a, b, in[14], S43, 2878612391); /* 51 */
	IMEEK_II(b, c, d, a, in[5], S44, 4237533241); /* 52 */
	IMEEK_II(a, b, c, d, in[12], S41, 1700485571); /* 53 */
	IMEEK_II(d, a, b, c, in[3], S42, 2399980690); /* 54 */
	IMEEK_II(c, d, a, b, in[10], S43, 4293915773); /* 55 */
	IMEEK_II(b, c, d, a, in[1], S44, 2240044497); /* 56 */
	IMEEK_II(a, b, c, d, in[8], S41, 1873313359); /* 57 */
	IMEEK_II(d, a, b, c, in[15], S42, 4264355552); /* 58 */
	IMEEK_II(c, d, a, b, in[6], S43, 2734768916); /* 59 */
	IMEEK_II(b, c, d, a, in[13], S44, 1309151649); /* 60 */
	IMEEK_II(a, b, c, d, in[4], S41, 4149444226); /* 61 */
	IMEEK_II(d, a, b, c, in[11], S42, 3174756917); /* 62 */
	IMEEK_II(c, d, a, b, in[2], S43, 718787259); /* 63 */
	IMEEK_II(b, c, d, a, in[9], S44, 3951481745); /* 64 */ 
	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

/*
*******************************************************************************
 *Function:     ImeekMD5string
 *Description:  对字符串进行MD5运算
 *Calls:       
 *Called By:   
 *Input:        *src:     字符串首地址
 *              md5_len:  md5结果是16位还是32位字符串
 *Output:       *md5_result:求出的MD5字符串存放首地址
 *Return:      
 *Others:       
*******************************************************************************
*/
void ImeekMD5string(char *md5_result,unsigned char *src, int md5_len)
{
	IMEEK_MD5_CTX mdContext;
	int i; 

    if(NULL == src){
	    return;
	}
	
    ImeekMD5Init(&mdContext);

	while(*src != '\0'){
		ImeekMD5Update(&mdContext, src, 1);
		src++;
	}
	ImeekMD5Final(&mdContext);
	memset(md5_result, 0, (md5_len + 1));
	if (md5_len == 16){
		for (i = 4; i < 12; i++){
			sprintf(&md5_result[(i - 4) * 2], "%02x", mdContext.digest[i]);
		}
	}
	else if (md5_len == 32){
		for (i = 0; i < 16; i++){
			sprintf(&md5_result[i * 2], "%02x", mdContext.digest[i]);
		}
	}
}


/*
*******************************************************************************
 *Function:     ImeekMD5File
 *Description:  对文件进行MD5运算
 *Calls:       
 *Called By:   
 *Input:        *path:    文件路径
 *              md5_len:  md5结果是16位还是32位字符串
 *              *md5_result:求出的MD5字符串存放首地址
 *Output:       
 *Return:      
 *Others:       
*******************************************************************************
*/
void ImeekMD5File(char *md5_result, const char *path, int md5_len)
{
	FRESULT res;
    FIL fp;
	IMEEK_MD5_CTX mdContext;
	UINT br;		 /* File read/write count */
	unsigned char data[256];
	int i; 
	
//	printf("ImeekMD5File beganing\n");
	res = f_open(&fp, path, FA_READ);
	if (res != FR_OK){
//		printf("open file fail, %d\n", res);
		return;
	}
	ImeekMD5Init(&mdContext);

	while(1){
	    res = f_read(&fp, data, 256, &br);
		if (res || br == 0){
			break; /* error or eof */
		}
		ImeekMD5Update(&mdContext, data, br);
	}
	
	ImeekMD5Final(&mdContext);
	memset(md5_result, 0, (md5_len + 1));
	if (md5_len == 16){
		for (i = 4; i < 12; i++){
			sprintf(&md5_result[(i - 4) * 2], "%02x", mdContext.digest[i]);
		}
	}
	else if (md5_len == 32){
		for (i = 0; i < 16; i++){
			sprintf(&md5_result[i * 2], "%02x", mdContext.digest[i]);
		}
	}

	f_close(&fp);
}

