/*该 .c 文件用于处理与联网相关的逻辑*/

#include <stdio.h>
#include "FreeRTOS.h"
#include "kernel/FreeRTOS/event_groups.h"
#include "kernel/os/os.h"
#include "driver/chip/hal_gpio.h"
#include "driver/chip/hal_util.h"
#include "app.h"
#include "ir_led.h"
#include "stepper_motor.h"
#include "rgb_led.h"
#include "net_msg.h"
#include "socket_connect.h"
#include "oid_airkiss.h"
#include "oid_flash.h"
#include "oid_network_connect.h"
#include "imeek_key.h"
#include "oid_audio.h"
#include "imeek_ota.h"

extern void ota_reboot(void);


#define NETWORK_TASK_CTRL_THREAD_STACK_SIZE		 (4 * 1024)
#define GET_SSID_PWD_TASK_CTRL_THREAD_STACK_SIZE (4 * 1024)

static OS_Thread_t network_handler;
static OS_Thread_t get_ssid_pwd_handler;


EventGroupHandle_t net_event_group;
STRUCT_NETWORK_EVENT imeek_network_event = {FALSE,NETWORK_DISCONNECT};


STRUCT_SSID_PWD ssid_pwd = {0,"imeektest","12345678"};
MYBOOL AirKissFlag = FALSE;

/*
*******************************************************************************
 *Function:     GetSsidPwd
 *Description:  获取SSID与PWD,如果内存中保存了，则从内存中获取;
 *                            如果内存中没有保存，则利用Airkiss获取；
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:成功获取到SSID
 *              FALSE:没有获取到SSID
 *Others:       当通过Airkiss的方式获取wifi账号与密码时，如果输入的账号或者密码
 *              错误将会长时间等待
*******************************************************************************
*/
void GetSsidPwd(STRUCT_SSID_PWD *l_ssid_pwd)
{
	char i;
	
//	flash_ssid_pwd_read(l_ssid_pwd);
//	//如果内存中保存了SSID,则直接返回即可
//    if(!(l_ssid_pwd->flag)){
//		APP_MSG_DBG("Get SSID and PWD from flash\n");
//		APP_MSG_DBG("get pwd,ssid,	ssid = %s,	pwd = %s\n",l_ssid_pwd->ssid , l_ssid_pwd->pwd);
//	}
//    else{
		AirKissFlag = TRUE;
		//内存中不存在SSID,通过Airkiss获取
		AudioPlayToEnd("file://music/Public/_4.mp3",SECOND_LEVEL_AUDIO,IMEEK_PLAY);
		APP_MSG_DBG("Never save SSID and PWD\n");
		for(i=0; i<3; i++){
			oid_airkiss_link(l_ssid_pwd);
			if(SSID_PWD_VALID == l_ssid_pwd->flag){
				while(IMEEK_PLAY == OidPlayAudio.CurrentState){
					OS_MSleep(50);
				}
				taskENTER_CRITICAL();
				printf("l_ssid_pwd->ssid = %s\n",l_ssid_pwd->ssid);
				flash_ssid_pwd_write(l_ssid_pwd);
				taskEXIT_CRITICAL();
				break;
			}
		}
		
		if(i<3){
			//Airkiss获取到SSID与PWD
			//待修改:不知道为什么再获取到ssid后，直接连接网络将卡死，所以这里先重启，直接再解决这个问题
			ota_reboot();
		}
		else{
			AudioPlayToEnd("file://music/Public/_2.mp3",SECOND_LEVEL_AUDIO,IMEEK_PLAY);
			imeek_network_event.network_state = NETWORK_DISCONNECT;
		}
//	}
}

/*
 *该函数用于处理有关联网相关的逻辑
 *当错过第一次联网时机后，会出一些问题
 */
void network_task(void *pvParameters)
{
    //需要建立一个结构体:将所有的当前状态都包含在结构中，
    //                   然后处理方式都在该函数中统一处理，
    //网络状态:  1、刚开机还没连接过网络
    //                 (1)账号密码标志位有效，读取账号密码，连接网络，
    //                    如果账号密码为错误的，则需要通过长按wifi复位
    //                    按键，手动进入airkiss配网模式，进入配网需要
    //                    有个眼睛灯提示
    //                 (2)账号密码标志位无效，进入airkiss配网模式。
    //           2、网络已连接
    //           3、网络已断开
//    MYBOOL ssid_flag = 1;   //TRUE:获取到ssid   FALSE:获取ssid失败
	EventBits_t uxBits;

	net_event_group = xEventGroupCreate();
	if(net_event_group == NULL){
		APP_MSG_DBG("net_event_group create failed!\r\n");
		return;
	}
				
//	ssid_flag = GetSsidPwd(&ssid_pwd);
	
	//获取到WIFI账号与密码
    if(FALSE == AirKissFlag){
//		printf("ssid_pwd.flag = %d\n",ssid_pwd.flag);
//		printf("ssid = %s\n",ssid_pwd.ssid);
//		printf("pwd  = %s\n",ssid_pwd.pwd);
		net_service_task_create();
		socket_task_init((uint8_t *)ssid_pwd.ssid,
						  strlen((const char *)ssid_pwd.ssid),
						 (uint8_t *)ssid_pwd.pwd);
	
		uxBits = xEventGroupWaitBits(net_event_group,	   
									 NET_CONNECT_SUCCESS | 
									 NET_CONNECT_FAIL	 | 
									 NET_DOWN , 		   
									 pdTRUE,			   
									 pdFALSE,			   
									 30000);//portMAX_DELAY永久等待
	
		if(uxBits & NET_CONNECT_SUCCESS){
			AudioPlayToEnd("file://music/Public/_5.mp3",SECOND_LEVEL_AUDIO,IMEEK_PLAY);
	//			APP_MSG_DBG("NETWORK_CONNECT\r\n");
			imeek_network_event.network_state = NETWORK_CONNECT;
		}
		else if(uxBits & NET_CONNECT_FAIL){
			AudioPlayToEnd("file://music/Public/_2.mp3",SECOND_LEVEL_AUDIO,IMEEK_PLAY);
	//			APP_MSG_DBG("NETWORK_DISCONNECT\r\n");
			imeek_network_event.network_state = NETWORK_DISCONNECT;
		}
		else if(uxBits & NET_DOWN){
	//			APP_MSG_DBG("NETWORK_DOWN\r\n");
			imeek_network_event.network_state = NETWORK_DISCONNECT;
		}
		else{
	//			APP_MSG_DBG("NETWORK_ERROR\r\n");
			AudioPlayToEnd("file://music/Public/_2.mp3",SECOND_LEVEL_AUDIO,IMEEK_PLAY);
		}
	}
    imeek_network_event.flag = TRUE;
	OS_ThreadDelete(&network_handler);
}

/*
*******************************************************************************
 *Function:     GetSsidPwdTask
 *Description:  获取账号与密码
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:内存中保存了SSID与PWD
 *              FALSE:内存中没有保存SSID与PWD
 *Others:       获取标志位的同时也已经将SSID与PWD保存在结构体中
*******************************************************************************
*/
void GetSsidPwdTask(void *pvParameters)
{
	GetSsidPwd(&ssid_pwd);
	OS_ThreadDelete(&get_ssid_pwd_handler);
//	while(1){
//        OS_Sleep(5);
//	}
}


int network_task_create(void)
{    
	if(HTTP_OTA == g_strOtaMode){
		if (OS_ThreadCreate(&network_handler,
							"network_task",
							network_task,
							NULL,
							OS_THREAD_PRIO_APP,
							NETWORK_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
			APP_MSG_DBG("network_connect thread create error\n");
			return -1;
		}
    }
	return 0;
}

int GetSsidPwdTaskCreate(void)
{
    //需要OTA升级且内存中没有账号密码时才需要开启Airkiss
	if((HTTP_OTA == g_strOtaMode)
	&& (SSID_PWD_NOVALID == ssid_pwd.flag)){
		if (OS_ThreadCreate(&get_ssid_pwd_handler,
							"get_ssid_pwd_task",
							GetSsidPwdTask,
							NULL,
							OS_THREAD_PRIO_APP,
							GET_SSID_PWD_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK){
			APP_MSG_DBG("GetSsidPwdTask thread create error\n");
			return -1;
		}
		OS_Sleep(1);
    }	
	return 0;
}

