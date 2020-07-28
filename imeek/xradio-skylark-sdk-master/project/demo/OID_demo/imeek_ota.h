#ifndef _IMEEK_OTA_
#define _IMEEK_OTA_

#include "ycq_list.h"

#ifndef RETURN_TRUE
#define RETURN_TRUE    0

#define RETURN_ERROR_1 1

//����2:רָС���˶����˲������˶��ĵط�
#define RETURN_ERROR_2 2

//����3:רָ��Χ�����˵�ͼ�߽�
#define RETURN_ERROR_3 3 
#endif

typedef enum{
    OTA_DETECT,
    OTA_CONTINUE,
	OTA_END
}ENUM_OTA_STATE;
extern ENUM_OTA_STATE ota_end_flag;

typedef enum{
	HTTP_OTA,
	SD_OTA,
}ENUM_OTA_MODE;
extern ENUM_OTA_MODE g_strOtaMode;

typedef enum{
	NOT_COMMUNICATE,     //����Ҫ�����������
	COMMUNICATE_ERROR,   //����������������г��ִ���
	COMMUNICATE_ONE_END, //������������������һ�ȡ��һ����������
	COMMUNICATE_ALL_END, //�����������������ȫ�����
}ENUM_HTTP_OTA_STATE;

typedef struct{
    ENUM_HTTP_OTA_STATE DetectFlag;   //�����ж��Ƿ��������������
	int   Ret;                        //url��md5��ʹ�����ǵ�free
	char *url;
	char *md5;
	int   version;
}STRUCT_HTTP_OTA;
extern STRUCT_HTTP_OTA CodeOtaStruct;

#define AUDIO_NUM      30
typedef struct{
    ENUM_HTTP_OTA_STATE DetectFlag;   //�����ж��Ƿ��������������
    int   Ret;
	int   versionCode;
	char *prefix;
	int   next;                       //�Ƿ�����һ������
	int   audio_num;
	char *audio_name[AUDIO_NUM];
	char *md5[AUDIO_NUM];
	char *audio_path[AUDIO_NUM];
}STRUCT_AUDIO_OTA;
extern STRUCT_AUDIO_OTA AudioOtaStruct;

typedef struct{
	List L_name;
	List L_md5;
}STRUCT_AUDIO_TABLE;
extern STRUCT_AUDIO_TABLE LocalAudioTable;


extern int OtaTaskCreate(void);
extern void SetAudioVersion(uint32_t *version);
extern void GetAudioVersion(uint32_t *version);
extern void SetCodeVersion(uint32_t *version);
extern void GetCodeVersion(uint32_t *version);
extern void GetOtaMode(ENUM_OTA_MODE *mode);
extern void WaitOtaEnd(void);

#endif
