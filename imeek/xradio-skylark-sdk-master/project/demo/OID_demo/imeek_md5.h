#ifndef _IMEEK_MD5_H_
#define _IMEEK_MD5_H_

/* typedef a 32 bit type */
typedef unsigned long int UINT4; 
/* Data structure for MD5 (Message Digest) computation */
typedef struct {	
    UINT4 i[2];                   /* number of _bits_ handled mod 2^64 */
	UINT4 buf[4];                 /* scratch buffer */
	unsigned char in[64];         /* input buffer */
	unsigned char digest[16];     /* actual digest after MD5Final call */
} IMEEK_MD5_CTX; 

void ImeekMD5File(char *file_md5, const char *path, int md5_len);
void ImeekMD5string(char *md5_result, unsigned char *src, int md5_len);

#endif
