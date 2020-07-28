#ifndef __OID_TRANSF_H__
#define __OID_TRANSF_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "stdio.h"
#include "string.h"
#include "oid_protocol.h"

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


#define OID_MSG_DBG_ON				1
#define OID_MSG_INF_ON				1
#define OID_MSG_WRN_ON				1
#define OID_MSG_ERR_ON				1
#define OID_MSG_ABORT_ON			0
#define OID_MSG_SYSLOG				printf
#define OID_MSG_ABORT()				do { } while (0)

#define OID_MSG_LOG(flags, fmt, arg...) \
    do {								\
        if (flags)						\
            OID_MSG_SYSLOG(fmt, ##arg);	\
    } while (0)
#define OID_MSG_DBG(fmt, arg...)	\
    OID_MSG_LOG(OID_MSG_DBG_ON, "[OID MSG DBG] "fmt, ##arg)
#define OID_MSG_INF(fmt, arg...)	\
    OID_MSG_LOG(OID_MSG_INF_ON, "[OID MSG INF] "fmt, ##arg)
#define OID_MSG_WRN(fmt, arg...)	\
    OID_MSG_LOG(OID_MSG_WRN_ON, "[OID MSG WRN] "fmt, ##arg)
#define OID_MSG_ERR(fmt, arg...)								 \
    do {														 \
        OID_MSG_LOG(OID_MSG_ERR_ON, "[OID MSG ERR] %s():%d, "fmt,\
               __func__, __LINE__, ##arg);						 \
        if (OID_MSG_ABORT_ON)									 \
            OID_MSG_ABORT();									 \
    } while (0)


extern OID_DATA_STRUCT oid_data;

int OidDetectionTaskCreate(void);

#ifdef __cplusplus
}
#endif

#endif /* __OID_TRANSF_H__ */

