/*�� .c �ļ����ڴ�����������ص��߼�*/

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
 *Description:  ��ȡSSID��PWD,����ڴ��б����ˣ�����ڴ��л�ȡ;
 *                            ����ڴ���û�б��棬������Airkiss��ȡ��
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:�ɹ���ȡ��SSID
 *              FALSE:û�л�ȡ��SSID
 *Others:       ��ͨ��Airkiss�ķ�ʽ��ȡwifi�˺�������ʱ�����������˺Ż�������
 *              ���󽫻᳤ʱ��ȴ�
*******************************************************************************
*/
void GetSsidPwd(STRUCT_SSID_PWD *l_ssid_pwd)
{
	char i;
	
//	flash_ssid_pwd_read(l_ssid_pwd);
//	//����ڴ��б�����SSID,��ֱ�ӷ��ؼ���
//    if(!(l_ssid_pwd->flag)){
//		APP_MSG_DBG("Get SSID and PWD from flash\n");
//		APP_MSG_DBG("get pwd,ssid,	ssid = %s,	pwd = %s\n",l_ssid_pwd->ssid , l_ssid_pwd->pwd);
//	}
//    else{
		AirKissFlag = TRUE;
		//�ڴ��в�����SSID,ͨ��Airkiss��ȡ
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
			//Airkiss��ȡ��SSID��PWD
			//���޸�:��֪��Ϊʲô�ٻ�ȡ��ssid��ֱ���������罫����������������������ֱ���ٽ���������
			ota_reboot();
		}
		else{
			AudioPlayToEnd("file://music/Public/_2.mp3",SECOND_LEVEL_AUDIO,IMEEK_PLAY);
			imeek_network_event.network_state = NETWORK_DISCONNECT;
		}
//	}
}

/*
 *�ú������ڴ����й�������ص��߼�
 *�������һ������ʱ���󣬻��һЩ����
 */
void network_task(void *pvParameters)
{
    //��Ҫ����һ���ṹ��:�����еĵ�ǰ״̬�������ڽṹ�У�
    //                   Ȼ����ʽ���ڸú�����ͳһ����
    //����״̬:  1���տ�����û���ӹ�����
    //                 (1)�˺������־λ��Ч����ȡ�˺����룬�������磬
    //                    ����˺�����Ϊ����ģ�����Ҫͨ������wifi��λ
    //                    �������ֶ�����airkiss����ģʽ������������Ҫ
    //                    �и��۾�����ʾ
    //                 (2)�˺������־λ��Ч������airkiss����ģʽ��
    //           2������������
    //           3�������ѶϿ�
//    MYBOOL ssid_flag = 1;   //TRUE:��ȡ��ssid   FALSE:��ȡssidʧ��
	EventBits_t uxBits;

	net_event_group = xEventGroupCreate();
	if(net_event_group == NULL){
		APP_MSG_DBG("net_event_group create failed!\r\n");
		return;
	}
				
//	ssid_flag = GetSsidPwd(&ssid_pwd);
	
	//��ȡ��WIFI�˺�������
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
									 30000);//portMAX_DELAY���õȴ�
	
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
 *Description:  ��ȡ�˺�������
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:�ڴ��б�����SSID��PWD
 *              FALSE:�ڴ���û�б���SSID��PWD
 *Others:       ��ȡ��־λ��ͬʱҲ�Ѿ���SSID��PWD�����ڽṹ����
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
    //��ҪOTA�������ڴ���û���˺�����ʱ����Ҫ����Airkiss
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

