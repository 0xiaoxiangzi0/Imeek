#ifndef _IMEEK_KEY_H_
#define _IMEEK_KEY_H_

typedef signed char s8,S8;
typedef signed short s16,S16;
typedef signed int s32,S32;
typedef unsigned char u8,U8;
typedef unsigned short u16,U16;
typedef unsigned int u32,U32;

/*
 **********************************************************
 *������ɶԳ������̰�������Ķ���
 **********************************************************
 */

typedef enum{
	KEY_NO_VALID,
    KEY_VALID
}ENUM_KEY_STATE;

      //���������ADC�ͣ�����Ҫʹ��, 1:ʹ��   0:ʧ��
#define KEY_ADC_ENABLE  1         

#define KEY_TYPE        0    //1:��ʾ�����Ͱ������   0����ʾ�����Ͱ������
#define KEY_SCAN_PERIOD 50  //ʹ�ù����еİ���ɨ������(��λ����)
#define KEY_CONT_TIME   2000 //���尴��������㳤��(��λ����)
#define KEY_INTERVAL    300  //�������ΰ����������������������(��λ����)
#define KEY_MUL_CLICK   2    //�����������ٴ������
#define KEY_NUM         3    //��Ӧ�ó����й��м�������(Ŀǰ�汾���֧��8��)

typedef struct {
#if KEY_ADC_ENABLE
    u8 KeyPress;         //�������ΪADC�ͣ���ͳһʹ�������Ͱ�������
#else
    GPIO_Port GpioX;
    GPIO_Pin GpioPin;
#endif
    u8 KeyType;                      //1:�����Ͱ���     0:�����Ͱ���:��ʶ��������ʱ����ȡ���͵�ƽ
    u8 State;                        //[1:0]:Trg,Cont
    ENUM_KEY_STATE ShortPressFlag;   //�̰�
    u8 KeyPressTimes;                //�̰�����
    ENUM_KEY_STATE KeyMulClickFlag;  //�������
    ENUM_KEY_STATE LongPressFlag;    //����
    u8 LoosenFlag   ;                //���ֱ�־λ
    u16 Interval;                    //������һ�ΰ���ʱ��
    u16 ContinueTime;                //�����ѳ������
    u16 ContTimeToPress;             //��������������㳤��
}STRUCT_KEY;

extern STRUCT_KEY g_strMultifunctionKey;
extern STRUCT_KEY g_strWifiResetKey;
extern STRUCT_KEY g_strTouchKey;
extern STRUCT_KEY *g_arrkey[KEY_NUM] ;

extern int GetKeyTaskCreat(void);

#endif


