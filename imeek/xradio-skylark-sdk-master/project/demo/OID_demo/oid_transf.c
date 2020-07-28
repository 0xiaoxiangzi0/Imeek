#include "driver/chip/hal_gpio.h"
#include "FreeRTOS.h"
#include "kernel/os/os.h"
#include "driver/chip/hal_wdg.h"

#include "oid_transf.h"
#include "oid_protocol.h"
#include "stepper_motor.h"

OID_DATA_STRUCT oid_data;


#define OID_SERVICE_TASK_CTRL_THREAD_STACK_SIZE		(1 * 1024)

static OS_Thread_t oid_service_handler;

///*
//*******************************************************************************
// *Function:     OIDReboot
// *Description:  
// *Calls:       
// *Called By:   
// *Input:        
// *Output:       
// *Return:       
// *Others:      
//*******************************************************************************
//*/
//void OIDReboot(void)
//{
//    HAL_WDG_Reboot();
//}

//����ʱʹ�ã����ڴ�ӡ��ȡ����OIDֵ
static void oid_data_printf(OID_DATA_STRUCT oid_data)
{
	if(OID_DATA_VALID == oid_data.flag){
		if(1 == oid_data.data_type){
			OID_MSG_DBG("\n\noid power on command\n");
			OID_MSG_DBG("	 oid cmd = 0x%x\n", (uint16_t)oid_data.cmd);
			switch(oid_data.cmd){
				case 0xfff8:
					OID_MSG_DBG("	 Decoder SOC has already entered to normal mode\n");
					break;
				case 0xfff1:
					OID_MSG_DBG("	 Decoder SOC has something wrong and has finished reset\n");
					break;
				case 0xfff6:
					OID_MSG_DBG("	 Decoder SOC is going to enter calibration mode.\n");
					OID_MSG_DBG("	 DSP/MCU has to set 2-wire SCK and SDIO as input mode\n");
					break;
			}
		}
		//������ͷ
		else if(1 == oid_data.oid_mode){
		//	OID_MSG_DBG("oid_mode:%d\n",oid_data.oid_mode);
			if(0 == oid_data.valid_code){
				OID_MSG_DBG("oid power on position data:\n");
				OID_MSG_DBG("	 valid code\n");
				if(oid_data.code_type){
					OID_MSG_DBG("	 position code\n");
					OID_MSG_DBG("		 x = %d\n",(uint16_t)oid_data.position.x);
					OID_MSG_DBG("		 y = %d\n",(uint16_t)oid_data.position.y);
				}
				else{
					OID_MSG_DBG("	 general code\n");				
					OID_MSG_DBG("		 index = %d\n",(uint32_t)oid_data.index);				
				}
			}
		}
		//������ͷ
		else if(0 == oid_data.oid_mode){
			if(0 == oid_data.valid_code){
				OID_MSG_DBG("oid_mode:%d\n",oid_data.oid_mode);
				OID_MSG_DBG("	 general code\n");				
				OID_MSG_DBG("		 index = %d\n",(uint32_t)oid_data.index);				
			}
		}
	}		
}

/*
*******************************************************************************
 *Function:     JdgOidReset
 *Description:  �ж�OID�ķ���ֵ�����OID������RESET�������Ҫ�Ե����ͷ
 *              �������µ�Ԥ����
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       1:�����ͷ��Ҫ�����³�ʼ��
 *              0:�����ͷ��������
 *Others:      
*******************************************************************************
*/
MYBOOL JdgOidReset(OID_DATA_STRUCT *data)
{
	if((1 == data->data_type)  
	&& (0xFFF1 == data->cmd)){
	    return 1;    
	}
	return 0;
}

/*
 *ʵ���鴴����OID��ͷĬ�Ͽ���ÿ50ms�ϱ�һ������;
 *���δ�չOID��ͷĬ�Ͽ���ÿ200ms�ϱ�һ�����ݣ�
*/
void oid_service_task(void *pvParameters)
{
	OID_MSG_INF("oid service thread create success, handler = 0x%x\n", (uint32_t)oid_service_handler.handle);
    uint32_t RecvData1;
	uint32_t RecvData2;
		
	while(1){
		taskENTER_CRITICAL();
		RecvData1 = 0;
		RecvData2 = 0;
		oid_data = RecvOIDData(&RecvData1,&RecvData2);
		taskEXIT_CRITICAL();
//		oid_data_printf(oid_data);

		OS_MSleep(20);
	}
}

int OidDetectionTaskCreate(void)
{
//	int i;
	OIDGpioInit();
	
	//����ǲ��ϵ縴λ�������û�жϵ磬����ʱ����ʴ�������ģʽ������Ԥ���ò���
	
//	for(i=3;TransmitCmdToOID(OID_CMD_MIN_FRAME_RATE_20) && i>0;i--){
//		OS_MSleep(100);
//	}
	if(TransmitCmdToOID(OID_CMD_MIN_FRAME_RATE_80)){
		printf("1111111111111OID INIT11111111111111\n");
		OS_MSleep(100);
		//���ѵ���ʱ���Ҫ���ٽ�������ɣ�������ܱ����
//		taskENTER_CRITICAL();
//		WakeUpOID();
//		taskEXIT_CRITICAL();
		if(FALSE == WakeUpOID()){
			printf("Wake UP OID Error\n");
			return -1;
		}
		OID_MSG_DBG("oid power on init success\n");
		DecoderInit();
		while(TransmitCmdToOID(OID_CMD_MIN_FRAME_RATE_80));
		while(TransmitCmdToOID(OID_CMD_POSITION_ENABLE));
		while(TransmitCmdToOID(OID_CMD_POSITION_WORKING_MODE_ENABLE));
//		while(TransmitCmdToOID(OID_CMD_AUTO_SWITCH_TO_POS_ENABLE));//�������ȡ�ĸ߶������½�
//		while(TransmitCmdToOID(OID_CMD_AUTO_SWITCH_TO_GEN_ENABLE));//
	}
    else{
		printf("222222222222OID NO INIT22222222222222");
    }
		
    if (OS_ThreadCreate(&oid_service_handler,
						"oid_service_task",
						oid_service_task,
						NULL,
						OS_THREAD_PRIO_APP,
						OID_SERVICE_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
		OID_MSG_INF("oid service thread create error\n");
		return -1;
	}
	return 0;
}


