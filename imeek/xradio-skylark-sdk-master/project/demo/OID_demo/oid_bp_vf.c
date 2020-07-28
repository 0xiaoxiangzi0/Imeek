
#include <stdio.h>
#include "kernel/os/os.h"
#include "driver/chip/hal_adc.h"
#include "driver/chip/private/hal_os.h"
#include "driver/chip/hal_wakeup.h"
#include "pm/pm.h"
#include "oid_bp_vf.h"
#include "oid_audio.h"
#include "app.h"

#define ADC_BURST_MODE			(0)
#define ADC_IRQ_MODE 			ADC_IRQ_DATA
#define ADC_CHL 				ADC_CHANNEL_VBAT
#define ADC_FEQ 				500000
#define ADC_FIRST_DELAY 		10

HAL_Semaphore voltage_sem;
#define ADC_VOL_SEM_INIT()		HAL_SemaphoreInitBinary(&voltage_sem)
#define ADC_VOL_SEM_WAIT()		HAL_SemaphoreWait(&voltage_sem, 10000)
#define ADC_VOL_SEM_RELEASE()	HAL_SemaphoreRelease(&voltage_sem)
#define ADC_VOL_SEM_DEINIT()	HAL_SemaphoreDeinit(&voltage_sem)

static uint8_t adc_value_status = 1;
static uint32_t adc_data[10];
static void adc_callback(void *arg)
{	
    HAL_Status status = HAL_OK;
	if (adc_value_status)		
		return;	
	ADC_Channel chl = *((ADC_Channel*)arg);	
	static uint32_t count = 0;
#if ADC_BURST_MODE	
    for (count = 0; count < 10; count++)		
		adc_data[count++] = HAL_ADC_GetFifoData();	
	status = HAL_ADC_FifoConfigChannel(chl, ADC_SELECT_DISABLE);	
	adc_value_status = 1;	
	ADC_VOL_SEM_RELEASE();
#else	
    if (HAL_ADC_GetIRQState(chl) == ADC_DATA_IRQ) {		
		if (count >= 10) {			
			status = HAL_ADC_ConfigChannel(chl, ADC_SELECT_DISABLE, ADC_IRQ_MODE, 0, 0);			
			count = 0;			
			adc_value_status = 1;			
			ADC_VOL_SEM_RELEASE();		
		} else			
		adc_data[count++] = HAL_ADC_GetValue(chl);	
	}
#endif	
    if (status != HAL_OK)		
		printf("ADC disconfig error--- %d\n", status);
}

static void adc_init()
{	
    HAL_Status status = HAL_ERROR;	
	ADC_InitParam initParam;	
	initParam.delay = ADC_FIRST_DELAY;	
	initParam.freq = ADC_FEQ;
#if (__CONFIG_CHIP_ARCH_VER == 2)	
    initParam.vref_mode = 1;
#endif	
    printf("ADC init...\n");
#if ADC_BURST_MODE	
    initParam.mode = ADC_BURST_CONV;
#else	
    initParam.mode = ADC_CONTI_CONV;
#endif	
    status = HAL_ADC_Init(&initParam);
    if (status != HAL_OK) {		
		printf("ADC init error %d\n", status);		
		return;
	}
}

static void adc_deinit()
{	
    HAL_Status status = HAL_ERROR;	
	status = HAL_ADC_DeInit();	
	if (status != HAL_OK)		
		printf("ADC deinit error %d\n", status);
}

static uint32_t adc_filter(uint32_t adc_data[10])
{	
    uint8_t j = 0, i = 0;	
	uint32_t sum = 0;	
	uint32_t max = 0;	
	for (i = 0; i < 10; i++) {	 
		for (j = i; j < 9; j++) {		 
			if (adc_data[i] >= adc_data[j+1]) {			 
				max = adc_data[i];			 
				adc_data[i] = adc_data[j+1];			 
				adc_data[j+1] = max;		 
			}	 
		}	
	}	
	for (i = 0; i < 10; i++) {	 
		sum = sum + adc_data[i];	
	}	
	sum = (sum - adc_data[0] - adc_data[9]) / 8;	
	return sum;
}

static void adc_voltage_config(uint8_t channel, uint8_t en)
{	
    HAL_Status status = HAL_ERROR;	
    ADC_Channel chl = (ADC_Channel)channel;	
	if (en) {		
		printf("ADC channel config...\n");
#if ADC_BURST_MODE		
        status = HAL_ADC_FifoConfigChannel(chl, ADC_SELECT_ENABLE);
#else		
        status = HAL_ADC_ConfigChannel(chl, ADC_SELECT_ENABLE, ADC_IRQ_MODE, 0, 0);
#endif		
        if (status != HAL_OK) {			
		    printf("ADC config error %d\n", status);			
		    return;		
	    }		
	    printf("ADC callback enable...\n");		
	    status = HAL_ADC_EnableIRQCallback(chl, adc_callback, &chl);		
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

void adc_voltage_init(void)
{
	ADC_VOL_SEM_INIT();	
	adc_init();
}

/*
  * The relationship between adc value and voltage conversion is as follows: 
  * voltage = adc_value * 2500 * ratio / 4096(mv), ratio=1 when adc channel is chl0~chl6, 
  * 			ratio=3 when adc channel is chl8, 
  */
uint32_t adc_voltage_get(void)
{	
    HAL_Status status = HAL_ERROR;	
	adc_value_status = 0;	
	ADC_Channel chl = (ADC_Channel)ADC_CHL;	
	uint8_t ratio = (chl == ADC_CHANNEL_VBAT) ? 3 : 1;
#if ADC_BURST_MODE	
    status = HAL_ADC_FifoConfigChannel(chl, ADC_SELECT_ENABLE);
#else	
    status = HAL_ADC_ConfigChannel(chl, ADC_SELECT_ENABLE, ADC_IRQ_MODE, 0, 0);
#endif	
    if (status != HAL_OK) {		
		printf("ADC it mode start error %d\n", status);	
	}	
	ADC_VOL_SEM_WAIT();	
	if (adc_value_status) {		
		uint32_t adc_value = adc_filter(adc_data);		
		return (adc_value * 2500 * ratio / 4096);	
		return adc_value;	
	}
	return 0;
}

/*
*******************************************************************************
 *Function:     LowBatteryGotoStandby
 *Description:  电量低进入待机模式，网络可以正常通信但是其他电源会被关闭
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:      
*******************************************************************************
*/
static void LowBatteryGotoStandby(void)
{
	HAL_Wakeup_SetTimer_Sec(15000);

	pm_enter_mode(PM_MODE_STANDBY);
}

//static void adc_voltage_deinit()
//{	
//    adc_deinit();	
//	ADC_VOL_SEM_DEINIT();
//}

int adc_get(void)
{	
	uint32_t i;
    static MYBOOL l_u8AdcFlag = TRUE;
	char *l_pu8Path = "file://music/Public/_50.mp3";
	uint32_t l_au32Voltage[8] = {3700,3700,3700,3700,
	                             3700,3700,3700,3700};
	uint8_t l_u8VolIndex = 0; 
	uint32_t l_u32Voltage = 0;
	
	adc_voltage_init();	
	adc_voltage_config(ADC_CHL, 1);
	while(1){
		OS_Sleep(5);
		l_u8VolIndex = (l_u8VolIndex+1) % 8;
		l_au32Voltage[l_u8VolIndex] = adc_voltage_get();
		for(i = 0; i<8; i++){
			l_u32Voltage += l_au32Voltage[i];
		}
		l_u32Voltage >>= 3;
//		printf("VBAT voltage: %dmV\n", l_u32Voltage);
		if(l_u32Voltage < 3300){
            //这里要进入睡眠模式            
			set_imeek_light_struct(LIGHT_OFF,LIGHT_RED,LIGHT_FORM,LIGHT_NO_CONTINUE,LIGHT_FOREVER,
								   LIGHT_OFF,LIGHT_RED,LIGHT_FORM,LIGHT_NO_CONTINUE,LIGHT_FOREVER);
			OS_Sleep(2);
            LowBatteryGotoStandby();
		}
		
		//一次开机只进行一次低电量报警，voltage已经转换为相应的电压值，单位mV
		if((l_u32Voltage < 3450) && (l_u8AdcFlag)){
			l_u8AdcFlag = FALSE;
			PlayAudio(l_pu8Path,FIRST_LEVEL_AUDIO,IMEEK_PLAY);
			set_imeek_light_struct(LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_NO_CONTINUE,2,
								   LIGHT_ON,LIGHT_RED,LIGHT_BRIGHT,LIGHT_CONTINUE,2);
		}
		l_u32Voltage = 0;
	}	
	return 0;
}

