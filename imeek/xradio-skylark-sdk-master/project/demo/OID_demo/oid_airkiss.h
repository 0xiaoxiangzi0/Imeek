#ifndef _OID_AIRKISS_H_
#define _OID_AIRKISS_H_

#define SSID_PWD_VALID   0
#define SSID_PWD_NOVALID 1

typedef struct
{
    int     flag;  //��ʾflash�е�ssid��pwd�Ƿ���Ч��  0:��Ч    ����ֵ:��Ч
	uint8_t ssid[64];
	uint8_t pwd[64];
}STRUCT_SSID_PWD;


void oid_airkiss_link(STRUCT_SSID_PWD *l_ssid_pwd);

#endif
