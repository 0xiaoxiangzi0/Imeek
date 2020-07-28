#ifndef __SOCKET_CONNECT_H__
#define __SOCKET_CONNECT_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "stdio.h"
#include "string.h"

#define SOCKET_MSG_DBG_ON				1
#define SOCKET_MSG_INF_ON				1
#define SOCKET_MSG_WRN_ON				1
#define SOCKET_MSG_ERR_ON				1
#define SOCKET_MSG_ABORT_ON				0
#define SOCKET_MSG_SYSLOG				printf
#define SOCKET_MSG_ABORT()				do { } while (0)

#define SOCKET_MSG_LOG(flags, fmt, arg...)	\
    do {									\
        if (flags)							\
            SOCKET_MSG_SYSLOG(fmt, ##arg);	\
    } while (0)
#define SOCKET_MSG_DBG(fmt, arg...)	\
    SOCKET_MSG_LOG(SOCKET_MSG_DBG_ON, "[SOCKET MSG DBG] "fmt, ##arg)
#define SOCKET_MSG_INFO(fmt, arg...) \
    SOCKET_MSG_LOG(SOCKET_MSG_INF_ON, "[SOCKET MSG INF] "fmt, ##arg)
#define SOCKET_MSG_WRN(fmt, arg...)	\
    SOCKET_MSG_LOG(SOCKET_MSG_WRN_ON, "[SOCKET MSG WRN] "fmt, ##arg)
#define SOCKET_MSG_ERR(fmt, arg...)								\
    do {														\
        SOCKET_MSG_LOG(SOCKET_MSG_ERR_ON, "[SOCKET MSG ERR] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);						\
        if (SOCKET_MSG_ABORT_ON)								\
            SOCKET_MSG_ABORT();									\
    } while (0)

#define REMOTE_IP   "192.168.1.112"
#define REMOTE_PORT 8087



void socket_task_init(uint8_t *ssid, uint8_t ssid_len, uint8_t *psk);
int  socket_task_create(void);
void socket_task_detele(void);


#ifdef __cplusplus
}
#endif

#endif /* __SOCKET_CONNECT_H__ */
