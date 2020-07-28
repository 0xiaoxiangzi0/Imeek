#ifndef _OID_AUDIO_H_
#define _OID_AUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdio.h"
#include "string.h"
#include "driver/chip/hal_gpio.h"

#define OID_PLAY_MSG_DBG_ON				1
#define OID_PLAY_MSG_INF_ON				1
#define OID_PLAY_MSG_WRN_ON				1
#define OID_PLAY_MSG_ERR_ON				1
#define OID_PLAY_MSG_ABORT_ON			0
#define OID_PLAY_MSG_SYSLOG				printf
#define OID_PLAY_MSG_ABORT()			do { } while (0)

#define OID_PLAY_MSG_LOG(flags, fmt, arg...) \
    do {								\
        if (flags)						\
            OID_PLAY_MSG_SYSLOG(fmt, ##arg);	\
    } while (0)
#define OID_PLAY_MSG_DBG(fmt, arg...)	\
    OID_PLAY_MSG_LOG(OID_PLAY_MSG_DBG_ON, "[OID_PLAY MSG DBG] "fmt, ##arg)
#define OID_PLAY_MSG_INF(fmt, arg...)	\
    OID_PLAY_MSG_LOG(OID_PLAY_MSG_INF_ON, "[OID_PLAY MSG INF] "fmt, ##arg)
#define OID_PLAY_MSG_WRN(fmt, arg...)	\
    OID_PLAY_MSG_LOG(OID_PLAY_MSG_WRN_ON, "[OID_PLAY MSG WRN] "fmt, ##arg)
#define OID_PLAY_MSG_ERR(fmt, arg...)								\
    do {														\
        OID_PLAY_MSG_LOG(OID_PLAY_MSG_ERR_ON, "[OID_PLAY MSG ERR] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);						\
        if (OID_PLAY_MSG_ABORT_ON)									\
            OID_PLAY_MSG_ABORT();									\
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


#ifndef FS_AUDIO_PLAY
#define FS_AUDIO_PLAY   0
#endif

#ifndef HTTP_AUDIO_PLAY
#define HTTP_AUDIO_PLAY 1
#endif

#ifndef OID_PLAY_MODE
#define OID_PLAY_MODE   FS_AUDIO_PLAY
#endif

#define AUDIO_PLAY_PORT	  GPIO_PORT_A
#define AUDIO_PLAY_PIN	  GPIO_PIN_23
#define AUDIO_PLAY_ON()   HAL_GPIO_WritePin(AUDIO_PLAY_PORT, AUDIO_PLAY_PIN, 1)
#define AUDIO_PLAY_OFF()  HAL_GPIO_WritePin(AUDIO_PLAY_PORT, AUDIO_PLAY_PIN, 0)

typedef enum{
    NO_PLAY_AUDIO,
    NEW_PLAY_AUDIO,
}ENUM_PLAY_AUDIO_FLAG;

typedef enum{
	ZERO_LEVEL_AUDIO,      //优先级最高
	FIRST_LEVEL_AUDIO,      
	SECOND_LEVEL_AUDIO,
	THIRD_LEVEL_AUDIO,
	FOURTH_LEVEL_AUDIO,
	FIFTH_LEVEL_AUDIO
}ENUM_PLAY_AUDIO_PRIORITY;

typedef enum{
	IMEEK_PLAY,
	IMEEK_PAUSE,
	IMEEK_STOP,
	IMEEK_NO_STATE
}ENUM_PLAY_TYPE;

typedef struct{
	ENUM_PLAY_TYPE CurrentState;
	ENUM_PLAY_TYPE NewState;
	MYBOOL IsPlayOver;                  //true:表示上一次完整播放 flase:表示上一次未完整播放
	ENUM_PLAY_AUDIO_PRIORITY Priority;  //当前播放音频的优先级
    ENUM_PLAY_AUDIO_FLAG  new_play_audio_flag;
	char  play_audio_url_new[100];
	char  play_audio_url_old[100];
}STRUCT_PLAY_AUDIO;
extern STRUCT_PLAY_AUDIO OidPlayAudio;

int ImeekAudioPlayStart(void);
extern OS_Mutex_t AudioPlayMutex;
uint8_t PlayAudio(char *name, ENUM_PLAY_AUDIO_PRIORITY priority,ENUM_PLAY_TYPE play_type);
void WaitAudioPlayOver(MYBOOL flag);
void AudioPlayToEnd(char *url,ENUM_PLAY_AUDIO_PRIORITY priority,ENUM_PLAY_TYPE play_type);

#ifdef __cplusplus
}
#endif


#endif
