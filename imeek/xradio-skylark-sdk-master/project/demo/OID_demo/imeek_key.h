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
 *以下完成对长按，短按，多击的定义
 **********************************************************
 */

typedef enum{
	KEY_NO_VALID,
    KEY_VALID
}ENUM_KEY_STATE;

      //如果按键是ADC型，则需要使能, 1:使能   0:失能
#define KEY_ADC_ENABLE  1         

#define KEY_TYPE        0    //1:表示上拉型按键设计   0：表示下拉型按键设计
#define KEY_SCAN_PERIOD 50  //使用过程中的按键扫描周期(单位毫秒)
#define KEY_CONT_TIME   2000 //定义按键按多久算长按(单位毫秒)
#define KEY_INTERVAL    300  //定义两次按键间隔多少秒以内算连按(单位毫秒)
#define KEY_MUL_CLICK   2    //定义连击多少次算完成
#define KEY_NUM         3    //该应用场景中共有几个按键(目前版本最多支持8个)

typedef struct {
#if KEY_ADC_ENABLE
    u8 KeyPress;         //如果按键为ADC型，则统一使用下拉型按键处理
#else
    GPIO_Port GpioX;
    GPIO_Pin GpioPin;
#endif
    u8 KeyType;                      //1:上拉型按键     0:下拉型按键:标识不按按键时，读取到低电平
    u8 State;                        //[1:0]:Trg,Cont
    ENUM_KEY_STATE ShortPressFlag;   //短按
    u8 KeyPressTimes;                //短按次数
    ENUM_KEY_STATE KeyMulClickFlag;  //完成连击
    ENUM_KEY_STATE LongPressFlag;    //长按
    u8 LoosenFlag   ;                //松手标志位
    u16 Interval;                    //距离上一次按键时间
    u16 ContinueTime;                //按键已持续多久
    u16 ContTimeToPress;             //按键持续按多久算长按
}STRUCT_KEY;

extern STRUCT_KEY g_strMultifunctionKey;
extern STRUCT_KEY g_strWifiResetKey;
extern STRUCT_KEY g_strTouchKey;
extern STRUCT_KEY *g_arrkey[KEY_NUM] ;

extern int GetKeyTaskCreat(void);

#endif


