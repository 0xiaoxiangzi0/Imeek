#include "wlan_msg.h"
#include "FreeRTOS.h"
#include "kernel/FreeRTOS/event_groups.h"
#include "kernel/os/os.h"
#include "net/wlan/wlan_defs.h"
#include "common/framework/sys_ctrl/sys_ctrl.h"
#include "common/framework/net_ctrl.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"
#include "net/udhcp/usr_dhcpd.h"
#include "net/wlan/wlan.h"
#include "lwip/sockets.h"
#include "lwip/ip.h"


//#define OID_SERVICE_TASK_CTRL_THREAD_STACK_SIZE  (2 * 1024)
//static OS_Thread_t wlan_msg_handler;

//void wlan_msg_task(void *pvParameters)
//{
//    int sock,ret;
//	struct sockaddr_in serv_addr;
//	char wlan_data[10] = {1,2,3,4,5,6,7,8,9,10};
//	char buffer[40];		
//	
//	while(1){
//	    sock = socket(AF_INET, SOCK_STREAM, 0);
//		if (sock < 0){
//			WLAN_MSG_DBG("socket create defeat\n");
//		}
//		else{
//			WLAN_MSG_DBG("socket create success\n");
//		}
//		
//		memset(&serv_addr, 0, sizeof(serv_addr));  
//		serv_addr.sin_family = AF_INET;                      
//        serv_addr.sin_addr.s_addr = inet_addr("192.168.52.1");  
//        serv_addr.sin_port = htons(8087);  

//        ret = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
//		if (ret < 0){
//			WLAN_MSG_DBG("socket connect defeat\n");
//		}
//		else{
//			WLAN_MSG_DBG("socket connect success\n");
//		}
//        
//		ret = send(sock, wlan_data, 10, 0);
//		if (ret < 0){
//			WLAN_MSG_DBG("socket send defeat\n");
//		}
//		else{
//			WLAN_MSG_DBG("socket send success\n");
//		}

//        ret = read(sock, buffer, sizeof(buffer)-1);
//		if (ret < 0){
//			WLAN_MSG_DBG("socket read defeat\n");
//		}
//		else{
//			WLAN_MSG_DBG("socket read success\n");
//		}
//		
//		close(sock);
//		
//		OS_Sleep(10);
//	}
//}

//int wlan_tread_create(void)
//{
//    if (OS_ThreadCreate(&wlan_msg_handler,
//						"wlan_msg_task",
//						wlan_msg_task,
//						NULL,
//						OS_THREAD_PRIO_APP,
//						OID_SERVICE_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
//		WLAN_MSG_INF("WLAN service thread create error\n");
//		return -1;
//	}
//	return 0;
//}

