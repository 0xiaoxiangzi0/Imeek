#ifndef __NET_MSG_H__
#define __NET_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "stdio.h"
#include "string.h"

#define NET_MSG_DBG_ON				0
#define NET_MSG_INF_ON				1
#define NET_MSG_WRN_ON				1
#define NET_MSG_ERR_ON				1
#define NET_MSG_ABORT_ON			0
#define NET_MSG_SYSLOG				printf
#define NET_MSG_ABORT()				do { } while (0)

#define NET_MSG_LOG(flags, fmt, arg...) \
    do {								\
        if (flags)						\
            NET_MSG_SYSLOG(fmt, ##arg);	\
    } while (0)
#define NET_MSG_DBG(fmt, arg...)	\
    NET_MSG_LOG(NET_MSG_DBG_ON, "[NET MSG DBG] "fmt, ##arg)
#define NET_MSG_INF(fmt, arg...)	\
    NET_MSG_LOG(NET_MSG_INF_ON, "[NET MSG INF] "fmt, ##arg)
#define NET_MSG_WRN(fmt, arg...)	\
    NET_MSG_LOG(NET_MSG_WRN_ON, "[NET MSG WRN] "fmt, ##arg)
#define NET_MSG_ERR(fmt, arg...)								\
    do {														\
        NET_MSG_LOG(NET_MSG_ERR_ON, "[NET MSG ERR] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);						\
        if (NET_MSG_ABORT_ON)									\
            NET_MSG_ABORT();									\
    } while (0)


int net_service_task_create(void);

#ifdef __cplusplus
}
#endif

#endif /* __NET_MSG_H__ */