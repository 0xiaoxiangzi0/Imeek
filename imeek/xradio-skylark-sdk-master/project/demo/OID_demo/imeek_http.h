#ifndef _IMEEK_HTTP_H_
#define _IMEEK_HTTP_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdio.h"
#include "string.h"

#define HTTPC_MSG_DBG_ON				1
#define HTTPC_MSG_INF_ON				1
#define HTTPC_MSG_WRN_ON				1
#define HTTPC_MSG_ERR_ON				1
#define HTTPC_MSG_ABORT_ON				0
#define HTTPC_MSG_SYSLOG				printf
#define HTTPC_MSG_ABORT()				do { } while (0)

#define HTTPC_MSG_LOG(flags, fmt, arg...)	\
    do {									\
        if (flags)							\
            HTTPC_MSG_SYSLOG(fmt, ##arg);	\
    } while (0)
#define HTTPC_MSG_DBG(fmt, arg...)	\
    HTTPC_MSG_LOG(HTTPC_MSG_DBG_ON, "[HTTPC MSG DBG] "fmt, ##arg)
#define HTTPC_MSG_INFO(fmt, arg...) \
    HTTPC_MSG_LOG(HTTPC_MSG_INF_ON, "[HTTPC MSG INF] "fmt, ##arg)
#define HTTPC_MSG_WRN(fmt, arg...)	\
    HTTPC_MSG_LOG(HTTPC_MSG_WRN_ON, "[HTTPC MSG WRN] "fmt, ##arg)
#define HTTPC_MSG_ERR(fmt, arg...)								\
    do {														\
        HTTPC_MSG_LOG(HTTPC_MSG_ERR_ON, "[HTTPC MSG ERR] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);						\
        if (HTTPC_MSG_ABORT_ON)								\
            HTTPC_MSG_ABORT();									\
    } while (0)

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


typedef enum{
    HTTPC_TASK_CREAT_SUCCESS,
    HTTPC_TASK_ALREADY,
	HTTPC_TASK_CREAT_ERROR,
}ENUM_HTTPC_STATE;

extern int http_task_create(void);
int GetAudioFile(char* buf, uint32_t *size,char *url, int use_ssl,int num);
void GetMacAddr(char *addr);

#ifdef __cplusplus
}
#endif

#endif
