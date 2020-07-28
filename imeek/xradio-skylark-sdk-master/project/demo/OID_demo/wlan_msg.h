#ifndef _WLAN_MSG_H_
#define _WLAN_MSG_H_

#include <string.h>

#define WLAN_MSG_DBG_ON				1
#define WLAN_MSG_INF_ON				1
#define WLAN_MSG_WRN_ON				1
#define WLAN_MSG_ERR_ON				1
#define WLAN_MSG_ABORT_ON			0
#define WLAN_MSG_SYSLOG				printf
#define WLAN_MSG_ABORT()		    do { } while (0)

#define WLAN_MSG_LOG(flags, fmt, arg...) \
    do {								\
        if (flags)						\
            WLAN_MSG_SYSLOG(fmt, ##arg);	\
    } while (0)
#define WLAN_MSG_DBG(fmt, arg...)	\
    WLAN_MSG_LOG(WLAN_MSG_DBG_ON, "[WLAN MSG DBG] "fmt, ##arg)
#define WLAN_MSG_INF(fmt, arg...)	\
    WLAN_MSG_LOG(WLAN_MSG_INF_ON, "[WLAN MSG INF] "fmt, ##arg)
#define WLAN_MSG_WRN(fmt, arg...)	\
    WLAN_MSG_LOG(WLAN_MSG_WRN_ON, "[WLAN MSG WRN] "fmt, ##arg)
#define WLAN_MSG_ERR(fmt, arg...)								\
    do {														\
        WLAN_MSG_LOG(WLAN_MSG_ERR_ON, "[WLAN MSG ERR] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);						\
        if (WLAN_MSG_ABORT_ON)									\
            WLAN_MSG_ABORT();									\
    } while (0)

//int wlan_tread_create(void);

#endif
