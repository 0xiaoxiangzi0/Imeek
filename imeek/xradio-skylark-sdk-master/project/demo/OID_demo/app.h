#ifndef _APP_H_
#define _APP_H_

#include "oid_protocol.h"
//#include "oid_code_distribution.h"

#define ABS(x) ((x) >= 0 ? (x) : (-x))

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


#ifndef RETURN_TRUE
#define RETURN_TRUE      0

#define RETURN_ERROR_1   1

//错误2:专指小车运动到了不允许运动的地方
#define RETURN_ERROR_2   2

//错误3:专指范围超出了地图边界
#define RETURN_ERROR_3   3 

#endif

#define APP_MSG_DBG_ON				0
#define APP_MSG_INF_ON				0
#define APP_MSG_WRN_ON				0
#define APP_MSG_ERR_ON				0
#define APP_MSG_ABORT_ON			0
#define APP_MSG_SYSLOG				printf
#define APP_MSG_ABORT()				do { } while (0)

#define APP_MSG_LOG(flags, fmt, arg...) \
    do {								\
        if (flags)						\
            APP_MSG_SYSLOG(fmt, ##arg);	\
    } while (0)
#define APP_MSG_DBG(fmt, arg...)	\
    APP_MSG_LOG(APP_MSG_DBG_ON, "[APP MSG DBG] "fmt, ##arg)
#define APP_MSG_INF(fmt, arg...)	\
    APP_MSG_LOG(APP_MSG_INF_ON, "[APP MSG INF] "fmt, ##arg)
#define APP_MSG_WRN(fmt, arg...)	\
    APP_MSG_LOG(APP_MSG_WRN_ON, "[APP MSG WRN] "fmt, ##arg)
#define APP_MSG_ERR(fmt, arg...)								 \
    do {														 \
        APP_MSG_LOG(APP_MSG_ERR_ON, "[APP MSG ERR] %s():%d, "fmt,\
               __func__, __LINE__, ##arg);						 \
        if (APP_MSG_ABORT_ON)									 \
            APP_MSG_ABORT();									 \
    } while (0)

typedef enum{
	VARIABLE_INIT,
//	GET_LEVEL,
    GET_INSTRUCTION_START,
	GET_INSTRUCTION_DIRECTION,
	GET_INSTRUCTION_EXECUTE,
	GET_INSTRUCTION_END,
	DETECT_INSTRUCTION_START,
	DETECT_INSTRUCTION_EXECUTE,
	DETECT_INSTRUCTION_END,
	EXECUTE_INSTRUCTION_PREPARE,
	EXECUTE_INSTRUCTION_START,
	EXECUTE_INSTRUCTION_DIRECTION,
	EXECUTE_INSTRUCTION_EXECUTE,
	EXECUTE_INSTRUCTION_END,
}ENUM_CURRENT_WORK_STATE;

typedef enum{
	MEEK_X_AXIS_LOW_TO_HIGH,
	MEEK_X_AXIS_HIGH_TO_LOW,
	MEEK_Y_AXIS_LOW_TO_HIGH,
	MEEK_Y_AXIS_HIGH_TO_LOW,
}ENUM_MEEK_DIRECTION_AXIS;

//表示米小克当前的运行状态
typedef enum{
    IDLE_MODE,
	TASK_MODE,
	FREE_MODE,
	SPELL_MODE,
	MUSIC_EDIT_MODE,
	RECORD_MODE,
	QUESTION_MODE,
    TRAILING_MODE,
    TEST_MODE,
	ALL_MODE  
}ENUM_RUNNING_STATE;
		
typedef struct{
	ENUM_RUNNING_STATE state;
	uint32_t map;
    
	//任务模式才用得到的变量运行的关卡及其对应的目标终点
	uint32_t level;
	uint32_t level_row;
	uint32_t level_column;

    //米小克当前所在的地图块
	uint32_t current_row;
	uint32_t current_column;
    
	//自由模式才用得到的变量:记录自由模式开始时所在地图位置
	uint8_t  start_flag;     //0:表示起始位置无效   1:起始位置有效
	uint32_t start_row;   
	uint32_t start_column;
    
	//获取指令卡阶段需要用到的变量
    ENUM_CURRENT_WORK_STATE current_mode_state;

	//检测阶段使用变量
 	uint8_t block_num;
	
	//用于判断指令卡的循环是否摆放正确
	signed char loop_sequence;

	//执行阶段使用变量
	ENUM_MEEK_DIRECTION_AXIS direction;
}STRUCT_IMEEK_WORK_STATE;

typedef struct{
    ENUM_CODE_TYPE code_flag;      //0:码值无效  1:识别到圈图码  2:识别到位置码
	OID_POSITION_STRUCT position;  //x、y position
	uint32_t index; 			   //圈图码码值
}STRUCT_APP_OID_DATA;

typedef struct{
	uint32_t instruction;
	uint32_t parameter;
}STRUCT_RUN_INSTRUCTION_MESSAGE;

/*
 ************************************************************
 *OID指向的位置
 ************************************************************
*/
typedef enum{
	OID_POINT_TO_NULL,
	OID_POINT_TO_PAINT,
	OID_POINT_TO_MATH,
	OID_POINT_TO_BADGE,
	OID_POINT_TO_STICKER,
	OID_POINT_TO_DOWNLOAD_AUDIO,
	OID_POINT_TO_ENGLISH_WORD,
	OID_POINT_TO_CHINESE_WORD,
}ENUM_OID_POINT_TO;

/*
 ************************************************************
 *按键触发的功能变量
 ************************************************************
*/
typedef enum{
	KEY_TRIGGER_NONE,
	KEY_TRIGGER_TRACKING,
	KEY_TRIGGER_CLEAR_WIFI,
	KEY_TRIGGER_CLEAR_USER,
	KEY_TRIGGER_TEST_MODE
}ENUM_KEY_TRIGGER_IMEEK_STATE;

/*
 ************************************************************
 *音乐创作模式使用到的结构，变量
 ************************************************************
*/
#define COMPOSE_MAX_NUM 256
typedef enum{
	COMPOSE_INIT,
	COMPOSE_READ,
	COMPOSE_PLAY
}ENUM_COMPOSE_STATE;

//typedef struct{
//	ENUM_COMPOSE_STATE state;
//    int      index;                 //最后一个音符的下标，-1:表示当前没有音符
//    char     first[COMPOSE_MAX_NUM];//音符存放处
//}STRUCT_COMPOSE;

/*
 ************************************************************
 *寻迹模式
 ************************************************************
*/
typedef enum{
	RECORD_INIT,
	RECORDING,
	RECORD_OVER
}ENUM_RECORD_STATE;

typedef struct{
	ENUM_RECORD_STATE state;
}STRUCT_IMEEK_RECORD;


/*
 ************************************************************
 *录音模式
 ************************************************************
*/
typedef enum{
	TRAILING_INIT,
	TRAILING,
	TRAILING_PLAY_OVER,
	TRAILING_OVER,
}ENUM_TRAILING_STATE;

typedef struct{
	ENUM_TRAILING_STATE state;
}STRUCT_IMEEK_TRAILING;

/*
 ************************************************************
 *米小克耳朵灯与眼睛灯控制结构体
 ************************************************************
*/
        //灯常亮
#define LIGHT_FOREVER   0xFFFFFF
	
typedef enum{
	LIGHT_RED,
	LIGHT_GREEN,
	LIGHT_BLUE,
	LIGHT_PINKISH,
	LIGHT_YELLOW,
	LIGHT_CYAN,
	LIGHT_WHITE,
}ENUM_LINGT_COLOR;

typedef enum{
	LIGHT_FORM,   //常亮
	LIGHT_BRIGHT, //闪烁
}ENUM_LIGHT_MODE;

typedef enum{
	LIGHT_ON,
	LIGHT_OFF,
}ENUM_LIGHT_STATE;

typedef enum{
	LIGHT_CONTINUE,
	LIGHT_NO_CONTINUE
}ENUM_LIGHT_END;

typedef struct{
	ENUM_LIGHT_STATE light_ear_state;
    ENUM_LINGT_COLOR light_ear_color;
	ENUM_LIGHT_MODE  light_ear_mode;
    ENUM_LIGHT_END   light_ear_end;
    uint32_t         light_ear_time_of_duration;  //1表示0.5s，2表示1秒 
	
	ENUM_LIGHT_STATE light_eyes_state;
	ENUM_LINGT_COLOR light_eyes_color;
	ENUM_LIGHT_MODE  light_eyes_mode;	
    ENUM_LIGHT_END   light_eyes_end;
    uint32_t         light_eyes_time_of_duration; //1表示0.5s，2表示1秒 
}STRUCT_LIGHT;

typedef struct{
	uint32_t LocalCodeVersion;       //软件版本
	uint32_t LocalAudioVersion;      //音频版本
	uint32_t MissionLevel;           //任务到第几关
	uint32_t MathLevel;              //数学到第几关
	uint32_t VolumeLevel;            //音量等级
	uint32_t EyesState;              //眼睛灯
	uint32_t BadgeState1;            //徽章获取状态1
	uint32_t BadgeState2;            //徽章获取状态2 
	uint32_t TaskProgess1;           //任务进度1
	uint32_t TaskProgess2;           //任务进度2
	uint32_t TaskProgess3;           //任务进度3
	uint32_t TaskProgess4;           //任务进度4
	uint32_t TaskProgess5;           //任务进度5
	uint32_t TaskProgess6;           //任务进度6
	uint32_t TaskProgess7;           //任务进度7
	uint32_t TaskProgess8;           //任务进度8
}STRUCT_FLASH_STATE;
extern STRUCT_FLASH_STATE g_strFlashState;

typedef enum{
	PUBLIC_AUDIO,
	PAINT1_AUDIO,
	PAINT2_AUDIO,
	PAINT3_AUDIO,
	PAINT4_AUDIO,
	PAINT5_AUDIO,
	PAINT6_AUDIO,
	PAINT7_AUDIO,
	PAINT8_AUDIO,
	PAINT9_AUDIO,
	PAINT10_AUDIO,
	MATH1_AUDIO,
	MATH2_AUDIO,
	MATH3_AUDIO,
	MATH4_AUDIO,
	MATH5_AUDIO,
	MATH6_AUDIO,
	MATH7_AUDIO,
	MATH8_AUDIO,
	MATH9_AUDIO,
	MATH10_AUDIO,
	BADGE_AUDIO,
	STICKER_AUDIO,
	ENGLISH_AUDIO,
	CHINESE_AUDIO,
	
	AUDIO_TOTAL
}ENUM_ALL_AUDIO_UPDATE_FLAG;

typedef struct{
	uint32_t AudioVersion;
	int      Addr;         //版本信息存放在flash的什么地址上
	char AudioType[15];
	MYBOOL Flag;
}STRUCT_ALL_AUDIO_UPDATE_DATA;
extern STRUCT_ALL_AUDIO_UPDATE_DATA g_aAudioUpdateData[AUDIO_TOTAL];

typedef struct{
	int  SendFlag;  //是否有发送过给服务器，如果有就不在发送
	                //TRUE:已经发送过     FALSE:还没有发送过
	char Id[16];
}STRUCT_IMEEK_ID;
extern STRUCT_IMEEK_ID g_strImeekId;

typedef enum{
	NO_QUESTION,
	MATH_QUESTION,
	PAINT_QUESTION
}ENUM_QUESTION_TYPE;

typedef struct{
	ENUM_QUESTION_TYPE Type; //什么类型问题
	int bit;                 //第几个的问题
	int first_or_two;        //该问题的第几问
	MYBOOL first_times;      //是否第一次答对该问题
}STRUCT_MISSION_STATE;
extern STRUCT_MISSION_STATE g_strMissionState;

//typedef struct{
//	uint8_t left_speed;
//	uint8_t right_speed;
//}STRUCT_IR_STATE;

int AppTaskCreate(void);
int LightTaskCreate(void);
int TrailingModeCreate(void);
//void GetFlashState(STRUCT_FLASH_STATE* FlashState,STRUCT_IMEEK_ID* ImeekId,
//                   STRUCT_SSID_PWD *SsidPwd,STRUCT_ALL_AUDIO_VERSION *AllAudioVersion);
void set_imeek_light_struct(ENUM_LIGHT_STATE ear_state,ENUM_LINGT_COLOR ear_color,ENUM_LIGHT_MODE ear_mode,ENUM_LIGHT_END ear_end,uint32_t ear_time,
	                        ENUM_LIGHT_STATE eyes_state,ENUM_LINGT_COLOR eyes_color,ENUM_LIGHT_MODE eyes_mode,ENUM_LIGHT_END eyes_end,uint32_t eyes_time);

#endif

