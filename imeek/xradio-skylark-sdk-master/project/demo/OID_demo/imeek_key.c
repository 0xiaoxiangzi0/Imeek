
#include <stdio.h>
#include "kernel/os/os.h"
#include "driver/chip/hal_adc.h"
#include "driver/chip/private/hal_os.h"
#include "imeek_key.h"

static OS_Thread_t get_key_task_handler;
#define KEY_TASK_CTRL_THREAD_STACK_SIZE    (1 * 1024)


STRUCT_KEY g_strMultifunctionKey = {0,0,0,0,0,0,0,1,65535,0,KEY_CONT_TIME/KEY_SCAN_PERIOD};
STRUCT_KEY g_strWifiResetKey     = {0,0,0,0,0,0,0,1,65535,0,KEY_CONT_TIME/KEY_SCAN_PERIOD};
STRUCT_KEY g_strTouchKey         = {0,0,0,0,0,0,0,1,65535,0,KEY_CONT_TIME/KEY_SCAN_PERIOD};
STRUCT_KEY *g_arrkey[KEY_NUM]    = {&g_strMultifunctionKey, &g_strWifiResetKey, &g_strTouchKey};

#define KEY_ADC_BURST_MODE			(0)
#define KEY_ADC_IRQ_MODE 			ADC_IRQ_DATA
#define KEY_ADC_CHL 				ADC_CHANNEL_0
#define KEY_ADC_FEQ 				500000
#define KEY_ADC_FIRST_DELAY 		10

HAL_Semaphore key_voltage_sem;
#define KEY_ADC_SEM_INIT()		HAL_SemaphoreInitBinary(&key_voltage_sem)
#define KEY_ADC_SEM_WAIT()		HAL_SemaphoreWait(&key_voltage_sem, 10000)
#define KEY_ADC_SEM_RELEASE()	HAL_SemaphoreRelease(&key_voltage_sem)
#define KEY_ADC_SEM_DEINIT()	HAL_SemaphoreDeinit(&key_voltage_sem)

/*
*******************************************************************************
 *Function:     KeyScan
 *Description:  按键检测
 *Calls:       
 *Called By:   
 *Input:       
 *Output:      
 *Return:      
 *Others:       
*******************************************************************************
*/
void KeyScan(STRUCT_KEY **key,u8 num)
{
    s8 l_s8i = 0;
    u8 bit;
    u8 l_u8ReadData = 0;
    u8 l_u8Trg = 0;
    u8 l_u8Cont = 0;
	
    for(l_s8i = num - 1; l_s8i >= 0; l_s8i--){
	    if(1 == key[l_s8i]->KeyType){
		    l_u8ReadData = ((l_u8ReadData << 1) | key[l_s8i]->KeyPress) ^ 0xff;
		}
		else{
		    l_u8ReadData = (l_u8ReadData << 1) | key[l_s8i]->KeyPress;
		}
				
		l_u8Cont = (l_u8Cont << 1)  | (key[l_s8i]->State & 0x01);
	}
		
    l_u8Trg     = l_u8ReadData & (l_u8ReadData ^ l_u8Cont);
    l_u8Cont    = l_u8ReadData;
		
    for(l_s8i = 0; l_s8i < num; l_s8i++){
		bit = 1 << l_s8i;
        key[l_s8i]->State = ((!!(l_u8Trg & bit)) << 1) | (!!(l_u8Cont & bit));
	}
}

/*
*******************************************************************************
 *Function:     KeyClassify
 *Description:  识别长按，短按，多次按键
 *Calls:       
 *Called By:   
 *Input:       
 *Output:      
 *Return:      
 *Others:       
*******************************************************************************
*/
void KeyClassify(STRUCT_KEY **key, u8 num)
{   
    u8 l_u8i;
    for(l_u8i = 0; l_u8i < num; l_u8i++){
        switch(key[l_u8i]->State){
	        //按键情况：上一次未按下，这一次未按下，
	    	case 0x00:
	        //按键情况：上一次按下，这一次未按下，松开
	        case 0x02:
	        //按键情况：上一次未按下，这一次按下，
            case 0x03:
				    //如果单前按键既不是长按，也不是连击，则认为是单击
    		    if((key[l_u8i]->Interval == (KEY_INTERVAL/KEY_SCAN_PERIOD + 1))
				&& (0 == key[l_u8i]->KeyPressTimes)){
				    if(0 == key[l_u8i]->LoosenFlag){
				        key[l_u8i]->LoosenFlag = 1;
					}
					else{
    				    key[l_u8i]->ShortPressFlag = 1;
					}
    			}
						
				    //连击次数足够，连击标志位置一
    		    if(key[l_u8i]->Interval > KEY_INTERVAL/KEY_SCAN_PERIOD){
				    key[l_u8i]->KeyPressTimes = 0;
    			}
    			if(key[l_u8i]->KeyPressTimes >= KEY_MUL_CLICK - 1){
				    key[l_u8i]->KeyMulClickFlag = 1;
			    }
						
    		    if(key[l_u8i]->Interval<65535){
    		        key[l_u8i]->Interval++;
    			}
    		    key[l_u8i]->ContinueTime = 0;
	            break;
				
		    //按键情况：上一次按下，这一次按下，
		    case 0x01:
			    //距离上一次按键时间，连续按着
			    if(0 == key[l_u8i]->Interval){
				    key[l_u8i]->ContinueTime++;
				    if((1 == key[l_u8i]->LoosenFlag) 
				    && (key[l_u8i]->ContinueTime > key[l_u8i]->ContTimeToPress)){
					    key[l_u8i]->LoosenFlag = 0;
						key[l_u8i]->LongPressFlag = 1;
					}
				}
			    else if(key[l_u8i]->Interval < KEY_INTERVAL/KEY_SCAN_PERIOD){
				    key[l_u8i]->KeyPressTimes++;
				}
				key[l_u8i]->Interval = 0;
			    break;
		    default:
		        break;
		    }
		}
}

static uint8_t key_adc_value_status = 1;
static uint32_t key_adc_data;
static void key_adc_callback(void *arg)
{	
    HAL_Status status = HAL_OK;
	if (key_adc_value_status)		
		return;	
	ADC_Channel chl = *((ADC_Channel*)arg);	
//	static uint32_t count = 0;
    if (HAL_ADC_GetIRQState(chl) == ADC_DATA_IRQ) {		
//		if (count >= 3) {			
			key_adc_data = HAL_ADC_GetValue(chl);	
			status = HAL_ADC_ConfigChannel(chl, ADC_SELECT_DISABLE, KEY_ADC_IRQ_MODE, 0, 0);			
//			count = 0;			
			key_adc_value_status = 1;			
			KEY_ADC_SEM_RELEASE();		
//		} else			
	}
    if (status != HAL_OK)		
		printf("ADC disconfig error--- %d\n", status);
}

static void key_adc_init()
{	
    HAL_Status status = HAL_ERROR;	
	ADC_InitParam initParam;	
	initParam.delay = KEY_ADC_FIRST_DELAY;	
	initParam.freq = KEY_ADC_FEQ;
#if (__CONFIG_CHIP_ARCH_VER == 2)	
    initParam.vref_mode = 1;
#endif	
    printf("ADC init...\n");
    initParam.mode = ADC_CONTI_CONV;
    status = HAL_ADC_Init(&initParam);
    if (status != HAL_OK) {		
		printf("ADC init error %d\n", status);		
		return;
	}
}

static void key_adc_deinit()
{	
    HAL_Status status = HAL_ERROR;	
	status = HAL_ADC_DeInit();	
	if (status != HAL_OK)		
		printf("ADC deinit error %d\n", status);
}

static uint32_t key_adc_filter(uint32_t key_adc_data[3])
{	
    uint8_t j = 0, i = 0;	
	uint32_t sum = 0;	
	uint32_t max = 0;	
	for (i = 0; i < 3; i++) {	 
		for (j = i; j < 2; j++) {		 
			if (key_adc_data[i] >= key_adc_data[j+1]) {			 
				max = key_adc_data[i];			 
				key_adc_data[i] = key_adc_data[j+1];			 
				key_adc_data[j+1] = max;		 
			}	 
		}	
	}	
//	for (i = 0; i < 3; i++) {	 
//		sum = sum + key_adc_data[i];	
//	}	
	sum = key_adc_data[1];	
	return sum;
}

static void key_adc_voltage_config(uint8_t channel, uint8_t en)
{	
    HAL_Status status = HAL_ERROR;	
    ADC_Channel chl = (ADC_Channel)channel;	
	if (en) {		
		printf("ADC channel config...\n");
        status = HAL_ADC_ConfigChannel(chl, ADC_SELECT_ENABLE, KEY_ADC_IRQ_MODE, 0, 0);
        if (status != HAL_OK) {			
		    printf("ADC config error %d\n", status);			
		    return;		
	    }		
	    printf("ADC callback enable...\n");		
	    status = HAL_ADC_EnableIRQCallback(chl, key_adc_callback, &chl);		
	    if (status != HAL_OK) {			
		    printf("ADC IRQ Enable error %d\n", status);			
		    return;		
	    }		
	    printf("ADC convert start...\n");		
	    status = HAL_ADC_Start_Conv_IT();		
	    if (status != HAL_OK) {			
		    printf("ADC it mode start error %d\n", status);			
		    return;		
	    }	
	} 
	else {		
		printf("ADC convert stop...\n");		
		HAL_ADC_Stop_Conv_IT();		
		printf("ADC callback disable...\n");		
		HAL_ADC_DisableIRQCallback(chl);	
	}
}

void key_adc_voltage_init(void)
{
	KEY_ADC_SEM_INIT();	
	key_adc_init();
}

uint32_t key_adc_get(void)
{
    HAL_Status status = HAL_ERROR;	
	key_adc_value_status = 0;	
	ADC_Channel chl = (ADC_Channel)KEY_ADC_CHL;	
    status = HAL_ADC_ConfigChannel(chl, ADC_SELECT_ENABLE, KEY_ADC_IRQ_MODE, 0, 0);
    if (status != HAL_OK) {		
		printf("ADC it mode start error %d\n", status);	
	}	
	KEY_ADC_SEM_WAIT();	
	if (key_adc_value_status) {		
//		uint32_t adc_value = key_adc_filter(key_adc_data);		
		return key_adc_data;
	}
	return 0;
}

//static void adc_voltage_deinit()
//{	
//    key_adc_deinit();	
//	KEY_ADC_SEM_DEINIT();
//}


/*
*******************************************************************************
 *Function:     get_key_task
 *Description:  获取按键状态
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       本应用中:
 *              1、按多功能、WIFI复位   674
 *              2、按触摸、WIFI复位按键 815
 *              3、单按WIFI复位按键     822
 *              4、按触摸、多功能按键   2017
 *              5、单按多功能按键       2226
 *              6、单按触摸按键         3970
*******************************************************************************
*/
void get_key_task(void *pvParameters)
{	
	uint32_t voltage;	
	key_adc_voltage_init();	
	key_adc_voltage_config(KEY_ADC_CHL, 1);
//	printf("ADC get channel voltage...\n");	
	while (1){		
		voltage = key_adc_get();
		if((voltage > 0) && (voltage <= 750)){
		    g_strWifiResetKey.KeyPress     = 1;
		    g_strMultifunctionKey.KeyPress = 1;
			g_strTouchKey.KeyPress = 0;
		}
		else if((voltage>750) && (voltage <= 900)){
		    g_strWifiResetKey.KeyPress     = 1;
		    g_strMultifunctionKey.KeyPress = 0;
			g_strTouchKey.KeyPress = 0;
		}
		else if((voltage>900) && (voltage <= 2500)){
		    g_strWifiResetKey.KeyPress     = 0;
		    g_strMultifunctionKey.KeyPress = 1;
			g_strTouchKey.KeyPress = 0;
		}
		//有的米小克在上拉后，只有4053
		else if((voltage>3500) && (voltage <= 4048)){
		    g_strWifiResetKey.KeyPress     = 0;
		    g_strMultifunctionKey.KeyPress = 0;
			g_strTouchKey.KeyPress = 1;
		}
		else{
			g_strTouchKey.KeyPress = 0;
		    g_strWifiResetKey.KeyPress     = 0;
		    g_strMultifunctionKey.KeyPress = 0;
		}
//		printf("voltage = %d \n",voltage);
//		printf("g_strTouchKey.KeyPress = %d \n",g_strTouchKey.KeyPress);
//		printf("g_strTouchKey.ShortPressFlag = %d \n",g_strTouchKey.ShortPressFlag);
//		printf("g_strWifiResetKey.KeyPress = %d \n",     g_strWifiResetKey.KeyPress);
//		printf("g_strMultifunctionKey.KeyPress = %d \n", g_strMultifunctionKey.KeyPress);
//  
//		printf("time = %d\n", OS_GetTicks());
		KeyScan(g_arrkey,KEY_NUM);
		KeyClassify(g_arrkey,KEY_NUM);
		OS_MSleep(26);	
	}	
}

int GetKeyTaskCreat(void)
{
	if (OS_ThreadCreate(&get_key_task_handler,
						"get_key_task",
						get_key_task,
						NULL,
						OS_THREAD_PRIO_APP,
						KEY_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
		printf("GetKeyTaskCreat error\n");
		return -1;
	}
	return 0;
}


