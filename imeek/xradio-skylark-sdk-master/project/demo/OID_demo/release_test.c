//#include <stdio.h>
//#include <string.h>
//#include "kernel/os/os.h"
//#include "driver/chip/hal_uart.h"
//#include "release_test.h"

//#define TEST_TASK_CTRL_THREAD_STACK_SIZE (1 * 1024)
//static OS_Thread_t TestModeHandler;

//#define IMEEEK_UARTID UART1_ID
//MYBOOL TestModeFlag = FALSE;
///*
//*******************************************************************************
// *Function:     UartInit
// *Description:  ´®¿Ú³õÊ¼»¯
// *Calls:        
// *Called By:   
// *Input:        
// *Output:       
// *Return:       
// *Others:       
//*******************************************************************************
//*/
//int UartInit(void)
//{
//	HAL_Status status = HAL_ERROR;
//	UART_InitParam param;

//	param.baudRate = 115200;
//	param.dataBits = UART_DATA_BITS_8;
//	param.stopBits = UART_STOP_BITS_1;
//	param.parity = UART_PARITY_NONE;
//	param.isAutoHwFlowCtrl = 0;

//	status = HAL_UART_Init(IMEEEK_UARTID, &param);
//	if (status != HAL_OK)
//		printf("uart init error %d\n", status);
//	return status;
//}

/**@bref Interrupt Mode:use interrupt receive & transmit data**/
//void UartItMode(void)
//{
//    int     i = 10;
//	int     len;
//	uint8_t rx_data;
//	char    buffer[20];
//	uint8_t len_str;

//	printf("uart%d it mode\n", UARTID);
//	len_str = snprintf(buffer, 20, "uart%d it mode.\n", UARTID);
//	HAL_UART_Transmit_IT(UARTID, (uint8_t *)buffer, len_str);

//	while(i--){
//		len = HAL_UART_Receive_IT(UARTID,	/*uartID*/
//								  &rx_data,	/*buf Pointer to the data buffer*/
//								  1,		/*size The maximum number of bytes to be received*/
//								  10000);	/*Timeout value in millisecond to receive data, HAL_WAIT_FOREVER for no timeout*/
//		if((len == 1) && (rx_data == 0xFA)){
//			TestModeFlag = TRUE;

//			len_str = snprintf(buffer, 20, "into test mode.\n", UARTID);
//			HAL_UART_Transmit_IT(UARTID, (uint8_t *)buffer, len_str);
//			break;
//		}
//		OS_MSleep(20);
//	}
//}

//void UartItModeDeinit(void)
//{
//	HAL_Status status = HAL_ERROR;

//	status = HAL_UART_DisableRxCallback(UARTID);
//	if (status != HAL_OK)
//		printf("uart RX disenable callback error %d\n", status);

//	status = HAL_UART_DeInit(UARTID);
//	if (status != HAL_OK)
//		printf("uart deinit error %d\n", status);
//}

/*Run this example, please connect the uart0 and uart 1*/
//int main(void)
//{
//	printf("uart example started.\n\n");
//	printf("uart%d will be used for echo.\n", UARTID);

//	printf("uart example over.\n");

//	return 0;
//}

//void TestMode(void *pvParameters)
//{
//	UartInit();
//	UartItMode();
//	UartItModeDeinit();
//	OS_ThreadDelete(&TestModeHandler);
//}

//int TestModeCreate(void)
//{
//	if (OS_ThreadCreate(&TestModeHandler,
//						"TestMode",
//						TestMode,
//						NULL,
//						OS_THREAD_PRIO_APP,
//						TEST_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
//		APP_MSG_DBG("Test thread create error\n");
//		return -1;
//	}
//	
//	return 0;
//}


