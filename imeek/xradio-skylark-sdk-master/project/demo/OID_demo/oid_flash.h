#ifndef _OID_FLASH_H_
#define _OID_FLASH_H_

#include "stdio.h"
#include "string.h"
#include "oid_airkiss.h"
#include "app.h"

#define FLASH_MSG_DBG_ON				0
#define FLASH_MSG_INF_ON				1
#define FLASH_MSG_WRN_ON				1
#define FLASH_MSG_ERR_ON				1
#define FLASH_MSG_ABORT_ON			    0
#define FLASH_MSG_SYSLOG				printf
#define FLASH_MSG_ABORT()				do { } while (0)

#define FLASH_MSG_LOG(flags, fmt, arg...) \
    do {								\
        if (flags)						\
            FLASH_MSG_SYSLOG(fmt, ##arg);	\
    } while (0)
#define FLASH_MSG_DBG(fmt, arg...)	\
    FLASH_MSG_LOG(FLASH_MSG_DBG_ON, "[FLASH MSG DBG] "fmt, ##arg)
#define FLASH_MSG_INF(fmt, arg...)	\
    FLASH_MSG_LOG(FLASH_MSG_INF_ON, "[FLASH MSG INF] "fmt, ##arg)
#define FLASH_MSG_WRN(fmt, arg...)	\
    FLASH_MSG_LOG(FLASH_MSG_WRN_ON, "[FLASH MSG WRN] "fmt, ##arg)
#define FLASH_MSG_ERR(fmt, arg...)								 \
    do {														 \
        FLASH_MSG_LOG(FLASH_MSG_ERR_ON, "[FLASH MSG ERR] %s():%d, "fmt,\
               __func__, __LINE__, ##arg);						 \
        if (FLASH_MSG_ABORT_ON)									 \
            FLASH_MSG_ABORT();									 \
    } while (0)


#define FLASH_USER_DATA_START_ADDR 0xFF000
#define FLASH_WLAN_SSID_PWD_ADDR   0xFF400
/**********************************************/
#define FLASH_STATE_ADDR           0XFF484

//����Ϊ�ýṹ��ÿ�����ݵ��׵�ַ������ʵ�ֵ�����д
#define FLASH_CODE_VERSION_ADDR    0XFF484//����汾
#define FLASH_AUDIO_VERSION_ADDR   0XFF488//��Ƶ�汾
#define FLASH_MISSION_LEVEL_ADDR   0XFF48C//���񵽵ڼ���
#define FLASH_MATH_LEVEL_ADDR      0XFF490//��ѧ���ڼ���
#define FLASH_VOLUME_LEVEL_ADDR    0XFF494//�����ȼ�
#define FLASH_EYES_STATE_ADDR      0XFF498//�۾���
#define FLASH_BADGE_1_ADDR         0XFF49C//���»�ȡ״̬1
#define FLASH_BADGE_2_ADDR         0XFF4A0//���»�ȡ״̬2 
#define FLASH_TASK_PROGRESS1_ADDR  0XFF4A4//�������1
#define FLASH_TASK_PROGRESS2_ADDR  0XFF4A8//�������2
#define FLASH_TASK_PROGRESS3_ADDR  0XFF4AC//�������3
#define FLASH_TASK_PROGRESS4_ADDR  0XFF4B0//�������4
#define FLASH_TASK_PROGRESS5_ADDR  0XFF4B4//�������5
#define FLASH_TASK_PROGRESS6_ADDR  0XFF4B8//�������6
#define FLASH_TASK_PROGRESS7_ADDR  0XFF4BC//�������7
#define FLASH_TASK_PROGRESS8_ADDR  0XFF4C0//�������8
/***********************************************/
#define FLASH_IMEEK_ID_ADDR        0xFF4C4//IMEEK_ID�ĵ�ַ

#define FLASH_PUBLIC_AUDIO_VERSION_ADDR  0xFF4CC //������Ƶ�汾�׵�ַ
#define FLASH_PAINT1_AUDIO_VERSION_ADDR  0xFF4D0 //�汾1��Ƶ�汾�׵�ַ
#define FLASH_PAINT2_AUDIO_VERSION_ADDR  0xFF4D4 //�汾2��Ƶ�汾�׵�ַ
#define FLASH_PAINT3_AUDIO_VERSION_ADDR  0xFF4D8 //�汾3��Ƶ�汾�׵�ַ
#define FLASH_PAINT4_AUDIO_VERSION_ADDR  0xFF4DC //�汾4��Ƶ�汾�׵�ַ
#define FLASH_PAINT5_AUDIO_VERSION_ADDR  0xFF4E0 //�汾5��Ƶ�汾�׵�ַ
#define FLASH_PAINT6_AUDIO_VERSION_ADDR  0xFF4E4 //�汾6��Ƶ�汾�׵�ַ
#define FLASH_PAINT7_AUDIO_VERSION_ADDR  0xFF4E8 //�汾7��Ƶ�汾�׵�ַ
#define FLASH_PAINT8_AUDIO_VERSION_ADDR  0xFF4EC //�汾8��Ƶ�汾�׵�ַ
#define FLASH_PAINT9_AUDIO_VERSION_ADDR  0xFF4F0 //�汾9��Ƶ�汾�׵�ַ
#define FLASH_PAINT10_AUDIO_VERSION_ADDR 0xFF4F4 //�汾10��Ƶ�汾�׵�ַ
#define FLASH_MATH1_AUDIO_VERSION_ADDR   0xFF4F8 //��ѧ1��Ƶ�汾�׵�ַ
#define FLASH_MATH2_AUDIO_VERSION_ADDR   0xFF4FC //��ѧ2��Ƶ�汾�׵�ַ
#define FLASH_MATH3_AUDIO_VERSION_ADDR   0xFF500 //��ѧ3��Ƶ�汾�׵�ַ
#define FLASH_MATH4_AUDIO_VERSION_ADDR   0xFF504 //��ѧ4��Ƶ�汾�׵�ַ
#define FLASH_MATH5_AUDIO_VERSION_ADDR   0xFF508 //��ѧ5��Ƶ�汾�׵�ַ
#define FLASH_MATH6_AUDIO_VERSION_ADDR   0xFF50C //��ѧ6��Ƶ�汾�׵�ַ
#define FLASH_MATH7_AUDIO_VERSION_ADDR   0xFF510 //��ѧ7��Ƶ�汾�׵�ַ
#define FLASH_MATH8_AUDIO_VERSION_ADDR   0xFF514 //��ѧ8��Ƶ�汾�׵�ַ
#define FLASH_MATH9_AUDIO_VERSION_ADDR   0xFF518 //��ѧ9��Ƶ�汾�׵�ַ
#define FLASH_MATH10_AUDIO_VERSION_ADDR  0xFF51C //��ѧ10��Ƶ�汾�׵�ַ
#define FLASH_BADGE_AUDIO_VERSION_ADDR   0xFF520 //���°���Ƶ�汾�׵�ַ
#define FLASH_STICKER_AUDIO_VERSION_ADDR 0xFF524 //��ֽ��Ƶ�汾�׵�ַ
#define FLASH_ENGLISH_AUDIO_VERSION_ADDR 0xFF528 //Ӣ����Ƶ�汾�׵�ַ
#define FLASH_CHINESE_AUDIO_VERSION_ADDR 0xFF52C //���ֿ���Ƶ�汾�׵�ַ
//��һ����ַ0XFF530

extern void flash_ssid_pwd_read(STRUCT_SSID_PWD *int_p);
extern void flash_ssid_pwd_write(STRUCT_SSID_PWD *int_p);
extern void flash_state_read(STRUCT_FLASH_STATE *int_p);
extern void flash_state_write(STRUCT_FLASH_STATE *int_p);
extern void flash_int_read(int start_addr, uint32_t *int_p);
extern void flash_int_write(int start_addr, uint32_t *int_p);
extern void flash_char_read(int start_addr, uint8_t *char_p);
extern void flash_char_write(int start_addr, uint8_t *char_p);
extern void imeek_id_read(STRUCT_IMEEK_ID *int_p);
extern void imeek_id_write(STRUCT_IMEEK_ID *int_p);

#endif
