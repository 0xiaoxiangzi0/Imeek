#ifndef _OID_NETWORK_CONNECT_H_
#define _OID_NETWORK_CONNECT_H_

#include "oid_airkiss.h"

#ifndef _BOOL_
#define _BOOL_
typedef unsigned char MYBOOL;
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif



//typedef enum{
//	GET_SSID_FAIL,
//	GET_SSID_FROM_FLASH,  
//	GET_SSID_FROM_AIRKISS,
//}ENUM_GET_SSID_METHOD;

typedef enum{
	NETWORK_DISCONNECT,  //网络断开
	NETWORK_CONNECT,     //网络已连接
}ENUM_NETWORK_STATE;

typedef struct{
    MYBOOL flag;     //TRUE:联网阶段结束	FALSE:联网阶段未结束，或还没开始
    ENUM_NETWORK_STATE network_state;
}STRUCT_NETWORK_EVENT;

extern STRUCT_NETWORK_EVENT imeek_network_event;

#define NET_TO_CONNECT		       (1 << 0)
#define NET_CONNECT_SUCCESS        (1 << 1)
#define NET_CONNECT_FAIL           (1 << 2)
#define NET_DOWN                   (1 << 3)

extern MYBOOL AirKissFlag;


int network_task_create(void);
int GetSsidPwdTaskCreate(void);

#endif
