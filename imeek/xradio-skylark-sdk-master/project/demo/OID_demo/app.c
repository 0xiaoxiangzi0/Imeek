#include <stdlib.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "kernel/FreeRTOS/event_groups.h"
#include "kernel/os/os.h"
#include "net/wlan/wlan_ext_req.h"
#include "driver/chip/hal_gpio.h"
#include "driver/chip/hal_util.h"
#include "driver/chip/hal_wdg.h"
#include "driver/chip/hal_wakeup.h"
#include "pm/pm.h"

#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "lwip/tcpip.h"
#include "net/udhcp/usr_dhcpd.h"
#include "common/framework/net_ctrl.h"
//#include "net_ctrl.h"
//#include "net_ctrl_debug.h"

#include "imeek_key.h"
#include "app.h"
#include "ir_led.h"
#include "stepper_motor.h"
#include "rgb_led.h"
#include "oid_airkiss.h"
#include "oid_flash.h"
#include "oid_network_connect.h"
#include "oid_bp_vf.h"
#include "oid_transf.h"
#include "oid_protocol.h"
#include "oid_code_distribution.h"
#include "oid_audio.h"
#include "imeek_idle_mode.h"
#include "imeek_record.h"
#include "app.h"
#include "imeek_file.h"
#include "pid.h"

#define GET_RT_OID_DATA 0
#if (GET_RT_OID_DATA == 1)
uint32_t datatemp[3000];
int indextemp = 0;
#endif

extern STRUCT_SSID_PWD ssid_pwd;

/**************空闲模式涉及的变量*****************/
   //5分钟没有操作米小可默认进入空闲模式
uint8_t g_u8IdleTimeStart = TRUE;

/*************************************************/
STRUCT_IMEEK_WORK_STATE imeek_work_state = {FREE_MODE,1,0,0,0,0,0,0, 
                                            0,0,VARIABLE_INIT,0,0,
                                            MEEK_X_AXIS_LOW_TO_HIGH};
STRUCT_LIGHT imeek_light = {LIGHT_ON,LIGHT_BLUE,LIGHT_FORM,LIGHT_CONTINUE,4,
	                        LIGHT_ON,LIGHT_BLUE,LIGHT_FORM,LIGHT_CONTINUE,4};
//STRUCT_COMPOSE g_strImeekCompose = {COMPOSE_INIT,-1};
//STRUCT_IMEEK_RECORD  g_strImeekRecord = {RECORD_OVER};
STRUCT_IMEEK_TRAILING g_strImeekTrailing = {TRAILING_OVER};


//米小克当前朝向
ENUM_MEEK_DIRECTION_AXIS meek_direction = MEEK_X_AXIS_LOW_TO_HIGH;
         //识别到用户拼接的指令卡位置，开始指令卡算0
uint32_t user_instruction_card_index = 0;
         //存储识别到的用户拼接的指令卡
uint32_t user_instruction_card[200] = {0};

#define APP_TASK_CTRL_THREAD_STACK_SIZE		            (7 * 1024)
#define RUN_INSTRUCT_TASK_CTRL_THREAD_STACK_SIZE	    (3 * 1024)
#define TRAILING_TASK_CTRL_THREAD_STACK_SIZE	        (1 * 1024)
#define BAT_TASK_CTRL_THREAD_STACK_SIZE		            (1 * 1024)
#define LIGHT_TASK_CTRL_THREAD_STACK_SIZE		        (1 * 1024)
#define IDLE_TASK_CTRL_THREAD_STACK_SIZE		        (1 * 1024)

static OS_Thread_t app_handler;
static OS_Thread_t run_instruct_handler;
static OS_Thread_t trailing_mode_handler;
static OS_Thread_t get_battery_voltage_task_handler;
static OS_Thread_t light_handler;
static OS_Thread_t idle_task_handler;

QueueHandle_t run_instrutcion_Queue;		 

#define RUN_INSTRUCTION_SUCCESS                     (1 << 0)
#define RUN_INSTRUCTION_CAR_FORBIDDEN			    (1 << 1)
#define RUN_INSTRUCTION_OVER_MAP			        (1 << 2)
EventGroupHandle_t run_instruction_event_group;
                 
STRUCT_FLASH_STATE g_strFlashState = {1,   //软件版本
	                                  1,   //音频版本
	                                  1,     //任务关卡
                                      1,     //数学关卡
                                      4,     //音量等级
                                      3,     //眼睛灯状态
                                      0x00,  //徽章状态1
	                                  0x00,  //徽章状态2
	                                  0x00,  //任务进度1
	                                  0x00,  //任务进度2
	                                  0x00,  //任务进度3
	                                  0x00,  //任务进度4
	                                  0x00,  //任务进度5
	                                  0x00,  //任务进度6
	                                  0x00,  //任务进度7
	                                  0x00,  //任务进度8
                                      };

STRUCT_ALL_AUDIO_UPDATE_DATA g_aAudioUpdateData[AUDIO_TOTAL] = {{0,FLASH_PUBLIC_AUDIO_VERSION_ADDR,"general",FALSE},
                                                                {0,FLASH_PAINT1_AUDIO_VERSION_ADDR,"map1",FALSE},
                                                                {0,FLASH_PAINT2_AUDIO_VERSION_ADDR,"map2",FALSE},
                                                                {0,FLASH_PAINT3_AUDIO_VERSION_ADDR,"map3",FALSE},
                                                                {0,FLASH_PAINT4_AUDIO_VERSION_ADDR,"map4",FALSE},
                                                                {0,FLASH_PAINT5_AUDIO_VERSION_ADDR,"map5",FALSE},
                                                                {0,FLASH_PAINT6_AUDIO_VERSION_ADDR,"map6",FALSE},
                                                                {0,FLASH_PAINT7_AUDIO_VERSION_ADDR,"map7",FALSE},
                                                                {0,FLASH_PAINT8_AUDIO_VERSION_ADDR,"map8",FALSE},
                                                                {0,FLASH_PAINT9_AUDIO_VERSION_ADDR,"map9",FALSE},
                                                                {0,FLASH_PAINT10_AUDIO_VERSION_ADDR,"map10",FALSE},
                                                                {0,FLASH_MATH1_AUDIO_VERSION_ADDR,"math1",FALSE},
                                                                {0,FLASH_MATH2_AUDIO_VERSION_ADDR,"math2",FALSE},
                                                                {0,FLASH_MATH3_AUDIO_VERSION_ADDR,"math3",FALSE},
                                                                {0,FLASH_MATH4_AUDIO_VERSION_ADDR,"math4",FALSE},
                                                                {0,FLASH_MATH5_AUDIO_VERSION_ADDR,"math5",FALSE},
                                                                {0,FLASH_MATH6_AUDIO_VERSION_ADDR,"math6",FALSE},
                                                                {0,FLASH_MATH7_AUDIO_VERSION_ADDR,"math7",FALSE},
                                                                {0,FLASH_MATH8_AUDIO_VERSION_ADDR,"math8",FALSE},
                                                                {0,FLASH_MATH9_AUDIO_VERSION_ADDR,"math9",FALSE},
                                                                {0,FLASH_MATH10_AUDIO_VERSION_ADDR,"math10",FALSE},
                                                                {0,FLASH_BADGE_AUDIO_VERSION_ADDR,"badge",FALSE},
                                                                {0,FLASH_STICKER_AUDIO_VERSION_ADDR,"sticker",FALSE},
                                                                {0,FLASH_ENGLISH_AUDIO_VERSION_ADDR,"EnglishSpell",FALSE},
                                                                {0,FLASH_CHINESE_AUDIO_VERSION_ADDR,"Chinese",FALSE},};

STRUCT_MISSION_STATE g_strMissionState = {NO_QUESTION,
	                                      0,
	                                      0,
	                                      FALSE,
	                                     };
STRUCT_IMEEK_ID g_strImeekId = {FALSE,"FF"};

/*
*******************************************************************************
 *Function:     AppReboot
 *Description:  软件复位
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void AppReboot(void)
{
	HAL_WDG_Reboot();
}

/*
*******************************************************************************
 *Function:     GetFlashState
 *Description:  获取内存中保存的每台米小克的记录值
 *Calls:       
 *Called By:   
 *Input:        STRUCT_FLASH_STATE* g_strFlashState:接收结构体的首地址
 *Output:       
 *Return:       
 *Others:       待修改:如果在修改内存的时候，断电了，用户保存的内存都将消失，
 *                     因此要添加机制，即使内存数据消失依然可以正确获取到
*******************************************************************************
*/
void GetFlashState(STRUCT_FLASH_STATE* FlashState,STRUCT_IMEEK_ID* ImeekId,
                   STRUCT_SSID_PWD *SsidPwd,STRUCT_ALL_AUDIO_UPDATE_DATA *AllAudioVersion)
{
    STRUCT_FLASH_STATE l_strFlashState;
//	char buff[20];
	
	flash_state_read(&l_strFlashState);
	//内存如果没有设置过，则将默认值写入内存中
	if(l_strFlashState.MissionLevel == 0xFFFFFFFF){
		flash_state_write(FlashState);
	}
	else{
		//内存已设置过，则将内存数据读出来
		flash_state_read(FlashState);
	}

	//获取米小克ID
	imeek_id_read(ImeekId);
	
	//获取SSID与PWD
	flash_ssid_pwd_read(SsidPwd);

	//获取英语音频版本信息
	flash_int_read(AllAudioVersion[ENGLISH_AUDIO].Addr,&AllAudioVersion[ENGLISH_AUDIO].AudioVersion);
	if(0xFFFFFFFF == AllAudioVersion[ENGLISH_AUDIO].AudioVersion){
		//内存还没有写过对应音频版本信息,则音频版本为0最低音频版本
		AllAudioVersion[ENGLISH_AUDIO].AudioVersion = 0;
	}

	//获取汉字音频版本信息
	flash_int_read(AllAudioVersion[CHINESE_AUDIO].Addr,&AllAudioVersion[CHINESE_AUDIO].AudioVersion);
	if(0xFFFFFFFF == AllAudioVersion[CHINESE_AUDIO].AudioVersion){
		//内存还没有写过对应音频版本信息,则音频版本为0最低音频版本
		AllAudioVersion[CHINESE_AUDIO].AudioVersion = 0;
	}
	
	printf("ImeekId.SendFlag = %d\n",ImeekId->SendFlag);
	printf("ImeekId->Id = %s\n",ImeekId->Id);
	printf("l_strFlashState.LocalCodeVersion = %d\n",FlashState->LocalCodeVersion);
	printf("l_strFlashState.LocalAudioVersion = %d\n",FlashState->LocalAudioVersion);
	printf("l_strFlashState.MissionLevel = %d\n",FlashState->MissionLevel);
	printf("l_strFlashState.VolumeLevel = %d\n",FlashState->VolumeLevel);
	printf("l_strFlashState.MathLevel = %d\n",FlashState->MathLevel);
	printf("l_strFlashState.EyesState = %d\n",FlashState->EyesState);
	printf("l_strFlashState.TaskProgess1 = %d\n",FlashState->TaskProgess1);
	printf("l_strFlashState.TaskProgess2 = %d\n",FlashState->TaskProgess2);
	printf("l_strFlashState.TaskProgess3 = %d\n",FlashState->TaskProgess3);
	printf("l_strFlashState.TaskProgess4 = %d\n",FlashState->TaskProgess4);
	printf("l_strFlashState.BadgeState1 = %d\n",FlashState->BadgeState1);
	printf("l_strFlashState.BadgeState2 = %d\n",FlashState->BadgeState2);
	printf("l_strFlashState.TaskProgess5 = %d\n",FlashState->TaskProgess5);
	printf("l_strFlashState.TaskProgess6 = %d\n",FlashState->TaskProgess6);
	printf("l_strFlashState.TaskProgess7 = %d\n",FlashState->TaskProgess7);
	printf("l_strFlashState.TaskProgess8 = %d\n",FlashState->TaskProgess8);
	printf("AllAudioVersion.EnglishAudioVersion = %d\n",AllAudioVersion[ENGLISH_AUDIO].AudioVersion);
	printf("AllAudioVersion.ChineseAudioVersion = %d\n",AllAudioVersion[CHINESE_AUDIO].AudioVersion);
}

/*
*******************************************************************************
 *Function:     StartUpLight
 *Description:  开机灯光展示
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void StartUpLight(uint32_t eyes_state)
{
    switch(eyes_state){
        case 1:
			imeek_light.light_eyes_color = LIGHT_RED;
			LED_R_ON();
			OS_MSleep(600);
			LED_R_OFF();
			OS_MSleep(100);
			LED_R_ON();
			OS_MSleep(600);
			LED_R_OFF();
			OS_MSleep(1100);
			LED_R_ON();
			break;
		case 2:
			imeek_light.light_eyes_color = LIGHT_GREEN;
			LED_G_ON();
			OS_MSleep(600);
			LED_G_OFF();
			OS_MSleep(100);
			LED_G_ON();
			OS_MSleep(600);
			LED_G_OFF();
			OS_MSleep(1100);
			LED_G_ON();
			break;
		case 3:
			imeek_light.light_eyes_color = LIGHT_BLUE;
			LED_B_ON();
			OS_MSleep(600);
			LED_B_OFF();
			OS_MSleep(100);
			LED_B_ON();
			OS_MSleep(600);
			LED_B_OFF();
			OS_MSleep(1100);
			LED_B_ON();
			break;
		default:
			break;
	}
	LED_EAR_ON();
}


void ImeekIdleAction(uint8_t num)
{
    uint32_t l_u32LeftMotorDirection  = OS_Rand32() % 2;
    uint32_t l_u32LeftMotorSpeed      = OS_Rand32() % 10;	
    uint32_t l_u32RightMotorDirection = OS_Rand32() % 3;
    uint32_t l_u32RightMotorSpeed     = OS_Rand32() % 10;

    if(num-- > 0){
		ImeekIdleAction(num);
		adj_motor_state(l_u32LeftMotorSpeed,l_u32LeftMotorDirection,
			            l_u32RightMotorSpeed,l_u32RightMotorDirection);
		OS_Sleep(2);
//		printf("ImeekIdleAction time = %d\n",num);
	}
	adj_motor_state(0,l_u32LeftMotorDirection,
			        0,l_u32RightMotorDirection);
}

//正斜率直线，求y值
uint32_t get_l1_y(STRUCT_APP_OID_DATA *l_OidData, STRUCT_BLOCK *block, uint32_t map)
{
    float k;
	float y2_y1;
	float x2_x1;
    uint32_t y;
	
	//斜率
	y2_y1 = (float)map_block_data[map][block->row][block->column].low_y_axis  - map_block_data[map][block->row][block->column].high_y_axis;
	x2_x1 = (float)map_block_data[map][block->row][block->column].high_x_axis  - map_block_data[map][block->row][block->column].low_x_axis;
    k = (float)y2_y1 / (float)x2_x1;

	//已知x求y值
	y = (uint32_t)(l_OidData->position.x - map_block_data[map][block->row][block->column].low_x_axis) * k 
	              + map_block_data[map][block->row][block->column].high_y_axis;

//	printf("map_block_data[map][block->row][block->column].low_x_axis = %d\n",map_block_data[map][block->row][block->column].low_x_axis);
//	printf("map_block_data[map][block->row][block->column].high_x_axis = %d\n",map_block_data[map][block->row][block->column].high_x_axis);
//	printf("map_block_data[map][block->row][block->column].low_y_axis = %d\n",map_block_data[map][block->row][block->column].low_y_axis);
//	printf("map_block_data[map][block->row][block->column].high_y_axis = %d\n",map_block_data[map][block->row][block->column].high_y_axis);
//	printf("k1 = %f\n",k);
//	printf("y2_y1 = %f\n",y2_y1);
//	printf("x2_x1 = %f\n",x2_x1);
//	printf("l_OidData->position.x = %d\n",l_OidData->position.x);
//	printf("l_OidData->position.y = %d\n",l_OidData->position.y);
	return y;
}

//负斜率直线，求y值
uint32_t get_l2_y(STRUCT_APP_OID_DATA *l_OidData, STRUCT_BLOCK *block, uint32_t map)
{
    float k;
	float y2_y1;
	float x2_x1;
    uint32_t y;
	
	//斜率
	y2_y1 = (float)map_block_data[map][block->row][block->column].high_y_axis  - map_block_data[map][block->row][block->column].low_y_axis;
	x2_x1 = (float)map_block_data[map][block->row][block->column].high_x_axis  - map_block_data[map][block->row][block->column].low_x_axis;
    k = (float)y2_y1 / (float)x2_x1;
	
	//已知x求y值
	y = (uint32_t)(l_OidData->position.x - map_block_data[map][block->row][block->column].low_x_axis) * k 
	              + map_block_data[map][block->row][block->column].low_y_axis;

//	printf("k2 = %f\n",k);
	return y;
}

void set_imeek_light_struct(ENUM_LIGHT_STATE ear_state,ENUM_LINGT_COLOR ear_color,ENUM_LIGHT_MODE ear_mode,ENUM_LIGHT_END ear_end,uint32_t ear_time,
	                        ENUM_LIGHT_STATE eyes_state,ENUM_LINGT_COLOR eyes_color,ENUM_LIGHT_MODE eyes_mode,ENUM_LIGHT_END eyes_end,uint32_t eyes_time)
{
	imeek_light.light_ear_state = ear_state;
    imeek_light.light_ear_color = ear_color;
	imeek_light.light_ear_mode  = ear_mode;
	imeek_light.light_ear_end   = ear_end;
	imeek_light.light_ear_time_of_duration = ear_time * 2;

	imeek_light.light_eyes_state = eyes_state;
	imeek_light.light_eyes_color = eyes_color;
	imeek_light.light_eyes_mode  = eyes_mode;
	imeek_light.light_eyes_end   = eyes_end;
	imeek_light.light_eyes_time_of_duration = eyes_time * 2;
}

/*
*******************************************************************************
 *Function:     CopyOidData
 *Description:  复制点读笔数据
 *              
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void CopyOidData(OID_DATA_STRUCT *des_oiddata,OID_DATA_STRUCT *src_oiddata)
{
	des_oiddata->flag       = src_oiddata->flag;
	des_oiddata->cmd        = src_oiddata->cmd;
	des_oiddata->code_type  = src_oiddata->code_type;
	des_oiddata->data_type  = src_oiddata->data_type;
	des_oiddata->index      = src_oiddata->index;
	des_oiddata->oid_mode   = src_oiddata->oid_mode;
	des_oiddata->position   = src_oiddata->position;
	des_oiddata->valid_code = src_oiddata->valid_code;
}

/*
*******************************************************************************
 *Function:     isSameOidData
 *Description:  做误码判断，并将最终结果返回
 *              
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
 *******************************************************************************
*/
MYBOOL OidDataDebounce(OID_DATA_STRUCT* l_oid_data)
{
	static OID_DATA_STRUCT l_aOidData[3] = {{FALSE},{FALSE},{FALSE}};
	static uint8_t index = 0;

	index = (index+1) % 3;
	CopyOidData(&l_aOidData[index],l_oid_data);
//	printf("---------------------OidDataDebounce----------------------\n");
//	printf("oid_data.flag = %d\n",oid_data.flag);
//	printf("oid_data.index = %d\n",oid_data.index);
//	printf("oid_data.position.x = %d\n",oid_data.position.x);
//	printf("oid_data.position.y = %d\n",oid_data.position.y);
//	printf("index = %d\n",index);
//	printf("l_aOidData[0].flag = %d\n",l_aOidData[0].flag);
//	printf("l_aOidData[0].index = %d\n",l_aOidData[0].index);
//	printf("l_aOidData[0].position.x = %d\n",l_aOidData[0].position.x);
//	printf("l_aOidData[0].position.y = %d\n",l_aOidData[0].position.y);
//	printf("l_aOidData[1].flag = %d\n",l_aOidData[1].flag);
//	printf("l_aOidData[1].index = %d\n",l_aOidData[1].index);
//	printf("l_aOidData[1].position.x = %d\n",l_aOidData[1].position.x);
//	printf("l_aOidData[1].position.y = %d\n",l_aOidData[1].position.y);
//	printf("l_aOidData[2].flag = %d\n",l_aOidData[2].flag);
//	printf("l_aOidData[2].index = %d\n",l_aOidData[2].index);
//	printf("l_aOidData[2].position.x = %d\n",l_aOidData[2].position.x);
//	printf("l_aOidData[2].position.y = %d\n",l_aOidData[2].position.y);
	
	//连续三次点读笔数据都有效
	if((l_aOidData[0].flag == l_aOidData[1].flag)
	&& (l_aOidData[1].flag == l_aOidData[2].flag)
	&& (l_aOidData[2].flag == TRUE)){
		//三次数据都是index，而不是命令
		if((l_aOidData[0].data_type == l_aOidData[1].data_type)
			&&(l_aOidData[1].data_type == l_aOidData[2].data_type)
			&&(l_aOidData[2].data_type == 0)){
			//三次都是圈图码
			if((l_aOidData[0].code_type == l_aOidData[1].code_type)
				&&(l_aOidData[1].code_type == l_aOidData[2].code_type)
				&&(l_aOidData[2].code_type == 0)){
				if((l_aOidData[0].index == l_aOidData[1].index)
					&&(l_aOidData[1].index == l_aOidData[2].index)){
					//三次都是圈图码，切码值相同
					return TRUE;
				}
			}
			//三次都是位置码
			else if((l_aOidData[0].code_type == l_aOidData[1].code_type)
					&&(l_aOidData[1].code_type == l_aOidData[2].code_type)
					&&(l_aOidData[2].code_type == 1)){
					if((l_aOidData[0].position.x == l_aOidData[1].position.x)
						&&(l_aOidData[1].position.x == l_aOidData[2].position.x)
						&&(l_aOidData[0].position.y == l_aOidData[1].position.y)
						&&(l_aOidData[1].position.y == l_aOidData[2].position.y)){
						//三次都是位置码，且三次位置码相同
						return TRUE;
					}
			}
		}
	}
	else if((l_aOidData[0].flag == l_aOidData[1].flag)
		&&  (l_aOidData[1].flag == l_aOidData[2].flag)
		&&  (l_aOidData[2].flag == FALSE)){
		//连续三次点读笔数据都无效
		return TRUE;
	}

	return FALSE;
}

/*
*******************************************************************************
 *Function:     GetRealTimeOidData
 *Description:  更新实时oid数据
 *              
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void GetRealTimeOidData(OID_DATA_STRUCT* l_OidData,
                        STRUCT_APP_OID_DATA *l_realtime_data)
{
	//读取到oid数据，使用的是3代笔头，数据有效
	if((OID_DATA_VALID == l_OidData->flag) && (0 == l_OidData->data_type)
	 &&(1 == l_OidData->oid_mode) && (0 == l_OidData->valid_code)){
		if(l_OidData->code_type){
			l_realtime_data->code_flag = POSITION_CODE;
			l_realtime_data->position.x = l_OidData->position.x;
			l_realtime_data->position.y = l_OidData->position.y;
		}
		else{
			l_realtime_data->code_flag = GENERAL_CODE;
			l_realtime_data->index = l_OidData->index;
		}
	}
	else{
	    l_realtime_data->code_flag = NO_VALID_CODE;
	}
}

/*
*******************************************************************************
 *Function:     GetDebounceOidData
 *Description:  更新debouce后的数据
 *              
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void GetDebounceOidData(OID_DATA_STRUCT* l_OidData,
                        STRUCT_APP_OID_DATA *l_debounce_data)
{
	//更新debounce后的数据
	if(OidDataDebounce(l_OidData)){
		GetRealTimeOidData(l_OidData,l_debounce_data);
	}
	else{
		l_debounce_data->code_flag = NO_VALID_CODE;
	}
}

/*
*******************************************************************************
 *Function:     GetAppOidData
 *Description:  更新实时oid数据，与经过debouce后的数据
 *              
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void GetAppOidData(OID_DATA_STRUCT* l_OidData,
                   STRUCT_APP_OID_DATA *l_realtime_data,
                   STRUCT_APP_OID_DATA *l_debounce_data)
{
	//更新实时数据
	GetRealTimeOidData(l_OidData,l_realtime_data);
//	printf("l_realtime_data.code_flag = %d\n",l_realtime_data->code_flag);
//	printf("l_realtime_data.index = %d\n",l_realtime_data->index);	
//	printf("l_realtime_data.position.x = %d\n",l_realtime_data->position.x);
//	printf("l_realtime_data.position.y = %d\n",l_realtime_data->position.y);
	
	//更新debounce后的数据
	GetDebounceOidData(l_OidData,l_debounce_data);
//	printf("l_debounce_data.code_flag = %d\n",l_debounce_data->code_flag);
//	printf("l_debounce_data.position.x = %d\n",l_debounce_data->position.x);
//	printf("l_debounce_data.position.y = %d\n",l_debounce_data->position.y);
}

/*
*******************************************************************************
 *Function:     isTaskMode
 *Description:  判断当前状态是否为任务模式
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:  is Task Mode
 *              FALSE: not Task Mode
 *Others:       该函数默认OID笔头已经获取到了坐标数据
*******************************************************************************
*/
MYBOOL isTaskMode(STRUCT_APP_OID_DATA* l_OidData)
{
#define TASK_MODE_LOW_X  405
#define TASK_MODE_HIGH_X 2642
#define TASK_MODE_LOW_Y  10000
#define TASK_MODE_HIGH_Y 10189
    if((l_OidData->position.x > TASK_MODE_LOW_X) && (l_OidData->position.x < TASK_MODE_HIGH_X)
	&&(l_OidData->position.y > TASK_MODE_LOW_Y) && (l_OidData->position.y < TASK_MODE_HIGH_Y)){
//	    printf("Task Mode\n");
        return TRUE;
	}

	return FALSE;
}

/*
*******************************************************************************
 *Function:     isFreeMode
 *Description:  判断当前状态是否为自由模式
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:  is Free Mode
 *              FALSE: not Free Mode
 *Others:       该函数默认OID笔头已经获取到了坐标数据
*******************************************************************************
*/
MYBOOL isFreeMode(STRUCT_APP_OID_DATA* l_OidData)
{
#define FREE_MODE_LOW_X  270
#define FREE_MODE_HIGH_X 403
#define FREE_MODE_LOW_Y  10000
#define FREE_MODE_HIGH_Y 10185
    if((l_OidData->position.x > FREE_MODE_LOW_X) && (l_OidData->position.x < FREE_MODE_HIGH_X)
	&&(l_OidData->position.y > FREE_MODE_LOW_Y) && (l_OidData->position.y < FREE_MODE_HIGH_Y)){
//	    printf("Free Mode\n");
        return TRUE;
	}

	return FALSE;
}

/*
*******************************************************************************
 *Function:     isMusicEditMode
 *Description:  判断当前状态是否为音乐编辑模式
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:  is Music Edit Mode
 *              FALSE: not Music Edit Mode
 *Others:       该函数默认OID笔头已经获取到了坐标数据
*******************************************************************************
*/
MYBOOL isMusicEditMode(STRUCT_APP_OID_DATA* l_OidData)
{
#define MUSIC_EDIT_MODE_LOW_X  0
#define MUSIC_EDIT_MODE_HIGH_X 133
#define MUSIC_EDIT_MODE_LOW_Y  10000
#define MUSIC_EDIT_MODE_HIGH_Y 10185
    if((l_OidData->position.x > MUSIC_EDIT_MODE_LOW_X) && (l_OidData->position.x < MUSIC_EDIT_MODE_HIGH_X)
	&&(l_OidData->position.y > MUSIC_EDIT_MODE_LOW_Y) && (l_OidData->position.y < MUSIC_EDIT_MODE_HIGH_Y)){
//	    printf("Music Mode\n");
        return TRUE;
	}

	return FALSE;
}

/*
*******************************************************************************
 *Function:     isRecordMode
 *Description:  判断当前状态是否为录音模式
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:  is Record Mode
 *              FALSE: not Record Mode
 *Others:       该函数默认OID笔头已经获取到了坐标数据
*******************************************************************************
*/
MYBOOL isRecordMode(STRUCT_APP_OID_DATA* l_OidData)
{
#define RECORD_MODE_LOW_X  135
#define RECORD_MODE_HIGH_X 268
#define RECORD_MODE_LOW_Y  10077
#define RECORD_MODE_HIGH_Y 10124
    if((l_OidData->position.x > RECORD_MODE_LOW_X) && (l_OidData->position.x < RECORD_MODE_HIGH_X)
	&& (l_OidData->position.y > RECORD_MODE_LOW_Y) && (l_OidData->position.y < RECORD_MODE_HIGH_Y)){
//	    printf("Record Mode\n");
        return TRUE;
	}

	return FALSE;
}

/*
*******************************************************************************
 *Function:     isQuestionMode
 *Description:  判断当前状态是否为问答模式
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:  is Question Mode
 *              FALSE: not Question Mode
 *Others:       该函数默认OID笔头已经获取到了坐标数据
*******************************************************************************
*/
MYBOOL isQuestionMode(STRUCT_APP_OID_DATA* l_OidData)
{
#define QUESTION_MODE_LOW_X1  2645
#define QUESTION_MODE_HIGH_X1 3332
#define QUESTION_MODE_LOW_Y1  10000
#define QUESTION_MODE_HIGH_Y1 10190

#define QUESTION_MODE_LOW_X2  550
#define QUESTION_MODE_HIGH_X2 1079
#define QUESTION_MODE_LOW_Y2  0
#define QUESTION_MODE_HIGH_Y2 392

    if((l_OidData->position.x > QUESTION_MODE_LOW_X1) && (l_OidData->position.x < QUESTION_MODE_HIGH_X1)
	&& (l_OidData->position.y > QUESTION_MODE_LOW_Y1) && (l_OidData->position.y < QUESTION_MODE_HIGH_Y1)){
//	    printf("Question Mode\n");
        return TRUE;
	}

    if((l_OidData->position.x > QUESTION_MODE_LOW_X2) && (l_OidData->position.x < QUESTION_MODE_HIGH_X2)
	&& (l_OidData->position.y > QUESTION_MODE_LOW_Y2) && (l_OidData->position.y < QUESTION_MODE_HIGH_Y2)){
//	    printf("Question Mode\n");
	    return TRUE;
	}
	
	return FALSE;
}

/*
*******************************************************************************
 *Function:     WriteUserData
 *Description:  如果同时长按了多功能按键与wifi复位按键，则清除用户数据
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void WriteUserData(ENUM_KEY_TRIGGER_IMEEK_STATE key_trigger_state)
{
	if(KEY_TRIGGER_CLEAR_USER == key_trigger_state){
		WaitAudioPlayOver(TRUE);
		g_strFlashState.MissionLevel = 1;
		g_strFlashState.MathLevel = 1;
		g_strFlashState.VolumeLevel = 4;
		g_strFlashState.EyesState = 1;
		g_strFlashState.BadgeState1 = 0;
		g_strFlashState.BadgeState2 = 0;
		g_strFlashState.TaskProgess1 = 0;
		g_strFlashState.TaskProgess2 = 0;
		g_strFlashState.TaskProgess3 = 0;
		g_strFlashState.TaskProgess4 = 0;
		g_strFlashState.TaskProgess5 = 0;
		g_strFlashState.TaskProgess6 = 0;
		g_strFlashState.TaskProgess7 = 0;
		g_strFlashState.TaskProgess8 = 0;
		taskENTER_CRITICAL();
		flash_state_write(&g_strFlashState);
		taskEXIT_CRITICAL();
		AudioPlayToEnd("file://music/Public/_51.mp3",ZERO_LEVEL_AUDIO,IMEEK_PLAY);
		AppReboot();
	}
}

/*
*******************************************************************************
 *Function:     ClearWifiData
 *Description:  如果只长按了wifi复位按键，则清除wifi数据
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void ClearWifiData(ENUM_KEY_TRIGGER_IMEEK_STATE key_trigger_state)
{
	if(KEY_TRIGGER_CLEAR_WIFI == key_trigger_state){
		WaitAudioPlayOver(TRUE);
		taskENTER_CRITICAL();
		ssid_pwd.flag = 1;
		flash_ssid_pwd_write(&ssid_pwd);					
		taskEXIT_CRITICAL();
		AudioPlayToEnd("file://music/Public/_6.mp3",SECOND_LEVEL_AUDIO,IMEEK_PLAY);
		AppReboot();
	}
}

/*
*******************************************************************************
 *Function:     JdgIntoIdleMode
 *Description:  判断空闲模式是否开始计时
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void JdgIntoIdleMode(STRUCT_APP_OID_DATA* l_OidData)
{   
    char *file = "file://music/Paint/001/P14-105.mp3";
      //如果没有点读，且当前不处于播放状态，空闲模式开始计时
	if((NO_VALID_CODE == l_OidData->code_flag)
	&& (TRAILING_MODE != imeek_work_state.state)){
	    g_u8IdleTimeStart = TRUE;
    }
	else{
	    g_u8IdleTimeStart = FALSE;
	}

	if((IMEEK_PLAY == OidPlayAudio.CurrentState)
	&& (!strcmp(OidPlayAudio.play_audio_url_old,file))){
	    g_u8IdleTimeStart = FALSE;
	}
//	printf("g_u8IdleTimeStart = %d\n",g_u8IdleTimeStart);
}

/*
*******************************************************************************
 *Function:     isPaintOid
 *Description:  判断是否点读笔是否是指向了绘本，以及指向了哪个绘本
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
MYBOOL isPaintOid(STRUCT_APP_OID_DATA* l_OidData, int *num)
{
	MYBOOL ret = FALSE;
    if((l_OidData->position.x > PAINT_1_LOW_X) && (l_OidData->position.x < PAINT_1_HIGH_X)
	&& (l_OidData->position.y > PAINT_1_LOW_Y) && (l_OidData->position.y < PAINT_1_HIGH_Y)){
		//绘本1，对应地图1
		*num = 1;
		ret = TRUE;
	}

	return ret;
}

/*
*******************************************************************************
 *Function:     isMathOid
 *Description:  判断是否点读笔指向了数学问题，以及指向了哪张地图的数学问题
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
MYBOOL isMathOid(STRUCT_APP_OID_DATA* l_OidData, int *num)
{
	MYBOOL ret = FALSE;
	if((l_OidData->position.x > MATH_1_LOW_X) && (l_OidData->position.x < MATH_1_HIGH_X)
	&& (l_OidData->position.y > MATH_1_LOW_Y) && (l_OidData->position.y < MATH_1_HIGH_Y)){
		*num = 1;
		ret = TRUE;
	}

	return ret;
}

/*
*******************************************************************************
 *Function:     isBadgeOid
 *Description:  判断是否点读笔指向了徽章榜
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
MYBOOL isBadgeOid(STRUCT_APP_OID_DATA* l_OidData)
{
	MYBOOL ret = FALSE;
    if((l_OidData->position.x > BADGE_1_LOW_X) && (l_OidData->position.x < BADGE_1_HIGH_X)
	&& (l_OidData->position.y > BADGE_1_LOW_Y) && (l_OidData->position.y < BADGE_1_HIGH_Y)){
		ret = TRUE;
	}

	return ret;
}

/*
*******************************************************************************
 *Function:     isStickerOid
 *Description:  判断是否点读笔指向了贴纸
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
MYBOOL isStickerOid(STRUCT_APP_OID_DATA* l_OidData)
{
	MYBOOL ret = FALSE;
	if((l_OidData->position.x > STICKER_1_LOW_X) && (l_OidData->position.x < STICKER_1_HIGH_X)
	&& (l_OidData->position.y > STICKER_1_LOW_Y) && (l_OidData->position.y < STICKER_1_HIGH_Y)){
		ret = TRUE;
	}
	
	return ret;
}

/*
*******************************************************************************
 *Function:     isEnglishLetterOID
 *Description:  判断是否点读笔指向了英语字母卡片
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
MYBOOL isEnglishLetterOID(STRUCT_APP_OID_DATA* l_OidData)
{
	MYBOOL ret = FALSE;
	if((l_OidData->position.x > ENGLISH_LETTER_LOW_X) && (l_OidData->position.x < ENGLISH_LETTER_HIGH_X)
	&& (l_OidData->position.y > ENGLISH_LETTER_LOW_Y) && (l_OidData->position.y < ENGLISH_LETTER_HIGH_Y)){
		ret = TRUE;
	}
	
	return ret;
}

/*
*******************************************************************************
 *Function:     isEnglishWordOID
 *Description:  判断是否点读笔指向了英语单词卡片
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
MYBOOL isEnglishWordOID(STRUCT_APP_OID_DATA* l_OidData)
{
	MYBOOL ret = FALSE;
	if((l_OidData->position.x > ENGLISH_WORD_LOW_X) && (l_OidData->position.x < ENGLISH_WORD_HIGH_X)
	&& (l_OidData->position.y > ENGLISH_WORD_LOW_Y) && (l_OidData->position.y < ENGLISH_WORD_HIGH_Y)){
		ret = TRUE;
	}
	
	return ret;
}

/*
*******************************************************************************
 *Function:     isChineseWordOID
 *Description:  判断是否点读笔指向了汉字卡片
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
MYBOOL isChineseWordOID(STRUCT_APP_OID_DATA* l_OidData)
{
	MYBOOL ret = FALSE;
	if((l_OidData->position.x > CHINESE_WORD_LOW_X) && (l_OidData->position.x < CHINESE_WORD_HIGH_X)
	&& (l_OidData->position.y > CHINESE_WORD_LOW_Y) && (l_OidData->position.y < CHINESE_WORD_HIGH_Y)){
		ret = TRUE;
	}
	
	return ret;
}

/*
*******************************************************************************
 *Function:     isDownLoadAudioOID
 *Description:  判断是否点读笔指向了音频下载卡片
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
MYBOOL isDownLoadAudioOID(STRUCT_APP_OID_DATA* l_OidData)
{
	MYBOOL ret = FALSE;
	if((l_OidData->position.x > CHINESE_AUDIO_DOWNLOAD_LOW_X) && (l_OidData->position.x < ENGLISH_WORD_DOWNLOAD_HIGH_X)
	&& (l_OidData->position.y > CHINESE_AUDIO_DOWNLOAD_LOW_Y) && (l_OidData->position.y < ENGLISH_WORD_DOWNLOAD_HIGH_Y)){
		ret = TRUE;
	}
	
	return ret;
}


/*
*******************************************************************************
 *Function:     KeyTriggerImeekState
 *Description:  检测按键是否触发了什么功能
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       ENUM_KEY_TRIGGER_IMEEK_STATE：按键触发的具体功能
 *Others:       
*******************************************************************************
*/
ENUM_KEY_TRIGGER_IMEEK_STATE KeyTriggerImeekState(void)
{
	ENUM_KEY_TRIGGER_IMEEK_STATE ret = KEY_TRIGGER_NONE;
	static uint32_t l_u32MulKeyLongPressTime   = 0;//等于0说明还没有开始计时
	static uint32_t l_u32WifiKeyLongPressTime  = 0;
	static uint32_t l_u32MulKeyShortPressTime  = 0;
	static uint32_t l_u32WifiKeyshortPressTime = 0;

	//获取当前按了哪些按键，并开始分别计时
	if (g_strMultifunctionKey.LongPressFlag 
	&& (l_u32MulKeyLongPressTime == 0)){
		l_u32MulKeyLongPressTime = OS_TicksToMSecs(OS_GetTicks());
	}

	if (g_strWifiResetKey.LongPressFlag 
	&& (l_u32WifiKeyLongPressTime == 0)){
		l_u32WifiKeyLongPressTime = OS_TicksToMSecs(OS_GetTicks());
	}
	
	if (g_strMultifunctionKey.ShortPressFlag 
	&& (l_u32MulKeyShortPressTime == 0)){
		l_u32MulKeyShortPressTime = OS_TicksToMSecs(OS_GetTicks());
	}

	if (g_strWifiResetKey.ShortPressFlag 
	&& (l_u32WifiKeyshortPressTime == 0)){
		l_u32WifiKeyshortPressTime = OS_TicksToMSecs(OS_GetTicks());
	}
//	printf("l_u32MulKeyLongPressTime = %d\n",l_u32MulKeyLongPressTime);
//	printf("l_u32WifiKeyLongPressTime = %d\n",l_u32WifiKeyLongPressTime);
//	printf("l_u32MulKeyShortPressTime = %d\n",l_u32MulKeyShortPressTime);
//	printf("l_u32WifiKeyshortPressTime = %d\n",l_u32WifiKeyshortPressTime);

	//根据按键情况，决定触发了什么状态
	if(0 != l_u32MulKeyLongPressTime){
		if(0 != l_u32WifiKeyLongPressTime){
			//多功能按键与WiFi都按下了
			ret = KEY_TRIGGER_CLEAR_USER;
			l_u32MulKeyLongPressTime = 0;
			l_u32WifiKeyLongPressTime = 0;
			g_strWifiResetKey.LongPressFlag = 0;
			g_strMultifunctionKey.LongPressFlag = 0;
		}
		else if((OS_TicksToMSecs(OS_GetTicks()) - l_u32MulKeyLongPressTime) >= 500){
			//多功能按键已经长按500ms，wifi按键还未长按
			ret = KEY_TRIGGER_TRACKING;
			l_u32MulKeyLongPressTime = 0;
			g_strMultifunctionKey.LongPressFlag = 0;
		}
	}
	else if((0 != l_u32WifiKeyLongPressTime)
	      &&((OS_TicksToMSecs(OS_GetTicks()) - l_u32WifiKeyLongPressTime) >= 500)){
			//wifi按键已经长按500ms，多功能按键还未长按
			ret = KEY_TRIGGER_CLEAR_WIFI;
			l_u32WifiKeyLongPressTime = 0;
			g_strWifiResetKey.LongPressFlag = 0;
	}
	
	if(0 != l_u32MulKeyShortPressTime){
		if(0 != l_u32WifiKeyshortPressTime){
			//多功能按键与WiFi都短按了
			ret = KEY_TRIGGER_TEST_MODE;
			l_u32MulKeyShortPressTime = 0;
			l_u32WifiKeyshortPressTime = 0;
			g_strMultifunctionKey.ShortPressFlag = 0;
			g_strWifiResetKey.ShortPressFlag = 0;
		}
		else if((OS_TicksToMSecs(OS_GetTicks()) - l_u32MulKeyShortPressTime) >= 300){
			//多功能按键已经短按300ms，wifi按键还未短按
			l_u32MulKeyShortPressTime = 0;
			g_strMultifunctionKey.ShortPressFlag = 0;
		}
	}
	else if((0 != l_u32WifiKeyshortPressTime)
	      &&((OS_TicksToMSecs(OS_GetTicks()) - l_u32WifiKeyshortPressTime) >= 300)){
			//wifi按键已经短按300ms，多功能按键还未短按
			l_u32WifiKeyshortPressTime = 0;
			g_strWifiResetKey.ShortPressFlag = 0;
	}

	return ret;
}


/*
*******************************************************************************
 *Function:     InitMissionState
 *Description:  根据米小克当前所处的状态，决定是否初始化某些标志位
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void InitMissionState(ENUM_KEY_TRIGGER_IMEEK_STATE key_trigger_state,
                      ENUM_RUNNING_STATE *state)
{
	static ENUM_RUNNING_STATE last_state = FREE_MODE;

	if(KEY_TRIGGER_TRACKING == key_trigger_state){
		if((*state == TRAILING_MODE) && (TRAILING == g_strImeekTrailing.state)){
			g_strImeekTrailing.state = TRAILING_PLAY_OVER;
		}
		else{
			*state = TRAILING_MODE;
			g_strImeekTrailing.state = TRAILING_INIT;
		}
	}
	else if(KEY_TRIGGER_TEST_MODE == key_trigger_state){
		*state = TEST_MODE;
		IR_ENABLE();
	}
	else if(*state != last_state){
		switch(*state){
			case TASK_MODE:
			case FREE_MODE:
			case MUSIC_EDIT_MODE:
			case QUESTION_MODE:
			case TRAILING_MODE:
			case RECORD_MODE:
			case SPELL_MODE:
				imeek_work_state.current_mode_state = VARIABLE_INIT;
//				g_strImeekCompose.state = COMPOSE_INIT;
				break;
			default:
				break;
		}
	}
	last_state = *state;
}

/*
*******************************************************************************
 *Function:     PlayPaintAudio
 *Description:  根据点读笔指示位置，播放相应位置音频
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:点读的是绘本
 *              FALSE:点读的不是绘本
 *Others:       
*******************************************************************************
*/      
MYBOOL PlayPaintAudio(STRUCT_APP_OID_DATA* l_OidData, char* url,
                      ENUM_PLAY_TYPE* play_state,    MYBOOL* write_flash,
                      MYBOOL *wait_play)
{
    int i = 0xFFFFFF;
	static uint32_t l_u32QuestionFlag = 0x00;
	static uint32_t l_u32QuestionFlag2 = 0x00;

	*write_flash = FALSE;
    if((l_OidData->position.x > PAINT_1_LOW_X) && (l_OidData->position.x < PAINT_1_HIGH_X)
	&& (l_OidData->position.y > PAINT_1_LOW_Y) && (l_OidData->position.y < PAINT_1_HIGH_Y)){
	    for(i=0;i<PAINT_OID_TO_AUDIO_NUM;i++){
			if((l_OidData->position.x > PaintOidToAudio[i].low_x_axis)
		    && (l_OidData->position.x < PaintOidToAudio[i].high_x_axis)
			&& (l_OidData->position.y > PaintOidToAudio[i].low_y_axis)
			&& (l_OidData->position.y < PaintOidToAudio[i].high_y_axis)){
			    switch(PaintOidToAudio[i].flag){
                    case 0:
						sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
						break;
					case 1:
						//第一个徽章
						if(0 == i){
							//第一个徽章获得
                            if(g_strFlashState.BadgeState1 & (1<<i)){
                                sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[1]);
							}
							//第一个徽章未获得
							else{
								g_strFlashState.BadgeState1 |= 1<<i;
								sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
                                *write_flash = TRUE;
								*wait_play = TRUE;
							}
						}
						else if(g_strFlashState.BadgeState1 & (1<<i)){
							sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[1]);
						}
						else{
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
						}
						break;
					case 6:
						//音量减小
						if('1' == *(PaintOidToAudio[i].audio_path[0]+6)){
                            if(g_strFlashState.VolumeLevel == 1){
								sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[1]);
							}
							else{
                                sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
								g_strFlashState.VolumeLevel--;
								*write_flash = TRUE;
							}
						}
						//音量增大
						else if('3' == *(PaintOidToAudio[i].audio_path[0]+6)){
                            if(g_strFlashState.VolumeLevel == 5){
								sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[1]);
							}
							else{
                                sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
								g_strFlashState.VolumeLevel++;
								*write_flash = TRUE;
							}
						}
						*wait_play = TRUE;
                        break;
					case 7:
						if('1' == *(PaintOidToAudio[i].audio_path[0]+6)){
							g_strFlashState.EyesState = 1;
							set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_FORM,LIGHT_CONTINUE,2,
												   LIGHT_ON,LIGHT_RED,LIGHT_FORM,LIGHT_CONTINUE,2);
							
						}
						else if('2' == *(PaintOidToAudio[i].audio_path[0]+6)){
							g_strFlashState.EyesState = 2;
							set_imeek_light_struct(LIGHT_ON,LIGHT_GREEN,LIGHT_FORM,LIGHT_CONTINUE,2,
												   LIGHT_ON,LIGHT_GREEN,LIGHT_FORM,LIGHT_CONTINUE,2);
						}
						else if('3' == *(PaintOidToAudio[i].audio_path[0]+6)){
                            g_strFlashState.EyesState = 3;
							set_imeek_light_struct(LIGHT_ON,LIGHT_BLUE,LIGHT_FORM,LIGHT_CONTINUE,2,
												   LIGHT_ON,LIGHT_BLUE,LIGHT_FORM,LIGHT_CONTINUE,2);
						}
						sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
						*write_flash = TRUE;
						break;
					case 9:
						if('0' == *(PaintOidToAudio[i].audio_path[0]+6)){
							*play_state = IMEEK_PAUSE;
						}
						else if('1' == *(PaintOidToAudio[i].audio_path[0]+6)){
							*play_state = IMEEK_PLAY;
						}
						else if('2' == *(PaintOidToAudio[i].audio_path[0]+6)){
                            *play_state = IMEEK_STOP;
						}
						sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
						break;
					case 10:
                        sprintf(url,"%s%s","file://music/Paint/001/",
						  PaintOidToAudio[i].audio_path[g_strFlashState.TaskProgess1 & 0x01]);
						break;
					case 11:
						if(l_u32QuestionFlag & 0x01){
							//故事播放完毕,且回答正确,内存还未记录过该事件,则记录
							if(('1' == *(PaintOidToAudio[i].audio_path[1]+6))
							&& !(g_strFlashState.TaskProgess1 & 0x01)){
							    g_strFlashState.TaskProgess1 |= 0x01;
								*write_flash = TRUE;
//								flash_int_write(FLASH_TASK_PROGRESS1_ADDR,&g_strFlashState.TaskProgess1);
							}
						    sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[1]);
						}
						else{
							//故事没有播放完毕，先播放故事
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
							l_u32QuestionFlag |= 0x01;
							*wait_play = TRUE;
						}
						break;
					case 12:
						if(g_strFlashState.MissionLevel >= (i-30)){
							sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[1]);
						}
						else{
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
						}

						if(39 == i){
                            *wait_play = TRUE;
						}
						break;
					case 13:
						if(g_strFlashState.MissionLevel > 9){
							if(!strcmp("P19-105.mp3",PaintOidToAudio[i].audio_path[1])){
                                g_strFlashState.BadgeState1 |= 1<<2;
								*write_flash = TRUE;
							}
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[1]);
						}
						else{
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
						}
						break;
					case 14:
						if(l_u32QuestionFlag & 0x02){
						    sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[1]);	
						}
						else{
							//故事没有播放完毕，先播放故事
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
							l_u32QuestionFlag |= 0x02;
							*wait_play = TRUE;
						}						
						break;
					case 15:
						sprintf(url,"%s%s%s","file://music/Public/",g_au8PianoPlayList[OS_Rand32()%9],".mp3");
						break;
					case 16:
						if(!strcmp("P20-101.mp3",PaintOidToAudio[i].audio_path[0])){
							l_u32QuestionFlag2 = 0x01;
							sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else if(l_u32QuestionFlag2 == 0x01){
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else{
                            sprintf(url,"%s","file://music/Public/_52.mp3");
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						break;
					case 17:
						if(!strcmp("P20-104.mp3",PaintOidToAudio[i].audio_path[0])){
							l_u32QuestionFlag2 = 0x02;
							sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else if(l_u32QuestionFlag2 == 0x02){
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else{
                            sprintf(url,"%s","file://music/Public/_52.mp3");
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						break;
					case 18:
						if(!strcmp("P21-101.mp3",PaintOidToAudio[i].audio_path[0])){
							l_u32QuestionFlag2 = 0x03;
							sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else if(l_u32QuestionFlag2 == 0x03){
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else{
                            sprintf(url,"%s","file://music/Public/_52.mp3");
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						break;
					case 19:
						if(!strcmp("P21-104.mp3",PaintOidToAudio[i].audio_path[0])){
							l_u32QuestionFlag2 = 0x04;
							sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else if(l_u32QuestionFlag2 == 0x04){
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else{
                            sprintf(url,"%s","file://music/Public/_52.mp3");
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						break;
					case 20:
						if(!strcmp("P22-101.mp3",PaintOidToAudio[i].audio_path[0])){
							l_u32QuestionFlag2 = 0x5;
							sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else if(l_u32QuestionFlag2 == 0x5){
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else{
                            sprintf(url,"%s","file://music/Public/_52.mp3");
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						break;
					case 21:
						if(!strcmp("P22-104.mp3",PaintOidToAudio[i].audio_path[0])){
							l_u32QuestionFlag2 = 0x6;
							sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else if(l_u32QuestionFlag2 == 0x6){
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else{
                            sprintf(url,"%s","file://music/Public/_52.mp3");
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						break;
					case 22:
						if(!strcmp("P23-101.mp3",PaintOidToAudio[i].audio_path[0])){
							l_u32QuestionFlag2 = 0x7;
							sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else if(l_u32QuestionFlag2 == 0x7){
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else{
                            sprintf(url,"%s","file://music/Public/_52.mp3");
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						break;
					case 23:
						if(!strcmp("P23-104.mp3",PaintOidToAudio[i].audio_path[0])){
							l_u32QuestionFlag2 = 0x8;
							sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else if(l_u32QuestionFlag2 == 0x8){
                            sprintf(url,"%s%s","file://music/Paint/001/",PaintOidToAudio[i].audio_path[0]);
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						else{
                            sprintf(url,"%s","file://music/Public/_52.mp3");
//							printf("url = %s\n",url);
//							printf("l_u32QuestionFlag2 = %d\n",l_u32QuestionFlag2);
						}
						break;
					default:
					    break;
				}
			    
				break;
			}
		}
	}
	return ((i>=PAINT_OID_TO_AUDIO_NUM) ? FALSE:TRUE);
}

/*
*******************************************************************************
 *Function:     PlayMathAudio
 *Description:  根据点读笔指示位置，播放相应位置音频
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
MYBOOL PlayMathAudio(STRUCT_APP_OID_DATA* l_OidData, char* url,MYBOOL* wait_play)
{
	static uint32_t l_u32QuestionFlag = 0x00;
    int i = 0xFFFFFF;
	
    if((l_OidData->position.x > MATH_1_LOW_X) && (l_OidData->position.x < MATH_1_HIGH_X)
	&& (l_OidData->position.y > MATH_1_LOW_Y) && (l_OidData->position.y < MATH_1_HIGH_Y)){
		for(i=0;i<MATH_OID_TO_AUDIO_NUM;i++){
			//得出当前是点的哪里
			if((l_OidData->position.x > MathOidToAudio[i].low_x_axis)
			&& (l_OidData->position.x < MathOidToAudio[i].high_x_axis)
			&& (l_OidData->position.y > MathOidToAudio[i].low_y_axis)
			&& (l_OidData->position.y < MathOidToAudio[i].high_y_axis)){
			    //根据内存保存的状态确定播放哪个音频
			    switch(MathOidToAudio[i].flag){
                    case 0:
						g_strMissionState.Type = NO_QUESTION;
						sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[0]);
						break;
					case 2:
						g_strMissionState.Type = MATH_QUESTION;
						g_strMissionState.bit  = i;
						g_strMissionState.first_or_two = (l_u32QuestionFlag>>i) & 0x01;
						g_strMissionState.first_times  = FALSE;
                        sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[g_strMissionState.first_or_two]);
						*wait_play = TRUE;
						break;
					case 3:
						//数学问题才回答
						if(MATH_QUESTION == g_strMissionState.Type){
//							printf("g_strMissionState.bit = %d\n",g_strMissionState.bit);
							//该答案是否是刚刚点击的问题的答案
							if((i/3 - 1) == g_strMissionState.bit){
								sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[g_strMissionState.first_or_two]);
								//答对了
								if(!strcmp("SX-001.mp3",MathOidToAudio[i].audio_path[g_strMissionState.first_or_two])){
									//第一次答对该问题
									if(g_strMissionState.first_times == FALSE){
										g_strMissionState.first_times = TRUE;
										l_u32QuestionFlag ^= (1<<g_strMissionState.bit);
									}
								}
							}
							else{
                                sprintf(url,"%s%s","file://music/Math/001/","SX-002.mp3");
							}
						}
						else{
                            sprintf(url,"%s%s","file://music/Math/001/","SX-002.mp3");
						}
						break;
					case 4:
						g_strMissionState.Type = MATH_QUESTION;
						g_strMissionState.bit  = i;
						g_strMissionState.first_or_two = (l_u32QuestionFlag>>i) & 0x01;
						g_strMissionState.first_times  = FALSE;
                        sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[g_strMissionState.first_or_two]);
						*wait_play = TRUE;
						break;
					case 5:
						//数学问题才回答
						if(MATH_QUESTION == g_strMissionState.Type){
//							printf("g_strMissionState.bit = %d\n",g_strMissionState.bit);
							//该答案是否是刚刚点击的问题的答案
							switch(g_strMissionState.bit){
								case 18:
									if(i==25){
										sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[1]);
									}
									else{
                                        sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[0]);
									}
									break;
								case 19:
									if(i==26){
										sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[1]);
									}
									else{
                                        sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[0]);
									}
									break;
								case 20:
									if(i==24){
										sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[1]);
									}
									else{
                                        sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[0]);
									}
									break;
								case 21:
									if(i==25){
										sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[1]);
									}
									else{
                                        sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[0]);
									}
									break;
								case 22:
									if(i==24){
										sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[1]);
									}
									else{
                                        sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[0]);
									}
									break;
								case 23:
									if(i==26){
										sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[1]);
									}
									else{
                                        sprintf(url,"%s%s","file://music/Math/001/",MathOidToAudio[i].audio_path[0]);
									}
									break;
								default:
									sprintf(url,"%s%s","file://music/Math/001/","SX-002.mp3");
									break;
							}
						}
						else{
                            sprintf(url,"%s%s","file://music/Math/001/","SX-002.mp3");
						}
						break;
				    default:
					    break;
				}
				break;
				
			}
		}
	}
	return ((i>=MATH_OID_TO_AUDIO_NUM) ? FALSE:TRUE);
}

/*
*******************************************************************************
 *Function:     PlayBadgeAudio
 *Description:  根据点读笔指示位置，播放相应位置音频
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
MYBOOL PlayBadgeAudio(STRUCT_APP_OID_DATA* l_OidData, char* url)
{
    int i = 0xFFFFFF;
    if((l_OidData->position.x > BADGE_1_LOW_X) && (l_OidData->position.x < BADGE_1_HIGH_X)
	&& (l_OidData->position.y > BADGE_1_LOW_Y) && (l_OidData->position.y < BADGE_1_HIGH_Y)){
		for(i=0;i<BADGE_OID_TO_AUDIO_NUM;i++){
			if((l_OidData->position.x > BadgeOidToAudio[i].low_x_axis)
			&& (l_OidData->position.x < BadgeOidToAudio[i].high_x_axis)
			&& (l_OidData->position.y > BadgeOidToAudio[i].low_y_axis)
			&& (l_OidData->position.y < BadgeOidToAudio[i].high_y_axis)){
				sprintf(url,"%s%s","file://music/Badge/",BadgeOidToAudio[i].audio_path[0]);
				break;
			}
		}
	}
	return ((i>=BADGE_OID_TO_AUDIO_NUM) ? FALSE:TRUE);
}

/*
*******************************************************************************
 *Function:     PlayStickerAudio
 *Description:  根据点读笔指示位置，播放相应位置音频
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
MYBOOL PlayStickerAudio(STRUCT_APP_OID_DATA* l_OidData, char* url)
{
	int i = 0xFFFFFF;
	if((l_OidData->position.x > STICKER_1_LOW_X) && (l_OidData->position.x < STICKER_1_HIGH_X)
	&& (l_OidData->position.y > STICKER_1_LOW_Y) && (l_OidData->position.y < STICKER_1_HIGH_Y)){
		for(i=0;i<STICKER_OID_TO_AUDIO_NUM;i++){
			if((l_OidData->position.x > StickerOidToAudio[i].low_x_axis)
			&& (l_OidData->position.x < StickerOidToAudio[i].high_x_axis)
			&& (l_OidData->position.y > StickerOidToAudio[i].low_y_axis)
			&& (l_OidData->position.y < StickerOidToAudio[i].high_y_axis)){
			    switch(StickerOidToAudio[i].flag){
					case 0:
						sprintf(url,"%s%s","file://music/Sticker/001/",StickerOidToAudio[i].audio_path[0]);
						break;
					case 1:
						if(g_strFlashState.BadgeState1 & (1<<i)){
							sprintf(url,"%s%s","file://music/Sticker/001/",StickerOidToAudio[i].audio_path[1]);
						}
						else{
							sprintf(url,"%s%s","file://music/Sticker/001/",StickerOidToAudio[i].audio_path[0]);
						}
						break;
					default:
						break;
				}
				break;
			}
		}
	}
	return ((i>=STICKER_OID_TO_AUDIO_NUM) ? FALSE:TRUE);
}

/*
*******************************************************************************
 *Function:     PlayEnglishWordAudio
 *Description:  根据点读笔指示位置，播放相应位置音频
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
MYBOOL PlayEnglishWordAudio(STRUCT_APP_OID_DATA* l_OidData, char* url)
{
	int i = 0xFFFFFF;
	if((l_OidData->position.x > ENGLISH_WORD_LOW_X) && (l_OidData->position.x < ENGLISH_WORD_HIGH_X)
	&& (l_OidData->position.y > ENGLISH_WORD_LOW_Y) && (l_OidData->position.y < ENGLISH_WORD_HIGH_Y)){
		for(i=0;i<ENGLISH_OID_AND_AUDIO_NUM;i++){
			if((l_OidData->position.x > EnglishWordOidToAudio[i].low_x_axis)
			&& (l_OidData->position.x < EnglishWordOidToAudio[i].high_x_axis)
			&& (l_OidData->position.y > EnglishWordOidToAudio[i].low_y_axis)
			&& (l_OidData->position.y < EnglishWordOidToAudio[i].high_y_axis)){
				sprintf(url,"%s%s%s","file://music/English/Spell/",EnglishWordOidToAudio[i].audio_path,".mp3");
				break;
			}
		}
	}
	return ((i>=ENGLISH_OID_AND_AUDIO_NUM) ? FALSE:TRUE);
}

/*
*******************************************************************************
 *Function:     PlayChineseWordAudio
 *Description:  根据点读笔指示位置，播放相应位置音频
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
MYBOOL PlayChineseWordAudio(STRUCT_APP_OID_DATA* l_OidData, char* url)
{
	int i = 0xFFFFFF;
	if((l_OidData->position.x > CHINESE_WORD_LOW_X) && (l_OidData->position.x < CHINESE_WORD_HIGH_X)
	&& (l_OidData->position.y > CHINESE_WORD_LOW_Y) && (l_OidData->position.y < CHINESE_WORD_HIGH_Y)){
		for(i=0;i<CHINESE_OID_AND_AUDIO_NUM;i++){
			if((l_OidData->position.x > ChineseWordOidToAudio[i].low_x_axis)
			&& (l_OidData->position.x < ChineseWordOidToAudio[i].high_x_axis)
			&& (l_OidData->position.y > ChineseWordOidToAudio[i].low_y_axis)
			&& (l_OidData->position.y < ChineseWordOidToAudio[i].high_y_axis)){
				sprintf(url,"%s%s%s","file://music/Chinese/",ChineseWordOidToAudio[i].audio_path,".mp3");
				break;
			}
		}
	}
	return ((i>=CHINESE_OID_AND_AUDIO_NUM) ? FALSE:TRUE);
}

/*
*******************************************************************************
 *Function:     UpdateDownloadAudioFlag
 *Description:  指向了音频下载更新卡片，更新对应更新标志位
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void UpdateDownloadAudioFlag(STRUCT_APP_OID_DATA* l_OidData,STRUCT_ALL_AUDIO_UPDATE_DATA *l_aAudioUpdateData)
{
	if((l_OidData->position.x > CHINESE_AUDIO_DOWNLOAD_LOW_X) && (l_OidData->position.x < CHINESE_AUDIO_DOWNLOAD_HIGH_X)
	&& (l_OidData->position.y > CHINESE_AUDIO_DOWNLOAD_LOW_Y) && (l_OidData->position.y < CHINESE_AUDIO_DOWNLOAD_HIGH_Y)){
		l_aAudioUpdateData[CHINESE_AUDIO].Flag = TRUE;
	}
	else if((l_OidData->position.x > ENGLISH_WORD_DOWNLOAD_LOW_X) && (l_OidData->position.x < ENGLISH_WORD_DOWNLOAD_HIGH_X)
	     && (l_OidData->position.y > ENGLISH_WORD_DOWNLOAD_LOW_Y) && (l_OidData->position.y < ENGLISH_WORD_DOWNLOAD_HIGH_Y)){
		l_aAudioUpdateData[ENGLISH_AUDIO].Flag = TRUE;
	}
	else if((l_OidData->position.x > ENGLISH_LETTER_DOWNLOAD_LOW_X) && (l_OidData->position.x < ENGLISH_LETTER_DOWNLOAD_HIGH_X)
	     && (l_OidData->position.y > ENGLISH_LETTER_DOWNLOAD_LOW_Y) && (l_OidData->position.y < ENGLISH_LETTER_DOWNLOAD_HIGH_Y)){
		l_aAudioUpdateData[ENGLISH_AUDIO].Flag = TRUE;
	}
}


/*
*******************************************************************************
 *Function:     PlayImeekAudio
 *Description:  根据点读内容播放相应音频
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void PlayImeekAudio(STRUCT_APP_OID_DATA* l_OidData,MYBOOL* write_flash,
                    ENUM_OID_POINT_TO oid_point_to)
{
	MYBOOL l_bWaitPlay = FALSE;
	ENUM_PLAY_TYPE play_state = IMEEK_PLAY;
	char url_temp[100];
	static STRUCT_APP_OID_DATA l_strLastOidData = {FALSE};
	
//#if (OID_PLAY_MODE == FS_AUDIO_PLAY)
//		char file[] = "file://music/";
//#elif (OID_PLAY_MODE == HTTP_AUDIO_PLAY)
//		char file[] = "http://api.linxbot.com/tmp/";
//#endif

	//点读笔如果一直放着不动，就不重复执行
	if((l_strLastOidData.index != l_OidData->index)
	|| (l_strLastOidData.position.x != l_OidData->position.x)
	|| (l_strLastOidData.position.y != l_OidData->position.y)){
		switch(oid_point_to){
			case OID_POINT_TO_PAINT:
				PlayPaintAudio(l_OidData,url_temp,&play_state,
							   write_flash,	&l_bWaitPlay);
				PlayAudio(url_temp,THIRD_LEVEL_AUDIO,play_state);
				break;
			case OID_POINT_TO_MATH:
				PlayMathAudio(l_OidData,url_temp,&l_bWaitPlay);
				PlayAudio(url_temp,THIRD_LEVEL_AUDIO,play_state);
				break;
			case OID_POINT_TO_BADGE:
				PlayBadgeAudio(l_OidData,url_temp);
				PlayAudio(url_temp,THIRD_LEVEL_AUDIO,play_state);
				break;
			case OID_POINT_TO_STICKER:
				PlayStickerAudio(l_OidData,url_temp);
				PlayAudio(url_temp,THIRD_LEVEL_AUDIO,play_state);
				break;
			case OID_POINT_TO_ENGLISH_WORD:
				PlayEnglishWordAudio(l_OidData,url_temp);
				PlayAudio(url_temp,THIRD_LEVEL_AUDIO,play_state);
				break;
			case OID_POINT_TO_CHINESE_WORD:
				PlayChineseWordAudio(l_OidData,url_temp);
				PlayAudio(url_temp,THIRD_LEVEL_AUDIO,play_state);
				break;
			case OID_POINT_TO_DOWNLOAD_AUDIO:
				UpdateDownloadAudioFlag(l_OidData,g_aAudioUpdateData);
				break;
			case OID_POINT_TO_NULL:
				break;
		}
		//如果是需要等待播放完成的音频，则等待
		WaitAudioPlayOver(l_bWaitPlay);
	}
	l_strLastOidData.code_flag = l_OidData->code_flag;
	l_strLastOidData.index = l_OidData->index;
	l_strLastOidData.position.x = l_OidData->position.x;
	l_strLastOidData.position.y = l_OidData->position.y;
}

/*
*******************************************************************************
 *Function:     MissionFreeSpellModeInit
 *Description:  自由模式、任务模式与单词拼写模式初始化
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void MissionFreeSpellModeInit(void)
{
	/*任务模式，且处于初始化阶段*/
	if(VARIABLE_INIT == imeek_work_state.current_mode_state){
//		printf("MissionFreeSpellModeInit\n");
		adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
		imeek_work_state.direction = MEEK_X_AXIS_LOW_TO_HIGH;
		imeek_work_state.current_mode_state= GET_INSTRUCTION_START;
		imeek_work_state.loop_sequence = 0;
		imeek_work_state.start_flag = 0;
	}
}

/*
*******************************************************************************
 *Function:     GetLevelBlock
 *Description:  获取当前关卡的终点地图块
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void GetLevelBlock(uint32_t level, uint32_t map)
{
	uint32_t i;
	if(TRAILING_MODE != imeek_work_state.state){
		if(TASK_MODE == imeek_work_state.state){
			for(i=0; i<MAP_LEVEL_NUM; i++){
				if(level == map_level_block[map][i].level){
					imeek_work_state.level_row	  = map_level_block[map][i].row;
					imeek_work_state.level_column = map_level_block[map][i].column;
					break;
				}
			}
		}
	}
}

/*
*******************************************************************************
 *Function:     GetMap
 *Description:  获取点读笔当前点了哪张地图
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void GetMap(STRUCT_APP_OID_DATA *l_OidData,uint32_t *map)
{
#define MAP_2_LOW_X  405
#define MAP_2_HIGH_X 2642
#define MAP_2_LOW_Y  10000
#define MAP_2_HIGH_Y 10189

	//地图1迪瓦尔王国
	if((l_OidData->position.x > MAP_2_LOW_X) && (l_OidData->position.x < MAP_2_HIGH_X)
	&&(l_OidData->position.y > MAP_2_LOW_Y) && (l_OidData->position.y < MAP_2_HIGH_Y)){
		*map = 1;
	}
	//地图2。。。
}

/*
*******************************************************************************
 *Function:     GetLevel
 *Description:  获取点读笔点了哪个关卡
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void GetLevel(STRUCT_APP_OID_DATA *l_OidData,uint32_t *l_u32Level)
{
	int i;

	//关卡数9
	for(i=31;i<40;i++){
		if((l_OidData->position.x > PaintOidToAudio[i].low_x_axis)
		&& (l_OidData->position.x < PaintOidToAudio[i].high_x_axis)
		&& (l_OidData->position.y > PaintOidToAudio[i].low_y_axis)
		&& (l_OidData->position.y < PaintOidToAudio[i].high_y_axis)){
		   //当前是否已经过到了这关，没过到放弃此次读取
		   if(g_strFlashState.MissionLevel >= (i-30)){
			   *l_u32Level = i-30;
		   }
		   break;
		}
	}
}

/*
*******************************************************************************
 *Function:     GetInstructionCard
 *Description:  实时获取指令卡--单张
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:扫描到了新卡片
 *              FALSE:没有扫描到卡片，或扫描的卡片不是新卡片
 *Others:       本函数检测的结果是没有debounce前的结果
*******************************************************************************
*/
MYBOOL GetInstructionCard(STRUCT_APP_OID_DATA	*l_OidData,ENUM_INSTRUCTION_CARD_VARIETY* card)
{
	uint32_t i;
	MYBOOL ret = FALSE;
	uint32_t new_low_target;
	uint32_t new_high_target;
	static uint16_t last_x = 0;
	static uint32_t old_high_target = 0;

//	printf("last_x = %d\n", last_x);
	*card = NO_INSTRUCTION_CARD;
	if((POSITION_CODE == l_OidData->code_flag)
	&& (g_arr_instuction_card[INSTRUCTION_START].low_x_axis <= l_OidData->position.x)
	&& (g_arr_instuction_card[PARAMETER_VOICE_2-1].high_x_axis >= l_OidData->position.x)
	&& (g_arr_instuction_card[INSTRUCTION_START].low_y_axis <= l_OidData->position.y)
	&& (g_arr_instuction_card[INSTRUCTION_START].high_y_axis >= l_OidData->position.y)){
		for(i=0; i<CARD_NUM; i++){
			if(INSTRUCTION_END != g_arr_instuction_card[i].instruction_card){
				new_low_target = g_arr_instuction_card[i].low_x_axis+8;
				new_high_target = g_arr_instuction_card[i].high_x_axis-8;
			}
			else{
				new_low_target = g_arr_instuction_card[i].low_x_axis+38;
				new_high_target = g_arr_instuction_card[i].high_x_axis;
			}

			//找到当前坐标属于哪张卡片
			if((l_OidData->position.x <= new_high_target)
			&& (l_OidData->position.x >= new_low_target)){
				//与上一次检测到的卡是否相同
				if(old_high_target == new_high_target){
					//当前横坐标小于上一次的值，且差值大于阈值
					if(l_OidData->position.x < last_x-10){
						//找到新的卡片
						ret = TRUE;
					}
				}
				else{
					//找到新的卡片
					ret = TRUE;
				}
				*card = g_arr_instuction_card[i].instruction_card;
				break;
			}
		}
		if(i < CARD_NUM){
			last_x = l_OidData->position.x;
			old_high_target = new_high_target;
		}
	}
	
	return ret;
}

/*
*******************************************************************************
 *Function:     GetDebounceInstruction
 *Description:  过滤拼接卡片中的错误卡片
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       因为点读笔数据很容易出错，或者没有检测到，因此判断产生新卡片的
 *              方式两种方法要同时用上：
 *              法1：当检测到圈图码0的时候，说明产生了新卡片
 *              法2：当连续检测到两次新卡片的位置码时，认为检测到新卡片
*******************************************************************************
*/
ENUM_INSTRUCTION_CARD_VARIETY GetDebounceInstruction(MYBOOL state, ENUM_INSTRUCTION_CARD_VARIETY card)
{
	static int index = 0;
	static ENUM_INSTRUCTION_CARD_VARIETY l_LastCard = NO_INSTRUCTION_CARD;
	static ENUM_INSTRUCTION_CARD_VARIETY l_SureCard = NO_INSTRUCTION_CARD;
	ENUM_INSTRUCTION_CARD_VARIETY ret_card = NO_INSTRUCTION_CARD;

	//检测到一次有效卡片,记录下来
	if(FALSE == state){
		if(card != NO_INSTRUCTION_CARD){
			switch(index){
				case 0:
					l_LastCard = card;
					index++;
					break;
				case 1:
					if(card == l_LastCard){
						l_SureCard = card;
						index++;
					}
					else{
						l_LastCard = card;
					}
					break;
				default:
					if(INSTRUCTION_START == l_SureCard){
						ret_card = l_SureCard;
						index = 0;
						l_LastCard = NO_INSTRUCTION_CARD;
						l_SureCard = NO_INSTRUCTION_CARD;
					}
					break;
			}
		}
		//如果检测到了停止卡片，则返回收到停止卡片
		if(INSTRUCTION_END == l_SureCard){
			ret_card = l_SureCard;
			index = 0;
			l_LastCard = NO_INSTRUCTION_CARD;
			l_SureCard = NO_INSTRUCTION_CARD;
		}
	}
	//检测到新的卡片的开始，则将之前检测的卡片弹出
	else {
		ret_card = l_SureCard;
		index = 0;
		l_LastCard = NO_INSTRUCTION_CARD;
		l_SureCard = NO_INSTRUCTION_CARD;
	}
	return ret_card;
}

/*
*******************************************************************************
 *Function:     GetSpellInstructionCard
 *Description:  实时获取英文单词拼写的指令卡--单张
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:结束上张卡片，开始新卡片
 *              FALSE:上张卡片还没有扫描结束
 *Others:       
*******************************************************************************
*/
MYBOOL GetSpellInstructionCard(STRUCT_APP_OID_DATA *l_OidData,ENUM_ENGLISH_SPELL_CARD *card)
{
    uint32_t i;
	uint32_t new_low_target;
	uint32_t new_high_target;
	static uint32_t old_high_target = 0;
	static uint16_t last_x = 0;
	MYBOOL ret = FALSE;

#if (GET_RT_OID_DATA == 1)
	datatemp[indextemp++] = l_OidData->code_flag;
	datatemp[indextemp++] = l_OidData->index;
	datatemp[indextemp++] = l_OidData->position.x;
	datatemp[indextemp++] = l_OidData->position.y;
#endif

	*card = NO_ENGLISH_SPELL_CARD;
	if((POSITION_CODE == l_OidData->code_flag)
	&& (g_arrEnglishSpellInstructionCard[ENGLISH_SPELL_A].low_x_axis <= l_OidData->position.x)
	&& (g_arrEnglishSpellInstructionCard[ENGLISH_SPELL_END].high_x_axis >= l_OidData->position.x)
	&& (g_arrEnglishSpellInstructionCard[ENGLISH_SPELL_END].low_y_axis <= l_OidData->position.y)
	&& (g_arrEnglishSpellInstructionCard[ENGLISH_SPELL_END].high_y_axis >= l_OidData->position.y)){
		for(i=0; i<SPELL_CARD_NUM; i++){
			if(ENGLISH_SPELL_END != g_arrEnglishSpellInstructionCard[i].instruction_card){
				new_low_target = g_arrEnglishSpellInstructionCard[i].low_x_axis+8;
				new_high_target = g_arrEnglishSpellInstructionCard[i].high_x_axis-8;
			}
			else{
				new_low_target = g_arrEnglishSpellInstructionCard[i].low_x_axis+33;
				new_high_target = g_arrEnglishSpellInstructionCard[i].high_x_axis;
			}

			//找到当前坐标属于哪张卡片
			if((l_OidData->position.x <= new_high_target)
			&& (l_OidData->position.x >= new_low_target)){
				//与上一次检测到的卡是否相同
				if(old_high_target == new_high_target){
					//当前横坐标小于上一次的值，且差值大于阈值
					if(l_OidData->position.x < last_x-10){
						//找到新的卡片
						ret = TRUE;
					}
				}
				else{
					//找到新的卡片
					ret = TRUE;
				}
				*card = g_arrEnglishSpellInstructionCard[i].instruction_card;
				break;
			}
		}
		if(i < SPELL_CARD_NUM){
			last_x = l_OidData->position.x;
			old_high_target = new_high_target;
		}
	}
//	else if((GENERAL_CODE == l_OidData->code_flag)
//		   && (0 == l_OidData->index)){
//		ret = TRUE;
//	}

	return ret;
}


uint8_t GetInstructionDirection(STRUCT_APP_OID_DATA *l_OidData)
{
    if((POSITION_CODE == l_OidData->code_flag)
    && (l_OidData->position.y < INSTRUCTION_MIDDLE_Y+3)
	&& (l_OidData->position.y > INSTRUCTION_MIDDLE_Y-3)
	&& (l_OidData->position.x > INSTRUCTION_MIDDLE_X)){
        return RETURN_TRUE;
    }
	adj_motor_state(1,MOTOR_BACKWARD,1,MOTOR_FORWARD);
	return RETURN_ERROR_1;
}

/*
*******************************************************************************
 *Function:     GetSpellCardDirection
 *Description:  获取字母拼写卡开始卡片的方向
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
uint8_t GetSpellCardDirection(STRUCT_APP_OID_DATA *l_OidData)
{
    if((POSITION_CODE == l_OidData->code_flag)
    && (l_OidData->position.y < SPELL_MIDDLE_Y+3)
	&& (l_OidData->position.y > SPELL_MIDDLE_Y-3)
	&& (l_OidData->position.x > SPELL_MIDDLE_X)){
        return RETURN_TRUE;
    }
	adj_motor_state(1,MOTOR_BACKWARD,1,MOTOR_FORWARD);
	return RETURN_ERROR_1;
}

void follow_the_instruction_card(STRUCT_APP_OID_DATA *l_OidData)
{
#define IMEEK_INSTRUCTION_CARD_SPEED 8
	if((l_OidData->position.y >= g_arr_instuction_card[0].low_y_axis) 
	&&(l_OidData->position.y < (g_arr_instuction_card[0].low_y_axis + 16))){
	    adj_motor_state(4,MOTOR_FORWARD,0,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y >= (g_arr_instuction_card[0].low_y_axis + 16)) 
	     &&(l_OidData->position.y <  (g_arr_instuction_card[0].low_y_axis + 18))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED-4,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y >= (g_arr_instuction_card[0].low_y_axis + 18)) 
	     &&(l_OidData->position.y <  (g_arr_instuction_card[0].low_y_axis + 20))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED-3,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y >= (g_arr_instuction_card[0].low_y_axis + 20)) 
	     &&(l_OidData->position.y <  (g_arr_instuction_card[0].low_y_axis + 22))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED-2,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y >= (g_arr_instuction_card[0].low_y_axis + 22)) 
	     &&(l_OidData->position.y <  INSTRUCTION_MIDDLE_Y-2)){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED-1,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y >= INSTRUCTION_MIDDLE_Y-2) 
	     && (l_OidData->position.y <= INSTRUCTION_MIDDLE_Y+2)){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y > INSTRUCTION_MIDDLE_Y+2) 
	     &&(l_OidData->position.y <= (g_arr_instuction_card[0].high_y_axis - 22))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED-1,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y > (g_arr_instuction_card[0].high_y_axis - 22)) 
	     && (l_OidData->position.y <=  (g_arr_instuction_card[0].high_y_axis - 20))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED-2,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y > (g_arr_instuction_card[0].high_y_axis - 20)) 
	     && (l_OidData->position.y <=  (g_arr_instuction_card[0].high_y_axis - 18))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED-3,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y > (g_arr_instuction_card[0].high_y_axis - 18)) 
	     && (l_OidData->position.y <=  (g_arr_instuction_card[0].high_y_axis - 16))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED-4,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y > (g_arr_instuction_card[0].high_y_axis - 16)) 
	     && (l_OidData->position.y <=  g_arr_instuction_card[0].high_y_axis)){
	    adj_motor_state(0,MOTOR_FORWARD,4,MOTOR_FORWARD);
	}
}

/*
*******************************************************************************
 *Function:     FollowSpellCard
 *Description:  根据读到的OID码值，调整imeek左右轮的速度
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void FollowSpellCard(STRUCT_APP_OID_DATA *l_OidData)
{
#define IMEEK_INSTRUCTION_CARD_SPEED 8
	if((l_OidData->position.y >= g_arrEnglishSpellInstructionCard[0].low_y_axis) 
	&&(l_OidData->position.y < (g_arrEnglishSpellInstructionCard[0].low_y_axis + 16))){
	    adj_motor_state(4,MOTOR_FORWARD,0,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y >= (g_arrEnglishSpellInstructionCard[0].low_y_axis + 16)) 
	     &&(l_OidData->position.y <  (g_arrEnglishSpellInstructionCard[0].low_y_axis + 18))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED-4,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y >= (g_arrEnglishSpellInstructionCard[0].low_y_axis + 18)) 
	     &&(l_OidData->position.y <  (g_arrEnglishSpellInstructionCard[0].low_y_axis + 20))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED-3,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y >= (g_arrEnglishSpellInstructionCard[0].low_y_axis + 20)) 
	     &&(l_OidData->position.y <  (g_arrEnglishSpellInstructionCard[0].low_y_axis + 22))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED-2,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y >= (g_arrEnglishSpellInstructionCard[0].low_y_axis + 22)) 
	     &&(l_OidData->position.y <  SPELL_MIDDLE_Y-2)){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED-1,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y >= SPELL_MIDDLE_Y-2) 
	     && (l_OidData->position.y <= SPELL_MIDDLE_Y+2)){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y > SPELL_MIDDLE_Y+2) 
	     &&(l_OidData->position.y <= (g_arrEnglishSpellInstructionCard[0].high_y_axis - 22))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED-1,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y > (g_arrEnglishSpellInstructionCard[0].high_y_axis - 22)) 
	     && (l_OidData->position.y <=  (g_arrEnglishSpellInstructionCard[0].high_y_axis - 20))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED-2,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y > (g_arrEnglishSpellInstructionCard[0].high_y_axis - 20)) 
	     && (l_OidData->position.y <=  (g_arrEnglishSpellInstructionCard[0].high_y_axis - 18))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED-3,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y > (g_arrEnglishSpellInstructionCard[0].high_y_axis - 18)) 
	     && (l_OidData->position.y <=  (g_arrEnglishSpellInstructionCard[0].high_y_axis - 16))){
	    adj_motor_state(IMEEK_INSTRUCTION_CARD_SPEED-4,MOTOR_FORWARD,
			            IMEEK_INSTRUCTION_CARD_SPEED,MOTOR_FORWARD);
	}
	else if((l_OidData->position.y > (g_arrEnglishSpellInstructionCard[0].high_y_axis - 16)) 
	     && (l_OidData->position.y <=  g_arrEnglishSpellInstructionCard[0].high_y_axis)){
	    adj_motor_state(0,MOTOR_FORWARD,4,MOTOR_FORWARD);
	}
}

/*
*******************************************************************************
 *Function:     GetTrueSpellCard
 *Description:  扫描检测到的卡片数据，返回出现频率最高的卡片
 *              如果都不相同，返回没识别到卡片
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
ENUM_ENGLISH_SPELL_CARD GetTrueSpellCard(ENUM_ENGLISH_SPELL_CARD card_1,
                                         ENUM_ENGLISH_SPELL_CARD card_2,
                                         ENUM_ENGLISH_SPELL_CARD card_3)
{
	if((card_1 == card_2)
	|| (card_1 == card_3)){
		return card_1;
	}
	else if(card_2 == card_3){
		return card_2;
	}
	return NO_ENGLISH_SPELL_CARD;
}

/*
*******************************************************************************
 *Function:     GetdebounceSpellData
 *Description:  过滤拼接卡片中的错误卡片
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       因为点读笔数据很容易出错，或者没有检测到，因为判断产生新卡片的
 *              方式两种方法要同时用上：
 *              法1：当检测到圈图码0的时候，说明产生了新卡片
 *              法2：当连续检测到两次新卡片的位置码时，认为检测到新卡片
*******************************************************************************
*/
ENUM_ENGLISH_SPELL_CARD GetdebounceSpellData(MYBOOL state, ENUM_ENGLISH_SPELL_CARD card)
{
	static int index = 0;
	static ENUM_ENGLISH_SPELL_CARD l_LastCard = NO_ENGLISH_SPELL_CARD;
	static ENUM_ENGLISH_SPELL_CARD l_SureCard = NO_ENGLISH_SPELL_CARD;
	ENUM_ENGLISH_SPELL_CARD ret_card = NO_ENGLISH_SPELL_CARD;

	//检测到一次有效卡片,记录下来
	if(FALSE == state){
		if(card != NO_ENGLISH_SPELL_CARD){
			switch(index){
				case 0:
					l_LastCard = card;
					index++;
					break;
				case 1:
					if(card == l_LastCard){
						l_SureCard = card;
						index++;
					}
					else{
						l_LastCard = card;
					}
					break;
				default:
					if(ENGLISH_SPELL_START == l_SureCard){
						ret_card = l_SureCard;
						index = 0;
						l_LastCard = NO_ENGLISH_SPELL_CARD;
						l_SureCard = NO_ENGLISH_SPELL_CARD;
					}
					break;
			}
		}
		//如果检测到了停止卡片，则返回收到停止卡片
		if(ENGLISH_SPELL_END == l_SureCard){
			ret_card = l_SureCard;
			index = 0;
			l_LastCard = NO_ENGLISH_SPELL_CARD;
			l_SureCard = NO_ENGLISH_SPELL_CARD;
		}
	}
	//检测到新的卡片的开始，则将之前检测的卡片弹出
	else {
		ret_card = l_SureCard;
		index = 0;
		l_LastCard = NO_ENGLISH_SPELL_CARD;
		l_SureCard = NO_ENGLISH_SPELL_CARD;
	}
	return ret_card;
}

/*
*******************************************************************************
 *Function:     GetInstruction
 *Description:  获取指令卡
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
static void GetInstruction(STRUCT_APP_OID_DATA *l_OidData)
{
    //读指令卡分为3个状态:
    //状态0: 识别开始指令卡
    //状态1: 识别方向
    //状态2: 识别其他卡片
    //状态3: 识别结束指令卡
//	uint8_t i;
	MYBOOL state;
	ENUM_INSTRUCTION_CARD_VARIETY l_enumRTCard;
	ENUM_INSTRUCTION_CARD_VARIETY l_enumDebounceCard;
	char path[50];

	switch(imeek_work_state.current_mode_state){
		case GET_INSTRUCTION_START:
			if(POSITION_CODE == l_OidData->code_flag){
				state = GetInstructionCard(l_OidData,&l_enumRTCard);
				l_enumDebounceCard = GetDebounceInstruction(state, l_enumRTCard);
				if(INSTRUCTION_START == l_enumDebounceCard){
					sprintf(path,"%s","file://music/Public/_42.mp3");
					PlayAudio(path,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
//					printf("start instruction\n");
					set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2,
										   LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
					imeek_work_state.current_mode_state= GET_INSTRUCTION_DIRECTION;
					user_instruction_card_index = 0;
				}
			}
			break;
		case GET_INSTRUCTION_DIRECTION:
//			printf("GetInstructionDirection\n");
			if(RETURN_TRUE == GetInstructionDirection(l_OidData)){
				imeek_work_state.current_mode_state= GET_INSTRUCTION_EXECUTE;
			}
			break;
		case GET_INSTRUCTION_EXECUTE:
//			printf("get_instruction_execute\n");
			if(POSITION_CODE == l_OidData->code_flag){
				follow_the_instruction_card(l_OidData);
				state = GetInstructionCard(l_OidData,&l_enumRTCard);
				l_enumDebounceCard = GetDebounceInstruction(state, l_enumRTCard);
				switch(l_enumDebounceCard){
					case NO_INSTRUCTION_CARD:
						break;
					case INSTRUCTION_START:
						break;
					case INSTRUCTION_END:
						imeek_work_state.current_mode_state= GET_INSTRUCTION_END;
						break;
					default:
						user_instruction_card[user_instruction_card_index++] = l_enumDebounceCard;
						break;
				}
			}
			break;
		case GET_INSTRUCTION_END:
			//测试检测到的卡片
//			printf("get_instruction_end\n");
//			APP_MSG_DBG("user_instruction_card: \n");
//			for(i=0; i<user_instruction_card_index; i++){
//				printf("%d ",user_instruction_card[i]);
//			}
//			printf("\n");
			imeek_work_state.current_mode_state = DETECT_INSTRUCTION_START;
			adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);			
			break;
		default:
			break;
	}
}

/*
*******************************************************************************
 *Function:     GetSpellInstruction
 *Description:  获取英文单词拼写的指令卡
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
static void GetSpellInstruction(STRUCT_APP_OID_DATA *l_OidData)
{
    //读指令卡分为3个状态:
    //状态0: 识别开始指令卡
    //状态1: 识别方向
    //状态2: 识别其他卡片
    //状态3: 识别结束指令卡
//    uint32_t i;
    MYBOOL state;
	ENUM_ENGLISH_SPELL_CARD l_enumRTCard;
	ENUM_ENGLISH_SPELL_CARD l_enumDebounceCard;
	char path[50];

	switch(imeek_work_state.current_mode_state){
		case GET_INSTRUCTION_START:
			state = GetSpellInstructionCard(l_OidData,&l_enumRTCard);
			l_enumDebounceCard = GetdebounceSpellData(state,l_enumRTCard);
			if(ENGLISH_SPELL_START == l_enumDebounceCard){
				sprintf(path,"%s","file://music/Public/_42.mp3");
				PlayAudio(path,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
				set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2,
									   LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
				imeek_work_state.current_mode_state= GET_INSTRUCTION_DIRECTION;
				user_instruction_card_index = 0;
			}
			break;
		case GET_INSTRUCTION_DIRECTION:
		    if(RETURN_TRUE == GetSpellCardDirection(l_OidData)){
			    imeek_work_state.current_mode_state= GET_INSTRUCTION_EXECUTE;
			}
			break;
		case GET_INSTRUCTION_EXECUTE:
			FollowSpellCard(l_OidData);
			state = GetSpellInstructionCard(l_OidData,&l_enumRTCard);
			l_enumDebounceCard = GetdebounceSpellData(state,l_enumRTCard);
			switch(l_enumDebounceCard){
				case NO_ENGLISH_SPELL_CARD:
					break;
				case ENGLISH_SPELL_START:
					break;
				case ENGLISH_SPELL_END:
					imeek_work_state.current_mode_state = GET_INSTRUCTION_END;
					break;
				default:
					user_instruction_card[user_instruction_card_index++] = l_enumDebounceCard;
					break;
			}
			break;
		case GET_INSTRUCTION_END:
			adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
			imeek_work_state.current_mode_state = DETECT_INSTRUCTION_START;
#if (GET_RT_OID_DATA == 1)
			//测试检测到的卡片
			printf("get_instruction_end\n");
			APP_MSG_DBG("user_instruction_card: \n");
			for(i=0; i<user_instruction_card_index; i++){
				printf("%c ",user_instruction_card[i]+'a');
			}
			printf("\n");
			
			for(i=0;i<3000;i++){
				printf("datatemp.codeflag: %d\n",datatemp[i]);
				i++;
				printf("datatemp[%d]: %d\n",i,datatemp[i]);
				i++;
				printf("datatemp[%d]: %d\n",i,datatemp[i]);
				i++;
				printf("datatemp[%d]: %d\n",i,datatemp[i]);
			}
#endif
			break;
		default:
			break;
	}
}


void DetectInstruction(uint32_t *instruction_data,uint32_t num)
{
    int k;
    uint8_t i,j;
	uint8_t parameter_detect_ok = FALSE;
	uint8_t error_data_flag = 0;
	char *l_u8RightPath = "file://music/Public/_45.mp3";
	char *l_u8ErrorPath = "file://music/Public/_46.mp3";

	switch(imeek_work_state.current_mode_state){
	    case DETECT_INSTRUCTION_START:
			imeek_work_state.current_mode_state= DETECT_INSTRUCTION_EXECUTE;
			imeek_work_state.block_num  = num;
			break;
		case DETECT_INSTRUCTION_EXECUTE:
//			printf("imeek_work_state.loop_sequence = %d\n",imeek_work_state.loop_sequence);
			//有几张指令卡,就需要检测几次
			for(k=0; k < imeek_work_state.block_num ; k += 2){
//				printf("k = %d\n",k);
				//每次检测都需要与所有指令卡一一比较
				for(i=0; i < INSTRUCTION_CARD_NUM; i++){
//					printf("i = %d\n",i);
					//指令卡是否正确
					if(code_instruction_card[i].instruction_card  == instruction_data[k]){
						//loop_sequence变量小于0说明右循环放在了左循环前，有错误
					   if(instruction_data[k] == code_instruction_card[4].instruction_card){
							parameter_detect_ok = TRUE;
							imeek_work_state.loop_sequence--;
							k -= 1;
						}
						else{
							if(instruction_data[k] == code_instruction_card[3].instruction_card){
								imeek_work_state.loop_sequence++;
							}
							//如果指令卡匹配，则匹配参数
							for(j=0; j < code_instruction_card[i].num; j++){
//								printf("j = %d\n",j);
								if(code_instruction_card[i].parameter_value[j] == instruction_data[k+1]){
									parameter_detect_ok = TRUE;
									break;
								}
							}
							//参数不正确
							if(j == code_instruction_card[i].num){								
//								PlayAudio(l_u8ErrorPath,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
								set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2,
													   LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
								APP_MSG_DBG("parameter error\n");
								error_data_flag = 1;
//								imeek_work_state.current_mode_state= GET_INSTRUCTION_START;
							}
						}
						if(imeek_work_state.loop_sequence < 0){
							error_data_flag = 1;
						}
//						printf("imeek_work_state.loop_sequence = %d\n",imeek_work_state.loop_sequence);
					}
					if(parameter_detect_ok){
//						printf("test1\n");
						parameter_detect_ok = FALSE;
						break;
					}
				}
//				printf("test2\n");
				//指令卡不正确
				if(i == INSTRUCTION_CARD_NUM){
//					PlayAudio(l_u8ErrorPath,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
					set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2,
										   LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
					APP_MSG_DBG("instruction error\n");
					error_data_flag = 1;
//					imeek_work_state.current_mode_state= GET_INSTRUCTION_START;
				}
			}
			
			//指令卡摆放有问题
			if((1 == error_data_flag) || (0 != imeek_work_state.loop_sequence)){
//				printf("right loop error\n");
				PlayAudio(l_u8ErrorPath,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
				imeek_work_state.current_mode_state = GET_INSTRUCTION_START;
			}
			else{
				PlayAudio(l_u8RightPath,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
				set_imeek_light_struct(LIGHT_ON,LIGHT_RED  ,LIGHT_BRIGHT,LIGHT_CONTINUE,2,
									   LIGHT_ON,LIGHT_GREEN,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
//				printf("instruction and parameter true\n");
				imeek_work_state.current_mode_state = DETECT_INSTRUCTION_END;
			}
		    break;
		case DETECT_INSTRUCTION_END:
			imeek_work_state.current_mode_state = EXECUTE_INSTRUCTION_PREPARE;
		    break;
		default:
			break;
	}
}

/*
*******************************************************************************
 *Function:     PlayEnglishSpellCard
 *Description:  根据读取到的指令卡片播放该内容对应的音频
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void PlayEnglishSpellCard(uint32_t *instruction_data,uint32_t num)
{
	uint32_t i;
	char l_s8Card[30] = {0};
	char l_s8Url[60] = {0};
	int ret;
	char *l_u8ErrorPath = "file://music/Public/_46.mp3";

	if(DETECT_INSTRUCTION_START == imeek_work_state.current_mode_state){
		imeek_work_state.block_num = num;
		for(i = 0;i<imeek_work_state.block_num;i++){
			l_s8Card[i] = (char)instruction_data[i]+'a';
		}
		sprintf(l_s8Card,"%s%s",l_s8Card,".mp3");
		ret = imeek_cmp_file_name("music/English/Spell",l_s8Card);
		if(RETURN_TRUE == ret){
			sprintf(l_s8Url,"%s%s","file://music/English/Spell/",l_s8Card);
//			printf("l_s8Url = %s\n",l_s8Url);
			PlayAudio(l_s8Url,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
		}
		else{
//			printf("No this english word\n");
			PlayAudio(l_u8ErrorPath,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
		}
		imeek_work_state.current_mode_state = VARIABLE_INIT;
	}
}


//获取当前小车处于地图哪个模块中
uint8_t get_map_block(STRUCT_APP_OID_DATA *l_OidData, STRUCT_BLOCK *block, uint32_t map)
{
    uint32_t i;

//	APP_MSG_DBG("l_OidData->position.x = %d\n",l_OidData->position.x);
//	APP_MSG_DBG("l_OidData->position.y = %d\n",l_OidData->position.y);
	
	for(i=0; i<MAP_COLUMN; i++){
		if((l_OidData->position.x < map_block_data[map][0][i].high_x_axis)
		&& (l_OidData->position.x >= map_block_data[map][0][i].low_x_axis)){
			block->column = i;
			break;
		}
	}
	if(MAP_COLUMN == i){
		return RETURN_ERROR_3;
	}
	
	for(i=0; i<MAP_ROW; i++){
		if((l_OidData->position.y < map_block_data[map][i][0].high_y_axis)
		&& (l_OidData->position.y >= map_block_data[map][i][0].low_y_axis)){
			block->row = i;
			break;
		}
	}
	if(MAP_ROW == i){
		return RETURN_ERROR_3;
	}

    if(MAP_NO == map_block_data[map][block->row][block->column].barrier){
		return RETURN_ERROR_3;
	}
	
	return RETURN_TRUE;
}

/*自由模式在执行卡片前要先确定开始的地图坐标与前进方向*/
uint8_t get_start_direction(STRUCT_APP_OID_DATA *l_OidData, uint32_t map)
{
    uint8_t res = RETURN_ERROR_1;
	uint32_t l1_y;
	uint32_t l2_y;
    static uint8_t state = 0;
    static STRUCT_BLOCK start_block = {10,10};

    if((TASK_MODE == imeek_work_state.state) && (POSITION_CODE == l_OidData->code_flag)){
		adj_motor_state(2,MOTOR_BACKWARD,2,MOTOR_FORWARD);
		if((l_OidData->position.x > ((map_block_data[map][0][0].low_x_axis + map_block_data[map][0][0].high_x_axis) / 2))
	    &&(l_OidData->position.y > (((map_block_data[map][0][0].low_y_axis + map_block_data[map][0][0].high_y_axis) / 2)-3))
		&&(l_OidData->position.y < (((map_block_data[map][0][0].low_y_axis + map_block_data[map][0][0].high_y_axis) / 2)+3))){
		    res = RETURN_TRUE;
		}
	}

	if((FREE_MODE == imeek_work_state.state) && (POSITION_CODE == l_OidData->code_flag)){
		switch(state){
		    case 0:
				//获取方向
				get_map_block(l_OidData,&start_block,map);
				l1_y = get_l1_y(l_OidData,&start_block,map);
				l2_y = get_l2_y(l_OidData,&start_block,map);
				if((l_OidData->position.y >= l1_y) && (l_OidData->position.y >= l2_y)){
					imeek_work_state.direction = MEEK_Y_AXIS_LOW_TO_HIGH;
				}
				else if((l_OidData->position.y >= l1_y) && (l_OidData->position.y <= l2_y)){
					imeek_work_state.direction = MEEK_X_AXIS_LOW_TO_HIGH;
				}
				else if((l_OidData->position.y <= l1_y) && (l_OidData->position.y >= l2_y)){
					imeek_work_state.direction = MEEK_X_AXIS_HIGH_TO_LOW;
				}
				else if((l_OidData->position.y <= l1_y) && (l_OidData->position.y <= l2_y)){
					imeek_work_state.direction = MEEK_Y_AXIS_HIGH_TO_LOW;
				}
				state = 1;
//				printf("l1_y = %d\n",l1_y);
//				printf("l2_y = %d\n",l2_y);
//				printf("imeek_work_state.direction = %d\n",imeek_work_state.direction);
				break;
			case 1:
				//根据方向，矫正小车位置
				if(MEEK_Y_AXIS_LOW_TO_HIGH == imeek_work_state.direction){
					if((l_OidData->position.x > (((map_block_data[map][start_block.row][start_block.column].low_x_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_x_axis)
											   / 2) - 3))
					&&(l_OidData->position.x < (((map_block_data[map][start_block.row][start_block.column].low_x_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_x_axis)
											   / 2) + 3))
					&&(l_OidData->position.y > ((map_block_data[map][start_block.row][start_block.column].low_y_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_y_axis)
											   / 2))){
						adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
						res = RETURN_TRUE;
						state = 0;
					}
				}
				else if(MEEK_Y_AXIS_HIGH_TO_LOW == imeek_work_state.direction){
					if((l_OidData->position.x > (((map_block_data[map][start_block.row][start_block.column].low_x_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_x_axis)
											   / 2) - 3))
					&&(l_OidData->position.x < (((map_block_data[map][start_block.row][start_block.column].low_x_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_x_axis)
											   / 2) + 3))
					&&(l_OidData->position.y < ((map_block_data[map][start_block.row][start_block.column].low_y_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_y_axis)
											   / 2))){
						adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
						res = RETURN_TRUE;
						state = 0;
					}
				}
				else if(MEEK_X_AXIS_LOW_TO_HIGH == imeek_work_state.direction){
					if((l_OidData->position.y > (((map_block_data[map][start_block.row][start_block.column].low_y_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_y_axis)
											   / 2) - 3))
					&&(l_OidData->position.y < (((map_block_data[map][start_block.row][start_block.column].low_y_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_y_axis)
											   / 2) + 3))
					&&(l_OidData->position.x > ((map_block_data[map][start_block.row][start_block.column].low_x_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_x_axis)
											   / 2))){
						adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
						res = RETURN_TRUE;
						state = 0;
					}
				}
				else if(MEEK_X_AXIS_HIGH_TO_LOW == imeek_work_state.direction){
					if((l_OidData->position.y > (((map_block_data[map][start_block.row][start_block.column].low_y_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_y_axis)
											   / 2) - 3))
					&&(l_OidData->position.y < (((map_block_data[map][start_block.row][start_block.column].low_y_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_y_axis)
											   / 2) + 3))
					&&(l_OidData->position.x < ((map_block_data[map][start_block.row][start_block.column].low_x_axis 
											   + map_block_data[map][start_block.row][start_block.column].high_x_axis)
											   / 2))){
						adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
						res = RETURN_TRUE;
						state = 0;
					}
				}
				adj_motor_state(1,MOTOR_BACKWARD,1,MOTOR_FORWARD);
				break;
		}
    }
	return res;
}

/*
*******************************************************************************
 *Function:     GetMapLocation
 *Description:  获取米小克在地图中的位置
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       0:在地图的开始位置
 *              1:不在地图开始位置
 *              2:不在本关地图上
 *              3:没读到码
 *Others:       
*******************************************************************************
*/      
uint8_t GetMapLocation(STRUCT_APP_OID_DATA *l_OidData,uint32_t map)
{
    //任务模式下寻找地图位置，必须是位置码
    if((TASK_MODE == imeek_work_state.state) 
	&& (POSITION_CODE == l_OidData->code_flag)){
		if((l_OidData->position.x > map_block_data[map][0][0].low_x_axis) 
		&& (l_OidData->position.x < map_block_data[map][5][7].high_x_axis)
		&& (l_OidData->position.y > map_block_data[map][5][0].low_y_axis) 
		&& (l_OidData->position.y < map_block_data[map][0][0].high_y_axis)){
		   //在地图上,且在开始位置
		   if((l_OidData->position.x > map_block_data[map][0][0].low_x_axis) 
		   && (l_OidData->position.x < map_block_data[map][0][0].high_x_axis)
		   && (l_OidData->position.y > map_block_data[map][0][0].low_y_axis) 
		   && (l_OidData->position.y < map_block_data[map][0][0].high_y_axis)){
		       return 0;
		   }
		   //在地图上，且不在开始位置
		   else{
		       return 1;
		   }
		}
		else{
            return 2;
		}
	}    

	if(FREE_MODE == imeek_work_state.state){
        imeek_work_state.current_mode_state = EXECUTE_INSTRUCTION_DIRECTION;
		return 0;
	}
	return 3;
}


uint8_t get_start_location(STRUCT_APP_OID_DATA *l_OidData,uint32_t map)
{   
    uint8_t res = RETURN_ERROR_1;
    //任务模式下寻找开始位置，必须是位置码
    if((TASK_MODE == imeek_work_state.state) && (POSITION_CODE == l_OidData->code_flag)){
		if((l_OidData->position.x > map_block_data[map][0][0].low_x_axis) 
		&& (l_OidData->position.x < map_block_data[map][0][0].high_x_axis)
		&& (l_OidData->position.y > map_block_data[map][0][0].low_y_axis) 
		&& (l_OidData->position.y < map_block_data[map][0][0].high_y_axis)){
		    return RETURN_TRUE;
		}
	}

	//自由模式下做两件事:确定当前开始位置，确定小车行进方向
	if((FREE_MODE == imeek_work_state.state) 
	&& (POSITION_CODE == l_OidData->code_flag)
	   //位置码为地图位置码，不是指令卡位置码
	&& (g_arr_instuction_card[0].low_x_axis > l_OidData->position.x)){
        res = RETURN_TRUE;
//	    res = get_start_direction(l_OidData,map);
	}
	return res;
}

//获取当前小车处于的模块在小车运动方向上的三等分值
uint32_t get_trisection_value(STRUCT_BLOCK *block, ENUM_MEEK_DIRECTION_AXIS direction, uint32_t map)
{
    uint32_t value = 0;
	if(MEEK_X_AXIS_LOW_TO_HIGH == direction){
		value = map_block_data[map][block->row][block->column].high_x_axis - 
				((map_block_data[map][block->row][block->column].high_x_axis - 
				 map_block_data[map][block->row][block->column].low_x_axis)/3);
	}
	else if(MEEK_X_AXIS_HIGH_TO_LOW == direction){
		value = map_block_data[map][block->row][block->column].low_x_axis + 
				((map_block_data[map][block->row][block->column].high_x_axis - 
				 map_block_data[map][block->row][block->column].low_x_axis)/3);
	}
	else if(MEEK_Y_AXIS_LOW_TO_HIGH == direction){
		value = map_block_data[map][block->row][block->column].high_y_axis - 
				((map_block_data[map][block->row][block->column].high_y_axis - 
				 map_block_data[map][block->row][block->column].low_y_axis)/3);
	}
	else if(MEEK_Y_AXIS_HIGH_TO_LOW == direction){
		value = map_block_data[map][block->row][block->column].low_y_axis + 
				((map_block_data[map][block->row][block->column].high_y_axis - 
				 map_block_data[map][block->row][block->column].low_y_axis)/3);
	}
	return value;
}

//判断小车当前是否运动于障碍物的模块中,如果是则找出要停止的点
uint8_t jdg_barrier(STRUCT_APP_OID_DATA *l_OidData,
                    uint32_t current_value,
                    ENUM_MEEK_DIRECTION_AXIS direction,
	                STRUCT_BLOCK *target_block,
	                uint32_t map)
{
	STRUCT_BLOCK current_block = {0,0};
	uint32_t error_border_value = 0;
	
	get_map_block(l_OidData,&current_block,map);
	imeek_work_state.current_row = current_block.row;
	imeek_work_state.current_column= current_block.column;

//	APP_MSG_DBG("current_block.row = %d\n",current_block.row);
//	APP_MSG_DBG("current_block.column = %d\n",current_block.column);
//	APP_MSG_DBG("target_block.row = %d\n",target_block->row);
//	APP_MSG_DBG("target_block.column = %d\n",target_block->column);
//	APP_MSG_DBG("map_block_data[current_block.row][current_block.column].barrier = %d\n",
//		        map_block_data[current_block.row][current_block.column].barrier);
	
    if(((target_block->row != current_block.row) || (target_block->column != current_block.column))
	&& (MAP_IS_BARRIER == map_block_data[map][current_block.row][current_block.column].barrier)){
        error_border_value = get_trisection_value(&current_block,direction,map);
    	APP_MSG_DBG("error_current_value = %d\n",error_border_value);
	
		if((MEEK_X_AXIS_LOW_TO_HIGH == direction) || (MEEK_Y_AXIS_LOW_TO_HIGH == direction)){
			if(error_border_value < current_value){
				return RETURN_ERROR_2;
			}
		}
		else if((MEEK_X_AXIS_HIGH_TO_LOW == direction) || (MEEK_Y_AXIS_HIGH_TO_LOW == direction)){
			if(error_border_value > current_value){
				return RETURN_ERROR_2;
			}
		}
	}
	return RETURN_TRUE;
}

uint8_t imeek_forward(STRUCT_APP_OID_DATA *l_OidData, ENUM_MEEK_DIRECTION_AXIS direction,
	                  uint32_t step_number, ENUM_RUNNING_STATE running_state,uint32_t map)
{
    //直走分为3个状态:
    //0:确认各数据值
    //1:直走
    //2:停止
    uint8_t return_value = 0;
    static uint32_t state = 0;
    static STRUCT_BLOCK start_block = {0,0};
    static STRUCT_BLOCK target_block = {0,0};
	static uint32_t pid_target = 0;
	static uint32_t stop_target_value = 0;
	uint32_t current_value = 0;
	
	uint32_t actual = 0;
	
	//在获取到OID数据才进行直行相关操作
	if(POSITION_CODE != l_OidData->code_flag){
//		printf("l_oid_data error\n");
		return RETURN_ERROR_1;
	}
	
	switch(state){
		case 0:
//			printf("imeek_forward get run data\n");
			//获取当前模块
			if(1 == imeek_work_state.start_flag){
				imeek_work_state.start_flag = 0;
			    start_block.column = imeek_work_state.start_column;
			    start_block.row = imeek_work_state.start_row;
			}
			else{
				return_value = get_map_block(l_OidData,&start_block,map);
				if(return_value){
					return return_value;
				}
			}
			
			//获取pid目标值与停止运动值(横坐标或纵坐标)
			if(MEEK_X_AXIS_LOW_TO_HIGH == direction){
				pid_target = (map_block_data[map][start_block.row][start_block.column].low_y_axis + map_block_data[map][start_block.row][start_block.column].high_y_axis) / 2;
				actual = l_OidData->position.y;
			
				target_block.column = start_block.column + step_number;
				target_block.row = start_block.row;
				current_value = l_OidData->position.x;
			}
			else if(MEEK_X_AXIS_HIGH_TO_LOW == direction){
				pid_target = (map_block_data[map][start_block.row][start_block.column].low_y_axis + map_block_data[map][start_block.row][start_block.column].high_y_axis) / 2;
				actual = l_OidData->position.y;
			
				target_block.column = start_block.column - step_number;
				target_block.row = start_block.row;
				current_value = l_OidData->position.x;

			}
			else if(MEEK_Y_AXIS_LOW_TO_HIGH == direction){
				pid_target = (map_block_data[map][start_block.row][start_block.column].low_x_axis + map_block_data[map][start_block.row][start_block.column].high_x_axis) / 2;
				actual = l_OidData->position.x;
			
				target_block.row= start_block.row - step_number;
				target_block.column = start_block.column;
				current_value = l_OidData->position.y;

			}
			else if(MEEK_Y_AXIS_HIGH_TO_LOW == direction){
				pid_target = (map_block_data[map][start_block.row][start_block.column].low_x_axis + map_block_data[map][start_block.row][start_block.column].high_x_axis) / 2;
				actual = l_OidData->position.x;
			
				target_block.row= start_block.row + step_number;
				target_block.column = start_block.column;
				current_value = l_OidData->position.y;

			}

//			printf("target_block.row = %d\n",   target_block.row);
//			printf("target_block.column = %d\n",target_block.column);
//			printf("start_block.row = %d\n",    start_block.row);
//			printf("start_block.column = %d\n", start_block.column);
//			printf("pid_target = %d\n",         target_block.row);
//			printf("target_block.column = %d\n",target_block.column);
//			printf("target_block.row = %d\n",target_block.row);
//			printf("target_block.column = %d\n",target_block.column);
			if((target_block.row >= MAP_ROW) 
			|| (target_block.column >= MAP_COLUMN)){
			    return RETURN_ERROR_3;
			}
			else if(MAP_NO == map_block_data[map][target_block.row][target_block.column].barrier){
			    return RETURN_ERROR_3;
			}
			else{
				stop_target_value = get_trisection_value(&target_block,direction,map);
			}
			state = 1;
			return RETURN_ERROR_1;
		case 1:			
			if(MEEK_X_AXIS_LOW_TO_HIGH == direction){
				actual = l_OidData->position.y;
				current_value = l_OidData->position.x;
				
		        if(stop_target_value > current_value){					
					if(pid_target > actual){
						adj_motor_state(7,MOTOR_FORWARD,4,MOTOR_FORWARD);
					}
					else if(pid_target < actual){
						adj_motor_state(4,MOTOR_FORWARD,7,MOTOR_FORWARD);
					}
					else if(pid_target == actual){
						adj_motor_state(6,MOTOR_FORWARD,6,MOTOR_FORWARD);
					}
				}
				else{
					adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
					state =0;
					return RETURN_TRUE;
				}
			}
			else if(MEEK_X_AXIS_HIGH_TO_LOW == direction){
				actual = l_OidData->position.y;
				current_value = l_OidData->position.x;
				
		        if(stop_target_value < current_value){
					if(pid_target > actual){
						adj_motor_state(4,MOTOR_FORWARD,7,MOTOR_FORWARD);
 					}
					else if(pid_target < actual){
						adj_motor_state(7,MOTOR_FORWARD,4,MOTOR_FORWARD);
 					}
					else if(pid_target == actual){
						adj_motor_state(6,MOTOR_FORWARD,6,MOTOR_FORWARD);
 					}
				}
				else{
					adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
 					state = 0;
					return RETURN_TRUE;
				}
			}
			else if(MEEK_Y_AXIS_LOW_TO_HIGH == direction){
				actual = l_OidData->position.x;
				current_value = l_OidData->position.y;

		        if(stop_target_value > current_value){
					if(pid_target > actual){
						adj_motor_state(4,MOTOR_FORWARD,7,MOTOR_FORWARD);
 					}
					else if(pid_target < actual){
						adj_motor_state(7,MOTOR_FORWARD,4,MOTOR_FORWARD);
 					}
					else if(pid_target == actual){
						adj_motor_state(6,MOTOR_FORWARD,6,MOTOR_FORWARD);
 					}
				}
				else{
					adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
 					state =0;
					return RETURN_TRUE;
				}
			}
			else if(MEEK_Y_AXIS_HIGH_TO_LOW == direction){
				actual = l_OidData->position.x;
				current_value = l_OidData->position.y;

				if(stop_target_value < current_value){
					if(pid_target > actual){
						adj_motor_state(7,MOTOR_FORWARD,4,MOTOR_FORWARD);
 					}
					else if(pid_target < actual){
						adj_motor_state(4,MOTOR_FORWARD,6,MOTOR_FORWARD);
 					}
					else if(pid_target == actual){
						adj_motor_state(6,MOTOR_FORWARD,6,MOTOR_FORWARD);
 					}
				}
				else{
					adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
 					state = 0;
					return RETURN_TRUE;
				}
			}

			if(TASK_MODE == running_state){
				return_value = jdg_barrier(l_OidData,
										   current_value,
										   direction,
										   &target_block,
										   map);
			}
			if(return_value){
		        //小车走到了不允许走的地方
		        APP_MSG_DBG("The car is on a forbidden road");
				adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
 				state =0;
				return RETURN_ERROR_2;
			}
			else{
				return RETURN_ERROR_1;
			}
			break;
	}
    return RETURN_TRUE;
}

void imeek_update_direction(uint32_t instruction, uint32_t parameter)
{
    if((INSTRUCTION_TURN_LEFT == instruction) && (PARAMETER_ANGLE_90 == parameter)){
		switch(imeek_work_state.direction){
			case MEEK_X_AXIS_LOW_TO_HIGH:
				imeek_work_state.direction = MEEK_Y_AXIS_HIGH_TO_LOW;
				break;
			case MEEK_X_AXIS_HIGH_TO_LOW:
				imeek_work_state.direction = MEEK_Y_AXIS_LOW_TO_HIGH;
				break;
			case MEEK_Y_AXIS_LOW_TO_HIGH:
				imeek_work_state.direction = MEEK_X_AXIS_LOW_TO_HIGH;
				break;
			case MEEK_Y_AXIS_HIGH_TO_LOW:
				imeek_work_state.direction = MEEK_X_AXIS_HIGH_TO_LOW;
				break;
		}
	}
	else if((INSTRUCTION_TURN_RIGHT == instruction) && (PARAMETER_ANGLE_90 == parameter)){
		switch(imeek_work_state.direction){
			case MEEK_X_AXIS_LOW_TO_HIGH:
				imeek_work_state.direction = MEEK_Y_AXIS_LOW_TO_HIGH;
				break;
			case MEEK_X_AXIS_HIGH_TO_LOW:
				imeek_work_state.direction = MEEK_Y_AXIS_HIGH_TO_LOW;
				break;
			case MEEK_Y_AXIS_LOW_TO_HIGH:
				imeek_work_state.direction = MEEK_X_AXIS_HIGH_TO_LOW;
				break;
			case MEEK_Y_AXIS_HIGH_TO_LOW:
				imeek_work_state.direction = MEEK_X_AXIS_LOW_TO_HIGH;
				break;
		}
	}
	else if(PARAMETER_ANGLE_180 == parameter){
		switch(imeek_work_state.direction){
			case MEEK_X_AXIS_LOW_TO_HIGH:
				imeek_work_state.direction = MEEK_X_AXIS_HIGH_TO_LOW;
				break;
			case MEEK_X_AXIS_HIGH_TO_LOW:
				imeek_work_state.direction = MEEK_X_AXIS_LOW_TO_HIGH;
				break;
			case MEEK_Y_AXIS_LOW_TO_HIGH:
				imeek_work_state.direction = MEEK_Y_AXIS_HIGH_TO_LOW;
				break;
			case MEEK_Y_AXIS_HIGH_TO_LOW:
				imeek_work_state.direction = MEEK_Y_AXIS_LOW_TO_HIGH;
				break;
		}
	}
}

uint8_t imeek_turn(STRUCT_APP_OID_DATA * l_OidData, ENUM_MEEK_DIRECTION_AXIS direction,
	               uint32_t instruction,           uint32_t parameter, uint32_t map)
{
	static STRUCT_BLOCK current_block = {0,0};
    static uint32_t state = 0;
	static uint32_t target_value_x = 0; 
	static uint32_t target_value_y = 0;
	uint32_t target_threshold = 5;

	switch(state){
		case 0:
//			APP_MSG_DBG("imeek_turn, state = 0\n");
            imeek_update_direction(instruction,parameter);
			get_map_block(l_OidData,&current_block,map);
			
			target_value_y = (map_block_data[map][current_block.row][current_block.column].low_y_axis 
						   + map_block_data[map][current_block.row][current_block.column].high_y_axis) / 2;
			
			target_value_x = (map_block_data[map][current_block.row][current_block.column].low_x_axis 
						   + map_block_data[map][current_block.row][current_block.column].high_x_axis) / 2;
			
			switch(instruction){
				case INSTRUCTION_TURN_LEFT:
					adj_motor_state(3,MOTOR_BACKWARD,3,MOTOR_FORWARD);
 					break;
				case INSTRUCTION_TURN_RIGHT:
					adj_motor_state(3,MOTOR_FORWARD,3,MOTOR_BACKWARD);
 					break;
			}
			state = 1;
			return RETURN_ERROR_1;
			break;
			
		case 1:
//			APP_MSG_DBG("target_value_y = %d\n",target_value_y);
//			APP_MSG_DBG("target_value_x = %d\n",target_value_x);
//			APP_MSG_DBG("l_OidData->position.x = %d\n",l_OidData->position.x);
//			APP_MSG_DBG("l_OidData->position.y = %d\n",l_OidData->position.y);
//			APP_MSG_DBG("direction = %d\n",direction);
			if(MEEK_X_AXIS_LOW_TO_HIGH == direction){
				if((l_OidData->position.y > (target_value_y - target_threshold))
			    && (l_OidData->position.y < (target_value_y + target_threshold))
			    && (l_OidData->position.x > target_value_x)){
					adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
 					state = 0;
					return RETURN_TRUE;
			    }
			}
			else if(MEEK_X_AXIS_HIGH_TO_LOW == direction){
				if((l_OidData->position.y > (target_value_y - target_threshold))
			    && (l_OidData->position.y < (target_value_y + target_threshold))
			    && (l_OidData->position.x < target_value_x)){
					adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
 					state = 0;
					return RETURN_TRUE;
			    } 
			}
			else if(MEEK_Y_AXIS_LOW_TO_HIGH == direction){
				if((l_OidData->position.x > (target_value_x - target_threshold))
			    && (l_OidData->position.x < (target_value_x + target_threshold))
			    && (l_OidData->position.y > target_value_y)){
					adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
					state = 0;
					return RETURN_TRUE;
			    }				
 			}
			else if(MEEK_Y_AXIS_HIGH_TO_LOW == direction){
				if((l_OidData->position.x > (target_value_x - target_threshold))
			    && (l_OidData->position.x < (target_value_x + target_threshold))
			    && (l_OidData->position.y < target_value_y)){
					adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
					state = 0;
					return RETURN_TRUE;
			    }				
 			}
			
			return RETURN_ERROR_1;
			break;
	}
	
	return RETURN_TRUE;
}

uint32_t imeek_play_audio(uint32_t parameter)
{
	char url_temp[100];

#if (OID_PLAY_MODE == FS_AUDIO_PLAY)
    char file[] = "file://music/";
#elif (OID_PLAY_MODE == HTTP_AUDIO_PLAY)
    char file[] = "http://api.linxbot.com/tmp/";
#endif
//    printf("parameter = %d\n",parameter);
//    printf("111111---imeek_play_audio-----1111111\n");
	sprintf(url_temp,"%s%s",file,voice_card_play_list[parameter - PARAMETER_VOICE_1]);
	PlayAudio(url_temp,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
	OS_Sleep(2);
	WaitAudioPlayOver(TRUE);
	return RETURN_TRUE;
}

uint32_t imeek_eyes_rgb(uint32_t parameter)
{
    switch(parameter){
		case PARAMETER_LIGHT_R:
			set_imeek_light_struct(LIGHT_OFF,LIGHT_RED,LIGHT_FORM,LIGHT_NO_CONTINUE,1,
								   LIGHT_ON ,LIGHT_RED,LIGHT_FORM,LIGHT_NO_CONTINUE,1);
			OS_Sleep(2);
			break;
		case PARAMETER_LIGHT_G:
			set_imeek_light_struct(LIGHT_OFF,LIGHT_RED  ,LIGHT_FORM,LIGHT_NO_CONTINUE,1,
								   LIGHT_ON ,LIGHT_GREEN,LIGHT_FORM,LIGHT_NO_CONTINUE,1);
			OS_Sleep(2);
			break;
		case PARAMETER_LIGHT_B:
			set_imeek_light_struct(LIGHT_OFF,LIGHT_RED ,LIGHT_FORM,LIGHT_NO_CONTINUE,1,
								   LIGHT_ON ,LIGHT_BLUE,LIGHT_FORM,LIGHT_NO_CONTINUE,1);
			OS_Sleep(2);
			break;
	}
//	printf("parameter = %d\n",parameter);
	return RETURN_TRUE;
}


uint8_t run_instruction(uint32_t instruction, uint32_t parameter,
	                    STRUCT_APP_OID_DATA *l_OidData)
{
    switch(instruction){
		case INSTRUCTION_FORWARD:
//			printf("instruction_forward = %d\n",parameter);
			return (imeek_forward(l_OidData,imeek_work_state.direction,
				                  parameter - PARAMETER_INIT,
				                  imeek_work_state.state,
				                  imeek_work_state.map));

        case INSTRUCTION_TURN_RIGHT:
		case INSTRUCTION_TURN_LEFT:
//			printf("instruction_turn_left = %d\n",parameter);
            return imeek_turn(l_OidData,imeek_work_state.direction,
                              instruction,parameter,imeek_work_state.map);
  			
		case INSTRUCTION_LOOP_LEFT:
//			printf("instruction_loop_left = %d\n",parameter);
			break;
			
		case INSTRUCTION_LOOP_RIGHT:
//			printf("instruction_loop_right = %d\n",parameter);
			break;
			
		case INSTRUCTION_VOICE:
//			printf("instruction_voice = %d\n",parameter);
			imeek_play_audio(parameter);
			break;
			
		case INSTRUCTION_LIGHT:
//			printf("instruction_light = %d\n",parameter);
			imeek_eyes_rgb(parameter);
			break;

		default:
			break;
	}

	return RETURN_TRUE;
}

uint8_t imeek_execute_instruction(uint32_t *instruction_data, uint32_t num, STRUCT_APP_OID_DATA *l_OidData)
{
    uint32_t i,j,k;
	uint32_t i_loop1 = 0,i_loop2 = 0;
	STRUCT_RUN_INSTRUCTION_MESSAGE message;
	EventBits_t uxBits;
	
    for(i=0; i < num; ){
		if(instruction_data[i] == code_instruction_card[3].instruction_card){
//			printf("instruction_data[i+1] = %d\n",instruction_data[i+1]);
			for(j=0; j<(instruction_data[i+1] - PARAMETER_INIT); j++){//第一个循环内运行的次数
			    i_loop1 = i + 2;
			    while(instruction_data[i_loop1] != code_instruction_card[4].instruction_card){
					if(instruction_data[i_loop1] == code_instruction_card[3].instruction_card){
						for(k=0; (k<instruction_data[i_loop1+1] - PARAMETER_INIT); k++){//第二个循环内运行的次数
							i_loop2 = i_loop1 + 2;
							while(instruction_data[i_loop2] != code_instruction_card[4].instruction_card){
                                message.instruction = instruction_data[i_loop2];
                                message.parameter = instruction_data[i_loop2+1];
                                xQueueSendToBack(run_instrutcion_Queue,(void *)&message, 5);
								uxBits = xEventGroupWaitBits(run_instruction_event_group,		\
  															 RUN_INSTRUCTION_CAR_FORBIDDEN |    \
  															 RUN_INSTRUCTION_SUCCESS 	  |     \
  															 RUN_INSTRUCTION_OVER_MAP,		    \
															 pdTRUE,							\
															 pdFALSE,							\
															 portMAX_DELAY);
								if(uxBits & RUN_INSTRUCTION_CAR_FORBIDDEN){
									APP_MSG_DBG("RUN_INSTRUCTION_CAR_FORBIDDEN\n");
									return RETURN_ERROR_2;
								}
								else if(uxBits & RUN_INSTRUCTION_OVER_MAP){
									APP_MSG_DBG("RUN_INSTRUCTION_OVER_MAP\n");
									return RETURN_ERROR_3;
								}
                                i_loop2 += 2;
							}
						}
						i_loop1 = i_loop2 + 1;
					}
					else{
                        message.instruction = instruction_data[i_loop1];
                        message.parameter = instruction_data[i_loop1+1];
                        xQueueSendToBack(run_instrutcion_Queue,(void *)&message, 5);
						uxBits = xEventGroupWaitBits(run_instruction_event_group,		\
													 RUN_INSTRUCTION_CAR_FORBIDDEN |	\
													 RUN_INSTRUCTION_SUCCESS       |    \
							                         RUN_INSTRUCTION_OVER_MAP,		    \
													 pdTRUE,							\
													 pdFALSE,							\
													 portMAX_DELAY);
						if(uxBits & RUN_INSTRUCTION_CAR_FORBIDDEN){
							APP_MSG_DBG("RUN_INSTRUCTION_CAR_FORBIDDEN\n");
							return RETURN_ERROR_2;
						}
						else if(uxBits & RUN_INSTRUCTION_OVER_MAP){
							APP_MSG_DBG("RUN_INSTRUCTION_OVER_MAP\n");
							return RETURN_ERROR_3;
						}
						i_loop1 += 2;
					}
				}
			}
			i = i_loop1 + 1;
		}
		else{
            message.instruction = instruction_data[i];
            message.parameter = instruction_data[i+1];
            xQueueSendToBack(run_instrutcion_Queue,(void *)&message, 5);
			uxBits = xEventGroupWaitBits(run_instruction_event_group,	    \
 										 RUN_INSTRUCTION_CAR_FORBIDDEN |    \
 										 RUN_INSTRUCTION_SUCCESS 	  |    \
 										 RUN_INSTRUCTION_OVER_MAP,		   \
										 pdTRUE,							\
										 pdFALSE,							\
										 portMAX_DELAY);
			if(uxBits & RUN_INSTRUCTION_CAR_FORBIDDEN){
				APP_MSG_DBG("RUN_INSTRUCTION_CAR_FORBIDDEN\n");
				return RETURN_ERROR_2;
			}
			else if(uxBits & RUN_INSTRUCTION_OVER_MAP){
				APP_MSG_DBG("RUN_INSTRUCTION_OVER_MAP\n");
				return RETURN_ERROR_3;
			}
			i += 2;
		}
	}
	APP_MSG_DBG("RUN_INSTRUCTION_CAR_TURE\n");
	return RETURN_TRUE;
}

void PlayInstructionCompleteAudioLight(uint8_t value,MYBOOL* write_flash)
{
    MYBOOL l_bWaitPlayOver = FALSE;
    char url_temp[100]="file://music/Paint/001/";
#if (OID_PLAY_MODE == FS_AUDIO_PLAY)
        char* l_u8RunStructPath = "file://music/Public/_47.mp3";
        char* l_u8RunOutPath = "file://music/Public/_48.mp3";
        char* l_u8RunErrorPath = "file://music/Public/_48.mp3";
        char* l_u8FreeOKPath = "file://music/Public/_10.mp3";
#elif (OID_PLAY_MODE == HTTP_AUDIO_PLAY)
		char file[] = "http://api.linxbot.com/tmp/";
#endif
    
	if(RETURN_ERROR_3 == value){
		//播放:再走就要离开地球啦，添加新的音频段
		PlayAudio(l_u8RunOutPath,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
		set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2,
							   LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
	}
	else if(RETURN_ERROR_2 == value){
		//播放:不能跨越建筑物
		PlayAudio(l_u8RunStructPath,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
		set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2,
							   LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
	}
	else if(RETURN_TRUE == value){
		//任务模式按指令卡走完
	    if(TASK_MODE == imeek_work_state.state){
//			printf("imeek_work_state.current_row = %d\n",imeek_work_state.current_row);
//			printf("imeek_work_state.current_column = %d\n",imeek_work_state.current_column);
//			printf("imeek_work_state.level_row = %d\n",imeek_work_state.level_row);
//			printf("imeek_work_state.level_column = %d\n",imeek_work_state.level_column);
			//米小克执行完所有指令卡，终点在目标位置
			if((imeek_work_state.current_row 
			== imeek_work_state.level_row)
			&&(imeek_work_state.current_column
			== imeek_work_state.level_column)){
				sprintf(url_temp,"%s%s%s",url_temp,map_level_block[imeek_work_state.map][imeek_work_state.level-1].finish_audio_path,"-117.mp3");
				PlayAudio(url_temp,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
				set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2,
									   LIGHT_ON,LIGHT_GREEN,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
								
				if(imeek_work_state.level == g_strFlashState.MissionLevel){
					g_strFlashState.MissionLevel++;
					*write_flash = TRUE;
				}
				//如果是每幅地图的最后一关且为第一次播放则要记录并不可打断音频
				if((MAP_2_9 == imeek_work_state.level) 
				&& !(g_strFlashState.BadgeState1 & (1<<1))){
				    g_strFlashState.BadgeState1 |= 1<<1;
					l_bWaitPlayOver = TRUE;
					*write_flash = TRUE;
                    WaitAudioPlayOver(l_bWaitPlayOver);
				}
			}
			//米小克执行完所有指令卡，终点不在目标位置
			else{
                PlayAudio(l_u8RunErrorPath,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
                set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2,
									   LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
			}
		}
		else if(FREE_MODE == imeek_work_state.state){
			//播放自由模式成功走完音频
			PlayAudio(l_u8FreeOKPath,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
			set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2,
								   LIGHT_ON,LIGHT_GREEN,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
		}
	}
}

void end_instruction_dance(void)
{
    adj_motor_state(7,MOTOR_FORWARD,7,MOTOR_BACKWARD);
	OS_Sleep(3);
}

/*
*******************************************************************************
 *Function:     NoNeedMap
 *Description:  检测是否到了执行卡片阶段，且卡片只有一个声音或者灯光
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:只有声音参数
 *              FALSE:没到卡片执行阶段，或卡片数量不仅仅只有声音或灯光参数
 *Others:       
*******************************************************************************
*/      
MYBOOL NoNeedMap(uint32_t *instruction_data,uint32_t num)
{

//    printf("imeek_work_state.current_mode_state = %d\n",imeek_work_state.current_mode_state);
//    printf("instruction_data[0] = %d\n",instruction_data[0]);
//    printf("num = %d\n",num);

    if((EXECUTE_INSTRUCTION_PREPARE == imeek_work_state.current_mode_state)
	&& (INSTRUCTION_VOICE == instruction_data[0])
	&& (2 == num)){
	    if(PARAMETER_VOICE_1 == instruction_data[1]){
			PlayAudio("file://music/Record/1.amr",THIRD_LEVEL_AUDIO,IMEEK_PLAY);
		}
		else if(PARAMETER_VOICE_2 == instruction_data[1]){
			PlayAudio("file://music/Record/2.amr",THIRD_LEVEL_AUDIO,IMEEK_PLAY);
		}
		imeek_work_state.current_mode_state = VARIABLE_INIT;
		return TRUE;
	}
	else if((EXECUTE_INSTRUCTION_PREPARE == imeek_work_state.current_mode_state)
	    &&  (INSTRUCTION_LIGHT == instruction_data[0])
	    &&  (2 == num)){
	    imeek_eyes_rgb(instruction_data[1]);
		imeek_work_state.current_mode_state = VARIABLE_INIT;
		return TRUE;
	}
	return FALSE;
}

/*
*******************************************************************************
 *Function:     ExecuteInstruction
 *Description:  执行指令卡片
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void ExecuteInstruction(uint32_t *instruction_data,    uint32_t num,
	                    STRUCT_APP_OID_DATA *l_OidData, uint32_t map,
	                    MYBOOL* write_flash)
{
    uint8_t return_value;
    uint8_t state;
	char url_temp[100];
	uint32_t rand;
	ENUM_INSTRUCTION_CARD_VARIETY l_enum_instruction_card;
#if (OID_PLAY_MODE == FS_AUDIO_PLAY)
    char file[] = "file://music/Public/";
    char* l_u8ErrorLocation = "file://music/Public/_49.mp3";
#elif (OID_PLAY_MODE == HTTP_AUDIO_PLAY)
    char file[] = "http://api.linxbot.com/tmp/";
#endif
	
	if(!NoNeedMap(instruction_data,num)){		
		switch(imeek_work_state.current_mode_state){
			case EXECUTE_INSTRUCTION_PREPARE:
				//如果在跑地图阶段又放到了开始指令卡片，则默认重新读取卡片
 				state = GetMapLocation(l_OidData,map);
				if(0 == state){
					imeek_work_state.current_mode_state = EXECUTE_INSTRUCTION_START;
				}
				else if(1 == state){
					PlayAudio(l_u8ErrorLocation,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
					imeek_work_state.current_mode_state = EXECUTE_INSTRUCTION_START;
				}
				break;
			case EXECUTE_INSTRUCTION_START:
//				printf("execute_instruction_start\n");
				GetInstructionCard(l_OidData,&l_enum_instruction_card);
				if(INSTRUCTION_START == l_enum_instruction_card){
                    imeek_work_state.current_mode_state = GET_INSTRUCTION_START;
					break;
				}
				
				if(RETURN_TRUE == get_start_location(l_OidData,map)){
					imeek_work_state.current_mode_state = EXECUTE_INSTRUCTION_DIRECTION;				
					rand =	OS_GetTicks() % GO_FORWARD_PLAY_LIST_NUM;
					sprintf(url_temp,"%s%s%s",file,g_au8GoForwardPlayList[rand],".mp3");
					PlayAudio(url_temp,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
					set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_NO_CONTINUE,2,
										   LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_NO_CONTINUE,2);
				}
				break;
				
			case EXECUTE_INSTRUCTION_DIRECTION:
				if(POSITION_CODE == l_OidData->code_flag){
					if(RETURN_TRUE == get_start_direction(l_OidData,map)){
						adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
						imeek_work_state.current_mode_state= EXECUTE_INSTRUCTION_EXECUTE;
					}
				}
				break;
			case EXECUTE_INSTRUCTION_EXECUTE:
//				printf("execute_instruction_execute\n");
				//阻塞，直到完成指令卡
				return_value = imeek_execute_instruction(instruction_data,num,l_OidData);
				PlayInstructionCompleteAudioLight(return_value,write_flash);
				imeek_work_state.current_mode_state = EXECUTE_INSTRUCTION_END;
				break;
				
			case EXECUTE_INSTRUCTION_END:
//			    printf("execute_instruction_end\n");
				end_instruction_dance();
				imeek_work_state.current_mode_state = VARIABLE_INIT;
				break;
				
			default:
				break;
		}	
	}
}

/*
*******************************************************************************
 *Function:     GetTrailing
 *Description:  获取三个寻迹的状态值
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
uint8_t GetTrailing(void)
{
    uint8_t state = 0x00;
	state =  (LEFT_IR_LED_READ()  << 0)
		   | (FRONT_IR_LED_READ() << 1)
		   | (RIGHT_IR_LED_READ() << 2);
	return state;
}

/*
*******************************************************************************
 *Function:     ImeekTrailingInit
 *Description:  刚切换到寻迹模式，做一些相关内容
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void ImeekTrailingInit(void)
{
    char* url = "file://music/Public/_40.mp3";
    if(TRAILING_INIT == g_strImeekTrailing.state){
		g_strImeekTrailing.state = TRAILING;
		
		IR_ENABLE();
        PlayAudio(url,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
	}
}


/*
*******************************************************************************
 *Function:     imeek_trailing
 *Description:  循迹模式
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void imeek_trailing(void)
{
    static uint8_t last_state = 0;
    uint8_t state;

	if(TRAILING == g_strImeekTrailing.state){
		state =  GetTrailing();
		switch(state){
			/*没有红外检测到黑*/
			case 0:
				set_imeek_light_struct(LIGHT_OFF,LIGHT_RED,LIGHT_FORM,LIGHT_CONTINUE,2,
									   LIGHT_ON,LIGHT_RED,LIGHT_FORM,LIGHT_CONTINUE,2);
				if(0 == last_state){
				    adj_motor_state(0,MOTOR_FORWARD,4,MOTOR_FORWARD);
				}
				break;
		
			/*左红外检测到黑*/
			case 1:
				last_state = state;
				set_imeek_light_struct(LIGHT_OFF,LIGHT_BLUE,LIGHT_FORM,LIGHT_CONTINUE,2,
									   LIGHT_ON,LIGHT_BLUE,LIGHT_FORM,LIGHT_CONTINUE,2);
				adj_motor_state(0,MOTOR_FORWARD,4,MOTOR_FORWARD);
				break;
		
			/*前红外检测到黑*/
			case 2:
				last_state = state;
				set_imeek_light_struct(LIGHT_OFF,LIGHT_GREEN,LIGHT_FORM,LIGHT_CONTINUE,2,
									   LIGHT_ON,LIGHT_GREEN,LIGHT_FORM,LIGHT_CONTINUE,2);
				adj_motor_state(4,MOTOR_FORWARD,4,MOTOR_FORWARD);
				break;
		
			/*左前红外检测到黑*/
			case 3:
				last_state = state;
				set_imeek_light_struct(LIGHT_OFF,LIGHT_PINKISH,LIGHT_FORM,LIGHT_CONTINUE,2,
									   LIGHT_ON,LIGHT_PINKISH,LIGHT_FORM,LIGHT_CONTINUE,2);
				adj_motor_state(4,MOTOR_FORWARD,7,MOTOR_FORWARD);
				break;
		
			/*右红外检测到黑*/
			case 4:
				last_state = state;
				set_imeek_light_struct(LIGHT_OFF,LIGHT_CYAN,LIGHT_FORM,LIGHT_CONTINUE,2,
									   LIGHT_ON,LIGHT_CYAN,LIGHT_FORM,LIGHT_CONTINUE,2);
				adj_motor_state(4,MOTOR_FORWARD,0,MOTOR_FORWARD);
				break;
		
			/*左右红外检测到黑*/
			case 5:
				last_state = state;
				set_imeek_light_struct(LIGHT_OFF,LIGHT_YELLOW,LIGHT_FORM,LIGHT_CONTINUE,2,
									   LIGHT_ON,LIGHT_YELLOW,LIGHT_FORM,LIGHT_CONTINUE,2);
				adj_motor_state(4,MOTOR_FORWARD,4,MOTOR_FORWARD);
				break;
		
			/*右前红外检测到黑*/
			case 6:
				last_state = state;
				set_imeek_light_struct(LIGHT_OFF,LIGHT_WHITE,LIGHT_FORM,LIGHT_CONTINUE,2,
									   LIGHT_ON,LIGHT_WHITE,LIGHT_FORM,LIGHT_CONTINUE,2);
				adj_motor_state(7,MOTOR_FORWARD,4,MOTOR_FORWARD);
				break;
		
			/*全检测到黑*/
			case 7:
				last_state = state;
				set_imeek_light_struct(LIGHT_OFF,LIGHT_BLUE,LIGHT_BRIGHT,LIGHT_CONTINUE,2,
									   LIGHT_OFF,LIGHT_BLUE,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
				adj_motor_state(4,MOTOR_FORWARD,4,MOTOR_FORWARD);
				break;
			default:
				break;
		}
	}
}

/*
*******************************************************************************
 *Function:     ImeekTrailingOver
 *Description:  退出寻迹模式
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void ImeekTrailingOver(void)
{
    char* url = "file://music/Public/_41.mp3";
	if(TRAILING_PLAY_OVER == g_strImeekTrailing.state){
		IR_DISABLE();
		adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
		
        PlayAudio(url,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
		g_strImeekTrailing.state = TRAILING_OVER;
		imeek_work_state.state = FREE_MODE;
		imeek_work_state.current_mode_state = VARIABLE_INIT;
	}
}

void light_task(void *pvParameters)
{
    uint8_t light_ear_state = 0;
    uint8_t light_eyes_state = 0;
	
    //耳朵灯初始化
	led_ear_gpio_init();
	//眼睛灯初始化
    rgb_gpio_init();
    StartUpLight(g_strFlashState.EyesState);

	while(1){
		//耳朵灯光控制
		if(LIGHT_ON == imeek_light.light_ear_state){
		    if(imeek_light.light_ear_time_of_duration){
				imeek_light.light_ear_time_of_duration--;
				if(LIGHT_FORM == imeek_light.light_ear_mode){
					LED_EAR_ON();
				}
				else{
					if(light_ear_state){
						light_ear_state = 0;
						LED_EAR_ON();				
					}
					else{
						light_ear_state = 1;
						LED_EAR_OFF();									
					}
				}
			}
			else{
				if(LIGHT_CONTINUE == imeek_light.light_ear_end){
					imeek_light.light_ear_mode = LIGHT_FORM;
					imeek_light.light_ear_time_of_duration = LIGHT_FOREVER;
				}
				else{
					imeek_light.light_ear_state = LIGHT_OFF;
					LED_EAR_OFF();
				}
			}
		}
		else{
		    LED_EAR_OFF();
		}

		//眼睛灯光控制
		if(LIGHT_ON == imeek_light.light_eyes_state){
			if(imeek_light.light_eyes_time_of_duration){
				imeek_light.light_eyes_time_of_duration--;
				if(LIGHT_FORM == imeek_light.light_eyes_mode){
					switch(imeek_light.light_eyes_color){
						case LIGHT_RED:
							LED_R_ON();
							break;
						case LIGHT_GREEN:
							LED_G_ON();
							break;
						case LIGHT_BLUE:
							LED_B_ON();
							break;
						case LIGHT_PINKISH:
							LED_PINKISH_ON();
							break;
						case LIGHT_YELLOW:
							LED_YELLOW_ON();
							break;
						case LIGHT_CYAN:
							LED_CYAN_ON();
							break;
						case LIGHT_WHITE:
							LED_WHITE_ON();
							break;
					}
				}
				else{
					if(light_eyes_state){
						light_eyes_state = 0;
						switch(imeek_light.light_eyes_color){
							case LIGHT_RED:
								LED_R_ON();
								break;
							case LIGHT_GREEN:
								LED_G_ON();
								break;
							case LIGHT_BLUE:
								LED_B_ON();
								break;
							case LIGHT_PINKISH:
								LED_PINKISH_ON();
								break;
							case LIGHT_YELLOW:
								LED_YELLOW_ON();
								break;
							case LIGHT_CYAN:
								LED_CYAN_ON();
								break;
							case LIGHT_WHITE:
								LED_WHITE_ON();
								break;
						}
					}
					else{
						light_eyes_state = 1;
						LED_EYES_ALL_OFF();									
					}
				}
			}
			else{
				if(LIGHT_CONTINUE == imeek_light.light_eyes_end){
					imeek_light.light_eyes_mode = LIGHT_FORM;
					imeek_light.light_eyes_time_of_duration = LIGHT_FOREVER;
				}
				else{
					imeek_light.light_eyes_state = LIGHT_OFF;
					LED_EYES_ALL_OFF();
				}
			}
		}
		else{
			LED_EYES_ALL_OFF();
		}

		OS_MSleep(500);
	}
}

/*
*******************************************************************************
 *Function:     LongIdleGotoStandby
 *Description:  空闲时间长，进入待机模式，网络可以正常通信但是其他电源会被关闭
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:      
*******************************************************************************
*/
static void LongIdleGotoStandby(void)
{
	HAL_Wakeup_SetTimer_Sec(15000);

	pm_enter_mode(PM_MODE_STANDBY);
}


uint8_t JdgTriggerAction(uint32_t time)
{
    uint8_t num = OS_Rand32() % 100;
//	printf("JdgTriggerAction num = %d\n",num);
    if((time >= 10) && (time <60) && (num < 15)){
		return TRUE;
	}
    else if((time >= 60) && (time <300) && (num < 30)){
		return TRUE;
	}
    else if((time >= 300) && (time <1200) && (num < 50)){
		return TRUE;
	}
    else if((time >= 1200) && (num < 80)){
		return TRUE;
	}
	return FALSE;
}

void ExecuteAction(void)
{
    char url_temp[100];
	char file[] = "file://music/Public/";
	ENUM_IMEEK_STATE l_enumTriggerAction = NO_STATE; 
    uint8_t num = OS_Rand32() % 100;
	
//	printf("ExecuteAction num = %d\n",num);
	
	if(num <= 16){
		l_enumTriggerAction = LEISURE;
	}
	else if((num > 16) && (num <= 36)){
		l_enumTriggerAction = FART;
	}
	else if((num > 36) && (num <= 72)){
		num = OS_Rand32() % 7;
		switch(num){
			case 0:
				l_enumTriggerAction = SLEEP;
				break;
			case 1:
				l_enumTriggerAction = NAP;
				break;
			case 2:
				l_enumTriggerAction = BRUSH_TEETH;
				break;
			case 3:
				l_enumTriggerAction = HAVE_A_MEAL;
				break;
			case 4:
				l_enumTriggerAction = AMAZE;
				break;
			case 5:
				l_enumTriggerAction = HISS;
				break;
			case 6:
				l_enumTriggerAction = DOUBT;
				break;
			default:
				break;
		}
	}
	else{
		l_enumTriggerAction = READ_BOOK;
	}

//	printf("l_enumTriggerAction =%d\n",l_enumTriggerAction);
	switch(l_enumTriggerAction){
		case NO_STATE:   //无状态
			break;
		case LEISURE:    //悠闲
			sprintf(url_temp,"%s%s%s",file,IdleModeAudio[LEISURE - 1],".mp3");
			ImeekIdleAction(3);
			break;
		case FART:	     //放屁
			sprintf(url_temp,"%s%s%s",file,IdleModeAudio[FART - 1],".mp3");
			break;
		case SLEEP:      //困了
			sprintf(url_temp,"%s%s%s",file,IdleModeAudio[SLEEP - 1],".mp3");
		    LED_B_OFF();
			OS_MSleep(500);
			LED_B_ON();
			break;
		case NAP:	     //打盹
			sprintf(url_temp,"%s%s%s",file,IdleModeAudio[NAP - 1],".mp3");
			break;
		case BRUSH_TEETH://刷牙
			sprintf(url_temp,"%s%s%s",file,IdleModeAudio[BRUSH_TEETH - 1],".mp3");
			break;
		case HAVE_A_MEAL://吃饭
			sprintf(url_temp,"%s%s%s",file,IdleModeAudio[HAVE_A_MEAL - 1],".mp3");
			break;
		case AMAZE: 	 //惊讶
			sprintf(url_temp,"%s%s%s",file,IdleModeAudio[AMAZE - 1],".mp3");
			break;			
		case HISS:		 //嘘
			sprintf(url_temp,"%s%s%s",file,IdleModeAudio[HISS - 1],".mp3");
			break;
		case DOUBT: 	 //疑惑
			sprintf(url_temp,"%s%s%s",file,IdleModeAudio[DOUBT - 1],".mp3");
			break;
		case READ_BOOK:  //看书
			sprintf(url_temp,"%s%s%s",file,IdleModeAudio[READ_BOOK - 1],".mp3");
			break;
		default:
			break;		
	}
	if(l_enumTriggerAction){
		PlayAudio(url_temp,FIFTH_LEVEL_AUDIO,IMEEK_PLAY);
	}
}

void IdleTask(void *pvParameters)
{
#define TIME_TO_IDLE  300
#define TIME_TO_SLEEP 600
	uint32_t i;
             //最近一次开始空闲时间戳
    uint32_t l_u32LastTickToSecs = OS_TicksToSecs(OS_GetTicks());
			 //空闲的时间
	uint32_t l_u32IdleTime = 0;	    
		     //上一次触发动作时间
	uint32_t l_u32LastActionTime = 0;
			 //未触发过动作的时间
	uint32_t l_u32NoActionTime = 0;
			 //是否触发动作
	uint8_t	 l_u32TriggerActionFlag = FALSE; 

    while(1){
		if(TRUE == g_u8IdleTimeStart){
			//获取空闲时间
			l_u32IdleTime = OS_TicksToSecs(OS_GetTicks()) - l_u32LastTickToSecs;
//			printf("idle time = %d\n",l_u32IdleTime);
			//空闲时间大于5分钟进入空闲模式
			if((l_u32IdleTime > TIME_TO_IDLE) && (l_u32IdleTime < TIME_TO_SLEEP)){
//				printf("into idle mode\n");
				l_u32NoActionTime = OS_TicksToSecs(OS_GetTicks()) - l_u32LastActionTime;
//				printf("l_u32NoActionTime = %d\n", l_u32NoActionTime);
				l_u32TriggerActionFlag = JdgTriggerAction(l_u32NoActionTime);
//				printf("l_u32TriggerActionFlag = %d\n", l_u32TriggerActionFlag);
				//需要触发动作
				if(l_u32TriggerActionFlag){
					l_u32LastActionTime = OS_TicksToSecs(OS_GetTicks());
//					printf("l_u32LastActionTime = %d\n", l_u32LastActionTime);
					ExecuteAction();
				}
			}
			//空闲时间大于10分钟，如果没有在更新音频，则进入睡眠模式
			else if(l_u32IdleTime >= TIME_TO_SLEEP){
				for(i=0; (i<AUDIO_TOTAL) && (g_aAudioUpdateData[i].Flag == FALSE); i++);
				if(i == AUDIO_TOTAL){
					set_imeek_light_struct(LIGHT_OFF,LIGHT_RED,LIGHT_FORM,LIGHT_NO_CONTINUE,LIGHT_FOREVER,
										   LIGHT_OFF,LIGHT_RED,LIGHT_FORM,LIGHT_NO_CONTINUE,LIGHT_FOREVER);
					OS_Sleep(2);
					LongIdleGotoStandby();
				}
			}
		}
		else{
			l_u32IdleTime = 0;
			l_u32LastTickToSecs = OS_TicksToSecs(OS_GetTicks());
		}
		OS_Sleep(5);
	}
}

///*
//*******************************************************************************
// *Function:     ComposeModeInit
// *Description:  音乐编辑模式变量初始化
// *Calls:       
// *Called By:   
// *Input:        
// *Output:       
// *Return:       
// *Others:       
//*******************************************************************************
//*/      
//void ComposeModeInit(void)
//{
//    if(COMPOSE_INIT == g_strImeekCompose.state){
//		printf("2222222222222222222\n");
//		g_strImeekCompose.state = COMPOSE_READ;
//		g_strImeekCompose.index = -1;
//	}
//}

///*
//*******************************************************************************
// *Function:     ComposeReadNote
// *Description:  音乐编辑模式读取音符
// *Calls:       
// *Called By:   
// *Input:        
// *Output:       
// *Return:       
// *Others:       
//*******************************************************************************
//*/      
//void ComposeReadNote(STRUCT_APP_OID_DATA* l_oid_data)
//{
//    int i;
//    if((l_oid_data->code_flag == POSITION_CODE)
//	&& (COMPOSE_READ == g_strImeekCompose.state)){
//		printf("3333333333333333333333\n");
//        for(i=0;i<NOTE_NUM;i++){
//            if((l_oid_data->position.x >= NoteCoordinate[i].low_x_axis)
//			&&(l_oid_data->position.x  <= NoteCoordinate[i].high_x_axis)
//			&&(l_oid_data->position.y  >= NoteCoordinate[i].low_y_axis)
//			&&(l_oid_data->position.y  <= NoteCoordinate[i].high_y_axis)){
//			    if(g_strImeekCompose.index  <  COMPOSE_MAX_NUM-1){
//					g_strImeekCompose.first[++g_strImeekCompose.index] = i+1;
//				}
//				break;
//			}
//		}
//		printf("ImeekCompose.index = %d\n",g_strImeekCompose.index);

//		if((l_oid_data->position.x >= g_strComposePlay.low_x_axis)
//		&&(l_oid_data->position.x <= g_strComposePlay.high_x_axis)
//		&&(l_oid_data->position.y >= g_strComposePlay.low_y_axis)
//		&&(l_oid_data->position.y <= g_strComposePlay.high_y_axis)){
//		    printf("44444444444444444444\n");
//            g_strImeekCompose.state = COMPOSE_PLAY;
//		}
//	}
//}

///*
//*******************************************************************************
// *Function:     ComposePlayNote
// *Description:  播放音频
// *Calls:       
// *Called By:   
// *Input:        
// *Output:       
// *Return:       
// *Others:       
//*******************************************************************************
//*/      
//void ComposePlayNote(STRUCT_APP_OID_DATA* l_oid_data)
//{
//    int i;
//    char url[50];
//	char file[] = "file://music/Paint/001/P01-10";
//    if(COMPOSE_PLAY == g_strImeekCompose.state){
//		printf("ImeekCompose.index = %d\n",g_strImeekCompose.index);
//		for(i=0; i<=g_strImeekCompose.index; i++){
//            printf("ImeekCompose.first[%d] = %d\n",i,g_strImeekCompose.first[i]);
//		}
//		if(g_strImeekCompose.index >= 0){
//			for(i=0;i<=g_strImeekCompose.index;i++){
//				sprintf(url,"%s%d%s",file,g_strImeekCompose.first[i]+2,".mp3");
//				printf("url = %s\n",url);
//				PlayAudio(url,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
//				OS_MSleep(450);
//			}
//		    g_strImeekCompose.state = COMPOSE_INIT;
//		}

//		//在播放过程中，又点击了新的音符，重新进入收集音符阶段
//        if((l_oid_data->position.x >= NoteCoordinate[0].low_x_axis)
//		&&(l_oid_data->position.x  <= NoteCoordinate[7].high_x_axis)
//		&&(l_oid_data->position.y  >= NoteCoordinate[0].low_y_axis)
//		&&(l_oid_data->position.y  <= NoteCoordinate[7].high_y_axis)){
//		    g_strImeekCompose.state = COMPOSE_INIT;
//		}
//	}
//}

/*
*******************************************************************************
 *Function:     GetRecordName
 *Description:  获取录音名
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
int GetRecordName(STRUCT_APP_OID_DATA* l_oid_data)
{
    if(POSITION_CODE == l_oid_data->code_flag){
		if((l_oid_data->position.x > 135) && (l_oid_data->position.x < 199)
		&&(l_oid_data->position.y > 10077) && (l_oid_data->position.y < 10124)){
			return 1;
		}
		else if((l_oid_data->position.x > 200) && (l_oid_data->position.x < 268)
			  &&(l_oid_data->position.y > 10077) && (l_oid_data->position.y < 10124)){
			return 2;
		}
	}
//	printf("Record Path ERROR\n");
    return 1;
}

///*
//*******************************************************************************
// *Function:     RecordModeInit
// *Description:  刚进入录音模式，初始化需要用到变量
// *Calls:       
// *Called By:   
// *Input:        
// *Output:       
// *Return:       
// *Others:       
//*******************************************************************************
//*/      
//void RecordModeInit(void)
//{
//    if(RECORD_INIT == g_strImeekRecord.state){
//        g_strImeekRecord.state = RECORDING;
//	}
//}

/*
*******************************************************************************
 *Function:     Recording
 *Description:  若处于录音模式，开始录音
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void Recording(STRUCT_APP_OID_DATA* l_oid_data)
{
	char *l_u8Path = "file://music/Paint/001/P02-108.mp3";
	
	adj_motor_state(0,MOTOR_FORWARD,0,MOTOR_FORWARD);
	OS_MSleep(1000);
	ImeekRecord.Index = GetRecordName(l_oid_data);
	WaitAudioPlayOver(TRUE);
	ImeekRecord.isRecord = TRUE;
	while(TRUE == ImeekRecord.isRecord){
		OS_MSleep(100);
	}
	imeek_work_state.state = FREE_MODE;
	imeek_work_state.current_mode_state = VARIABLE_INIT;
	PlayAudio(l_u8Path,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
	WaitAudioPlayOver(TRUE);
}

///*
//*******************************************************************************
// *Function:     ComposePlay
// *Description:  音乐编辑模式播放
// *Calls:       
// *Called By:   
// *Input:        
// *Output:       
// *Return:       
// *Others:       
//*******************************************************************************
//*/      
//void ComposePlay(STRUCT_APP_OID_DATA* l_oid_data)
//{
//    int i;
//	char url[50] = "file://music/Compose/";
//    if(COMPOSE_PLAY == g_strImeekCompose.state){
//        for(i=0; i<=g_strImeekCompose.index; i++){
//			sprintf(url,"%s%d%s",url,g_strImeekCompose.first[i],".mp3");
//		}
//	}

//}

/*
*******************************************************************************
 *Function:     ExtractMsgFromOidData
 *Description:  从OID数据中提取以下信息：
 *              1、米小克应该处于什么模式
 *              2、哪张地图
 *              3、哪个关卡
 *              4、点击的是绘本，数学问题页，徽章榜还是贴纸
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void ExtractMsgFromOidData(STRUCT_APP_OID_DATA *l_debounce_data,
                           ENUM_OID_POINT_TO *oid_point_to)
{
	//哪本绘本或地图
	int paint_num;

	if(TRAILING_MODE != imeek_work_state.state){
		if(POSITION_CODE == l_debounce_data->code_flag){
			if(isPaintOid(l_debounce_data,&paint_num)){
				*oid_point_to = OID_POINT_TO_PAINT;
				if(isTaskMode(l_debounce_data)){
					imeek_work_state.state = TASK_MODE;
					GetMap(l_debounce_data,&imeek_work_state.map);
					GetLevel(l_debounce_data,&imeek_work_state.level);
					GetLevelBlock(imeek_work_state.level,imeek_work_state.map);
				}
				else if(isFreeMode(l_debounce_data)){
					imeek_work_state.state = FREE_MODE;
				}
//				else if(isMusicEditMode(l_debounce_data)){
//					imeek_work_state.state = MUSIC_EDIT_MODE;
//				}
				else if(isRecordMode(l_debounce_data)){
					imeek_work_state.state = RECORD_MODE;
				}
			}
			else if(isMathOid(l_debounce_data,&paint_num)){
				*oid_point_to = OID_POINT_TO_MATH;
			}
			else if(isBadgeOid(l_debounce_data)){
				*oid_point_to = OID_POINT_TO_BADGE;
			}
			else if(isStickerOid(l_debounce_data)){
				*oid_point_to = OID_POINT_TO_STICKER;
			}
			else if(isEnglishLetterOID(l_debounce_data)){
				imeek_work_state.state = SPELL_MODE;
			}
			else if(isEnglishWordOID(l_debounce_data)){
				*oid_point_to = OID_POINT_TO_ENGLISH_WORD;
			}
			else if(isChineseWordOID(l_debounce_data)){
				*oid_point_to = OID_POINT_TO_CHINESE_WORD;
			}
			else if(isDownLoadAudioOID(l_debounce_data)){
				*oid_point_to = OID_POINT_TO_DOWNLOAD_AUDIO;
			}
			else{
				*oid_point_to = OID_POINT_TO_NULL;
			}
		}
		else if(GENERAL_CODE == l_debounce_data->code_flag){
			switch(l_debounce_data->index){
				case 202:
				case 203:
				case 204:
				case 205:
				case 206:
				case 207:
				case 218:
				case 219:
					imeek_work_state.state = TEST_MODE;
					break;
				default:
					break;
			}
		}
	}
	else{
		oid_point_to = OID_POINT_TO_NULL;
	}
}

/*
*******************************************************************************
 *Function:     JudgeMode
 *Description:  判断各模块是否OK
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void JudgeMode(STRUCT_APP_OID_DATA* l_oid_data)
{
    char path[30];
	char *file = "file://music/FF";
    char *l_u8Path1  = "01.mp3";
    char *l_u8Path2  = "02.mp3";
    char *l_u8Path3  = "03.mp3";
    char *l_u8Path4  = "04.mp3";
	char *l_u8Path5  = "05.mp3";  //ID还未发送至服务器
	char *l_u8Path6  = "06.mp3";  //ID已发送至服务器
	char *l_u8Path7  = "07.mp3";  //米小克还未写ID
	char *l_u8Path8  = "08.mp3";  //米小克ID为
//	char *l_u8Path9  = "09.mp3";  //米小克ID为
	char *l_u8Path20  = "20.mp3"; //米小克ID已清除
	char *l_u8Path21  = "file://music/Public/_51.mp3"; //米小克用户数据已清除
	char *l_u8Path22  = "22.mp3"; //触摸OK
//	char *l_u8Path23  = "23.mp3"; //米小克WIFI已关闭
	char *l_u8Path24  = "24.mp3"; //测试通过
	char *l_u8Path25  = "25.mp3"; //测试失败
	char *l_u8Path26  = "26.mp3"; //测试网络信号
	char *l_u8Path27  = "27.mp3"; //软件版本号为
	char *l_u8Path28  = "28.mp3"; //软件版本号降为1
	
    uint8_t state = 0;
    int  i;
    int signal_strong = 0;
	STRUCT_IMEEK_ID ImeekId;
	char buff[20];
	uint8_t l_u8CodeTemp[8];
	uint32_t l_32CodeTemp;
	wlan_ext_signal_t signal;

//	printf("l_oid_data->index = %d\n",l_oid_data->index); 
	if(202 == l_oid_data->index){
		//测试网络信号
		WaitAudioPlayOver(TRUE);
		sprintf(path,"%s%s",file,l_u8Path26);
		PlayAudio(path,ZERO_LEVEL_AUDIO,IMEEK_PLAY);
		WaitAudioPlayOver(TRUE);
		while(FALSE == imeek_network_event.flag){
            OS_MSleep(500);
		}
		
        //等待网络连接成功或失败后，修改WIFI账号与密码
		ssid_pwd.flag = SSID_PWD_VALID;
		sprintf((char *)ssid_pwd.ssid,"%s","imeektest");
		sprintf((char *)ssid_pwd.pwd,"%s","12345678");
//		sprintf(path,"%s%s",file,l_u8Path9);
 		taskENTER_CRITICAL();
		flash_ssid_pwd_write(&ssid_pwd);
		taskEXIT_CRITICAL();
//		PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
//		WaitAudioPlayOver(TRUE);

		//重新连接网络
		wlan_sta_disable();
		OS_MSleep(500);
		net_switch_mode(WLAN_MODE_STA);
		wlan_sta_set((uint8_t *)ssid_pwd.ssid, 
			         strlen((const char *)ssid_pwd.ssid),
			         (uint8_t *)ssid_pwd.pwd);
		wlan_sta_enable();

		OS_Sleep(20);

		//测试信号强度
 	    for(i = 0; i<20; i++){
			wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_SIGNAL, (int)(&signal));
 			signal_strong  += (int)(signal.noise + (signal.rssi/2));
            OS_MSleep(200);
		}
		signal_strong /= 20;
		signal_strong = ABS(signal_strong);
		wlan_sta_disconnect();

        while(1){
			if((signal_strong < 70) && (signal_strong >= 30)){
				sprintf(path,"%s%s",file,l_u8Path24);
			}
			else{
				sprintf(path,"%s%s",file,l_u8Path25);
			}
			PlayAudio(path,ZERO_LEVEL_AUDIO,IMEEK_PLAY);
			WaitAudioPlayOver(TRUE);
			
			sprintf(path,"%s%d%s","file://music/FF1",signal_strong/10,".mp3");
			PlayAudio(path,ZERO_LEVEL_AUDIO,IMEEK_PLAY);
			WaitAudioPlayOver(TRUE);
			
			sprintf(path,"%s%d%s","file://music/FF1",signal_strong%10,".mp3");
			PlayAudio(path,ZERO_LEVEL_AUDIO,IMEEK_PLAY);
			WaitAudioPlayOver(TRUE);
 		}
 	}
	else if(203 == l_oid_data->index){
		//清除用户数据
		g_strFlashState.MissionLevel = 1;
		g_strFlashState.MathLevel = 1;
		g_strFlashState.VolumeLevel = 4;
		g_strFlashState.EyesState = 1;
		g_strFlashState.BadgeState1 = 0;
		g_strFlashState.BadgeState2 = 0;
		g_strFlashState.TaskProgess1 = 0;
		g_strFlashState.TaskProgess2 = 0;
		g_strFlashState.TaskProgess3 = 0;
		g_strFlashState.TaskProgess4 = 0;
		g_strFlashState.TaskProgess5 = 0;
		g_strFlashState.TaskProgess6 = 0;
		g_strFlashState.TaskProgess7 = 0;
		g_strFlashState.TaskProgess8 = 0;
		WaitAudioPlayOver(TRUE);
		
		taskENTER_CRITICAL();
		flash_state_write(&g_strFlashState);
	    ssid_pwd.flag = 1;
		flash_ssid_pwd_write(&ssid_pwd);					
		taskEXIT_CRITICAL();
		
		PlayAudio(l_u8Path21,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
		WaitAudioPlayOver(TRUE);
    }
	else if(204 == l_oid_data->index){
		//测试物理直线
	    adj_motor_state(9,MOTOR_FORWARD,9,MOTOR_FORWARD);
    }
	//写入ID号
	else if(205 == l_oid_data->index){
		//如果米小克还没有ID,则从SD卡中读取ID,写入内存中
	//	  imeek_id_write(ImeekId);
		imeek_id_read(&ImeekId);
		//米小克ID以82开头
		//待修改:所有的写入都应该做一个校验，读出比较
		if((ImeekId.Id[0] != '8')
		|| (ImeekId.Id[1] != '2')){
			if(ImeekReadFileLine("imeek_id/IMEEK_ID",buff,20)){
				//米小克ID为12位，因此13位必须为结束符，以防读出的数据大于12位
				buff[12] = '\0';
				WaitAudioPlayOver(TRUE);
				strcpy(ImeekId.Id,buff);
				ImeekId.SendFlag = FALSE;
				taskENTER_CRITICAL();
				imeek_id_write(&ImeekId);
				taskEXIT_CRITICAL();
			}
			else{
//				printf("read id error\n");
			}
		}
		imeek_id_read(&g_strImeekId);
		while(1){
			//已经写有ID了
			if('8' == g_strImeekId.Id[0]){
				sprintf(path,"%s%s",file,l_u8Path8);
//				printf("path8 = %s\n",path);
				PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
				WaitAudioPlayOver(TRUE);
				for(i=0;i<5;i++){
					sprintf(path,"%s%d%s","file://music/FF1",g_strImeekId.Id[i+7]-'0',".mp3");
//					printf("pathid = %s\n",path);
					PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
					WaitAudioPlayOver(TRUE);
				}
			}
			else{
				sprintf(path,"%s%s",file,l_u8Path7);
				PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
				WaitAudioPlayOver(TRUE);
			}
			
			if(FALSE == g_strImeekId.SendFlag){
				sprintf(path,"%s%s",file,l_u8Path5);
				PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
				WaitAudioPlayOver(TRUE);
			}
			else{
				sprintf(path,"%s%s",file,l_u8Path6);
                PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
				WaitAudioPlayOver(TRUE);
			}
			OS_MSleep(100);
		}
	}
	//清除米小克ID
	else if(206 == l_oid_data->index){
	    g_strImeekId.SendFlag = FALSE;
	    sprintf(g_strImeekId.Id,"%s","FF");
		WaitAudioPlayOver(TRUE);
		imeek_id_write(&g_strImeekId);
		sprintf(path,"%s%s",file,l_u8Path20);
		PlayAudio(path,THIRD_LEVEL_AUDIO,IMEEK_PLAY);
		WaitAudioPlayOver(TRUE);		
	}
	//触摸测试
	else if(207 == l_oid_data->index){
		while(1){
			set_imeek_light_struct(LIGHT_ON,LIGHT_BLUE,LIGHT_FORM,LIGHT_CONTINUE,2,
								   LIGHT_ON,LIGHT_BLUE,LIGHT_FORM,LIGHT_CONTINUE,2);
			for(i=0;i<10;i++){
				if(1 == g_strTouchKey.ShortPressFlag){
					g_strTouchKey.ShortPressFlag = 0;
					sprintf(path,"%s%s",file,l_u8Path22);
					WaitAudioPlayOver(TRUE);
					PlayAudio(path,ZERO_LEVEL_AUDIO,IMEEK_PLAY);
					WaitAudioPlayOver(TRUE);
					break;
				}
				OS_MSleep(100);
			}
			set_imeek_light_struct(LIGHT_ON,LIGHT_GREEN,LIGHT_FORM,LIGHT_CONTINUE,2,
								   LIGHT_ON,LIGHT_GREEN,LIGHT_FORM,LIGHT_CONTINUE,2);
			for(i=0;i<10;i++){
				if(1 == g_strTouchKey.ShortPressFlag){
					g_strTouchKey.ShortPressFlag = 0;
					sprintf(path,"%s%s",file,l_u8Path22);
					WaitAudioPlayOver(TRUE);
					PlayAudio(path,ZERO_LEVEL_AUDIO,IMEEK_PLAY);
					WaitAudioPlayOver(TRUE);
					break;
				}
				OS_MSleep(100);
			}
			set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_FORM,LIGHT_CONTINUE,2,
								   LIGHT_ON,LIGHT_RED,LIGHT_FORM,LIGHT_CONTINUE,2);
			for(i=0;i<10;i++){
				if(1 == g_strTouchKey.ShortPressFlag){
					g_strTouchKey.ShortPressFlag = 0;
					sprintf(path,"%s%s",file,l_u8Path22);
					WaitAudioPlayOver(TRUE);
					PlayAudio(path,ZERO_LEVEL_AUDIO,IMEEK_PLAY);
					WaitAudioPlayOver(TRUE);
					break;
				}
				OS_MSleep(100);
			}
		}
	}
	else if(218 == l_oid_data->index){
		l_32CodeTemp = g_strFlashState.LocalCodeVersion;
		l_u8CodeTemp[7] = l_32CodeTemp / 10000000;
		l_32CodeTemp    = l_32CodeTemp % 10000000;
		l_u8CodeTemp[6] = l_32CodeTemp / 1000000;
		l_32CodeTemp    = l_32CodeTemp % 1000000;
		l_u8CodeTemp[5] = l_32CodeTemp / 100000;
		l_32CodeTemp    = l_32CodeTemp % 100000;
		l_u8CodeTemp[4] = l_32CodeTemp / 10000;
		l_32CodeTemp    = l_32CodeTemp % 10000;
		l_u8CodeTemp[3] = l_32CodeTemp / 1000;
		l_32CodeTemp    = l_32CodeTemp % 1000;
		l_u8CodeTemp[2] = l_32CodeTemp / 100;
		l_32CodeTemp    = l_32CodeTemp % 100;
		l_u8CodeTemp[1] = l_32CodeTemp / 10;
		l_32CodeTemp    = l_32CodeTemp % 10;
		l_u8CodeTemp[0] = l_32CodeTemp;
		for(i=0;i<8;i++){
			printf("l_u8CodeTemp[%d] = %d",i,l_u8CodeTemp[i]);
		}
		printf("\n");
		WaitAudioPlayOver(TRUE);
		while(1){
			sprintf(path,"%s%s",file,l_u8Path27);
			printf("PathCode = %s\n",path);
			PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
			WaitAudioPlayOver(TRUE);
			for(i=8;i>0;i--){
				sprintf(path,"%s%d%s","file://music/FF1",l_u8CodeTemp[i-1],".mp3");
				printf("PathCode = %s\n",path);
				PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
				WaitAudioPlayOver(TRUE);
			}
		}
		
	}	
	else if(219 == l_oid_data->index){
		//降低软件版本号
		g_strFlashState.LocalCodeVersion = 1;
		WaitAudioPlayOver(TRUE);
		
		taskENTER_CRITICAL();
		flash_state_write(&g_strFlashState);
		taskEXIT_CRITICAL();
		
		sprintf(path,"%s%s",file,l_u8Path28);
		printf("PathCode = %s\n",path);
		PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
		WaitAudioPlayOver(TRUE);
	}
	else if(200 == l_oid_data->index){
	}
	else if(230 == l_oid_data->index){
	}
	else if(231 == l_oid_data->index){
	}
    else if(NO_VALID_CODE == l_oid_data->code_flag){
	    while(1){
			adj_motor_state(6,MOTOR_FORWARD,6,MOTOR_FORWARD);
			state = GetTrailing();
			if(state & 0x01){
				sprintf(path,"%s%s",file,l_u8Path3);
			}
			else if(state & 0x02){
				sprintf(path,"%s%s",file,l_u8Path2);
			}
			else if(state & 0x04){
				sprintf(path,"%s%s",file,l_u8Path4);
			}
			else{
				sprintf(path,"%s%s",file,l_u8Path1);
			}
			PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
//	        printf("IR_state = %d\n",state);
			OS_MSleep(1000);
	   }
    }
}

/*
*******************************************************************************
 *Function:     PlayOidData
 *Description:  播放OID码值
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void PlayOidData(STRUCT_APP_OID_DATA *l_debounce_data)
{
	char *l_ps8Path29 = "file://music/FF29.mp3";
	char *l_ps8Path30 = "file://music/FF30.mp3";
	char *l_ps8Path31 = "file://music/FF31.mp3";
	char path[30];

	uint32_t i;
	uint32_t l_32CodeTemp;
	uint8_t l_u8CodeTemp[20];
	if(GENERAL_CODE == l_debounce_data->code_flag){
		PlayAudio(l_ps8Path29,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
		l_32CodeTemp = l_debounce_data->index;
		l_u8CodeTemp[4] = l_32CodeTemp / 10000;
		l_32CodeTemp	= l_32CodeTemp % 10000;
		l_u8CodeTemp[3] = l_32CodeTemp / 1000;
		l_32CodeTemp	= l_32CodeTemp % 1000;
		l_u8CodeTemp[2] = l_32CodeTemp / 100;
		l_32CodeTemp	= l_32CodeTemp % 100;
		l_u8CodeTemp[1] = l_32CodeTemp / 10;
		l_32CodeTemp	= l_32CodeTemp % 10;
		l_u8CodeTemp[0] = l_32CodeTemp;
		for(i=0; i<5; i++){
			printf("l_u8CodeTemp[%d] = %d",i,l_u8CodeTemp[i]);
		}
		printf("\n");
		WaitAudioPlayOver(TRUE);
		for(i=5;i>0;i--){
			sprintf(path,"%s%d%s","file://music/FF1",l_u8CodeTemp[i-1],".mp3");
			printf("PathCode = %s\n",path);
			PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
			WaitAudioPlayOver(TRUE);
		}
	}
	else if(POSITION_CODE == l_debounce_data->code_flag){
		PlayAudio(l_ps8Path30,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
		l_32CodeTemp = l_debounce_data->position.x;
		l_u8CodeTemp[4] = l_32CodeTemp / 10000;
		l_32CodeTemp	= l_32CodeTemp % 10000;
		l_u8CodeTemp[3] = l_32CodeTemp / 1000;
		l_32CodeTemp	= l_32CodeTemp % 1000;
		l_u8CodeTemp[2] = l_32CodeTemp / 100;
		l_32CodeTemp	= l_32CodeTemp % 100;
		l_u8CodeTemp[1] = l_32CodeTemp / 10;
		l_32CodeTemp	= l_32CodeTemp % 10;
		l_u8CodeTemp[0] = l_32CodeTemp;
		for(i=0; i<5; i++){
			printf("l_u8CodeTemp[%d] = %d\n",i,l_u8CodeTemp[i]);
		}
		printf("\n");
		WaitAudioPlayOver(TRUE);
		for(i=5;i>0;i--){
			sprintf(path,"%s%d%s","file://music/FF1",l_u8CodeTemp[i-1],".mp3");
			printf("PathCode = %s\n",path);
			PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
			WaitAudioPlayOver(TRUE);
		}

		PlayAudio(l_ps8Path31,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
		l_32CodeTemp = l_debounce_data->position.y;
		l_u8CodeTemp[4] = l_32CodeTemp / 10000;
		l_32CodeTemp	= l_32CodeTemp % 10000;
		l_u8CodeTemp[3] = l_32CodeTemp / 1000;
		l_32CodeTemp	= l_32CodeTemp % 1000;
		l_u8CodeTemp[2] = l_32CodeTemp / 100;
		l_32CodeTemp	= l_32CodeTemp % 100;
		l_u8CodeTemp[1] = l_32CodeTemp / 10;
		l_32CodeTemp	= l_32CodeTemp % 10;
		l_u8CodeTemp[0] = l_32CodeTemp;
		for(i=0; i<5; i++){
			printf("l_u8CodeTemp[%d] = %d",i,l_u8CodeTemp[i]);
		}
		printf("\n");
		WaitAudioPlayOver(TRUE);
		for(i=5;i>0;i--){
			sprintf(path,"%s%d%s","file://music/FF1",l_u8CodeTemp[i-1],".mp3");
			printf("PathCode = %s\n",path);
			PlayAudio(path,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
			WaitAudioPlayOver(TRUE);
		}		
	}
}


/*
*******************************************************************************
 *Function:     ExecuteTask
 *Description:  执行当前任务
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void ExecuteTask(STRUCT_APP_OID_DATA *l_realtime_data,
                 STRUCT_APP_OID_DATA *l_debounce_data,
                 ENUM_RUNNING_STATE work_state,
                 MYBOOL* write_flash)
{
	switch(work_state){
		case TASK_MODE:
//			printf("TASK MODE\n");
		case FREE_MODE:
//			printf("FREE MODE\n");
//			printf("imeek_work_state.current_mode_state = %d\n",imeek_work_state.current_mode_state);
			//初始化用到的变量
			MissionFreeSpellModeInit();
			//获取指令卡数据:这里的数据需要实时
			GetInstruction(l_realtime_data);
			//判断指令卡摆放是否正确
			DetectInstruction(user_instruction_card,user_instruction_card_index);
			//执行指令卡片：这里的数据需要实时
			ExecuteInstruction(user_instruction_card,user_instruction_card_index,
							   l_realtime_data, imeek_work_state.map,write_flash);
			break;
		case SPELL_MODE:
			MissionFreeSpellModeInit();
			GetSpellInstruction(l_realtime_data);
			PlayEnglishSpellCard(user_instruction_card,user_instruction_card_index);
			break;
		case MUSIC_EDIT_MODE:
//			ComposeModeInit();
//			ComposeReadNote(l_debounce_data);
//			ComposePlayNote(&app_oid_data);
			break;

		case RECORD_MODE:
//			RecordModeInit();
			Recording(l_debounce_data);
			break;

		case QUESTION_MODE:
			break;
//		case TRAILING_MODE:
//			ImeekTrailingInit();
//			imeek_trailing();
//			ImeekTrailingOver();
//			break;
		case TEST_MODE:
			JudgeMode(l_debounce_data);
//			PlayOidData(l_debounce_data);
			break;
		default:
			break;
	}
}

void WriteFlash(MYBOOL write_flash_flag, ENUM_PLAY_TYPE play_state)
{
    static MYBOOL l_bLastFlag = FALSE;

	//本次标志需要写数据或之前有记录要写数据
    if((TRUE == write_flash_flag)
	|| (TRUE == l_bLastFlag)){
        if(IMEEK_PLAY != play_state){
			taskENTER_CRITICAL();
			l_bLastFlag = FALSE;
			flash_state_write(&g_strFlashState);
			taskEXIT_CRITICAL();
		}
		else{
            l_bLastFlag = TRUE;
		}
	}
}


void app_task(void *pvParameters)
{
	MYBOOL l_bWirteFlash1 = FALSE;
	MYBOOL l_bWirteFlash2 = FALSE;
	ENUM_OID_POINT_TO l_enumOidPointTo = OID_POINT_TO_NULL;
	STRUCT_APP_OID_DATA l_strRTAppOidData;
	STRUCT_APP_OID_DATA l_strDebounceOidData;
	ENUM_KEY_TRIGGER_IMEEK_STATE l_enumKeyTriggerState;
	
	run_instrutcion_Queue = xQueueCreate( 2, sizeof( STRUCT_RUN_INSTRUCTION_MESSAGE ) );
	if(NULL == run_instrutcion_Queue){
		APP_MSG_DBG("run_instrutcion_Queue create error\n");
		return;
	}
    
	run_instruction_event_group = xEventGroupCreate();
	if(run_instruction_event_group == NULL){
		APP_MSG_DBG("run_instruction_event_group create failed!\r\n");
		return;
	}
    
 	while(1){
		//获取OID码值
		GetAppOidData(&oid_data,&l_strRTAppOidData,&l_strDebounceOidData);
		
		//获得组合按键状态
		l_enumKeyTriggerState = KeyTriggerImeekState();

		//判断是否要进入休闲模式
		JdgIntoIdleMode(&l_strDebounceOidData);
		ExtractMsgFromOidData(&l_strDebounceOidData,&l_enumOidPointTo);
		//获取接收的任务
		InitMissionState(l_enumKeyTriggerState,
		                 &imeek_work_state.state);
		
		//执行任务，播放音频
		PlayImeekAudio(&l_strDebounceOidData,
		               &l_bWirteFlash1,
		               l_enumOidPointTo);
		ExecuteTask(&l_strRTAppOidData,
		            &l_strDebounceOidData,
		            imeek_work_state.state,
		            &l_bWirteFlash2);
		
		//记录数据
		WriteUserData(l_enumKeyTriggerState);
		ClearWifiData(l_enumKeyTriggerState);
		WriteFlash(l_bWirteFlash1|l_bWirteFlash2,OidPlayAudio.CurrentState);
		oid_data.flag = OID_DATA_NO_VALID;
		l_bWirteFlash1 = FALSE;
		l_bWirteFlash2 = FALSE;
		OS_MSleep(30);
	}
}

void run_instruct_task(void *pvParameters)
{
    uint8_t return_value = 0;
	STRUCT_APP_OID_DATA l_strRTAppOidData;
	STRUCT_APP_OID_DATA l_strDebounceOidData;

	BaseType_t xResult;
    STRUCT_RUN_INSTRUCTION_MESSAGE pxRxedMessage;

	while(1){
		if( run_instrutcion_Queue != 0 ){
			xResult = xQueueReceive(run_instrutcion_Queue, &(pxRxedMessage), (TickType_t) 10);
			if(xResult == pdPASS){
				while(1){
//					APP_MSG_DBG("run_instruct_task run\n");
					GetAppOidData(&oid_data,&l_strRTAppOidData,&l_strDebounceOidData);
//					printf("imeek_work_state.direction = %d\n",imeek_work_state.direction);
//					printf("pxRxedMessage.instruction = %d\n",pxRxedMessage.instruction);
//					printf("pxRxedMessage.parameter = %d\n",pxRxedMessage.parameter);
					return_value = run_instruction(pxRxedMessage.instruction,pxRxedMessage.parameter,&l_strRTAppOidData);
					if(RETURN_TRUE == return_value){
						//释放信号量
						xEventGroupSetBits(run_instruction_event_group, RUN_INSTRUCTION_SUCCESS);
						break;
					}
					else if(RETURN_ERROR_2 == return_value){
						xEventGroupSetBits(run_instruction_event_group, RUN_INSTRUCTION_CAR_FORBIDDEN);
						break;
					}
					else if(RETURN_ERROR_3 == return_value){
						xEventGroupSetBits(run_instruction_event_group, RUN_INSTRUCTION_OVER_MAP);
						break;
					}
//					l_strRTAppOidData.code_flag = NO_VALID_CODE;
					OS_MSleep(40);
				}
			}
		}
	}
}

/*
 *该函数用于运行与用户操作无关的逻辑代码段,包括:
 *       1:电池电量检测
 */
void get_battery_voltage_task(void *pvParameters)
{
	adc_get();
}

int get_battery_voltage()
{
	if (OS_ThreadCreate(&get_battery_voltage_task_handler,
						"get_battery_voltage_task",
						get_battery_voltage_task,
						NULL,
						OS_THREAD_PRIO_APP,
						BAT_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
		APP_MSG_DBG("app thread create error\n");
		return -1;
	}
	return 0;
}

void trailing_mode_task(void *pvParameters)
{
	//红外寻迹传感器初始化,红外寻迹传感器占了用调试引脚
	IrEnableGpioInit();
	ir_led_gpio_init();
	
    while(1){
		ImeekTrailingInit();
		imeek_trailing();
		ImeekTrailingOver();
		OS_MSleep(100);
	}
    
}

int LightTaskCreate(void)
{
	if (OS_ThreadCreate(&light_handler,
						"light_task",
						light_task,
						NULL,
						OS_THREAD_PRIO_APP,
						LIGHT_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK){
		return -1;
	}
	return 0;
}

int AppTaskCreate(void)
{
    get_battery_voltage();

	if (OS_ThreadCreate(&idle_task_handler,
						"idle_task",
						IdleTask,
						NULL,
						OS_THREAD_PRIO_APP,
						IDLE_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
		return -1;
	}
	
	if (OS_ThreadCreate(&app_handler,
						"app_task",
						app_task,
						NULL,
						OS_THREAD_PRIO_APP,
						APP_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
		return -1;
	}
    
	if (OS_ThreadCreate(&run_instruct_handler,
						"run_instruct_task",
						run_instruct_task,
						NULL,
						OS_THREAD_PRIO_APP,
						RUN_INSTRUCT_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
		APP_MSG_DBG("app thread create error\n");
		return -1;
	}
	
	return 0;
}

int TrailingModeCreate(void)
{
	if (OS_ThreadCreate(&trailing_mode_handler,
						"trailing_mode_task",
						trailing_mode_task,
						NULL,
						OS_THREAD_PRIO_APP,
						TRAILING_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
		APP_MSG_DBG("app thread create error\n");
		return -1;
	}
	
	return 0;

}

