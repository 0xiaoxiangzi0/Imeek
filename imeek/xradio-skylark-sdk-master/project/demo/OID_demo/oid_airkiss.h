#ifndef _OID_AIRKISS_H_
#define _OID_AIRKISS_H_

#define SSID_PWD_VALID   0
#define SSID_PWD_NOVALID 1

typedef struct
{
    int     flag;  //表示flash中的ssid与pwd是否有效，  0:有效    其他值:无效
	uint8_t ssid[64];
	uint8_t pwd[64];
}STRUCT_SSID_PWD;


void oid_airkiss_link(STRUCT_SSID_PWD *l_ssid_pwd);

#endif
