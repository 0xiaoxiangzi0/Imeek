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
#include "net_msg.h"
#include "socket_connect.h"
#include "oid_audio.h"
#include "oid_network_connect.h"
#include "imeek_http.h"

#include "smartlink/sc_assistant.h"
#include "smartlink/airkiss/wlan_airkiss.h"


//#define DEVICE_TYPE "gh_a12b34cd567e"
//#define DEVICE_ID   "0080e129e8d1"


#define NET_SERVICE_TASK_CTRL_THREAD_STACK_SIZE		(1 * 1024)

#define NET_EVEVT_CONNECTED							(1 << 0)
#define NET_EVEVT_DISCONNECTED						(1 << 1)
#define NET_EVEVT_SCAN_SUCCESS						(1 << 2)
#define NET_EVEVT_SCAN_FAILED						(1 << 3)
#define NET_EVEVT_4WAY_HANDSHAKE_FAILED				(1 << 4)
#define NET_EVEVT_CONNECT_FAILED					(1 << 5)
#define NET_EVEVT_CONNECT_LOSS						(1 << 6)
#define NET_EVEVT_NETWORK_UP						(1 << 7)
#define NET_EVEVT_NETWORK_DOWN						(1 << 8)


static OS_Thread_t net_service_handler;
static EventGroupHandle_t xCreatedEventGroup;
extern EventGroupHandle_t net_event_group;


void net_event_ob(uint32_t event, uint32_t data, void *arg)
{	
	uint16_t type;

	type = EVENT_SUBTYPE(event);
	NET_MSG_INF("%s msg (%u, %u)\n", __func__, type, data);

	switch (type) {
		case NET_CTRL_MSG_WLAN_CONNECTED:
			xEventGroupSetBits(xCreatedEventGroup, NET_EVEVT_CONNECTED);
			NET_MSG_INF("NET_CTRL_MSG_WLAN_CONNECTED\n");
			break;
		case NET_CTRL_MSG_WLAN_DISCONNECTED:
			xEventGroupSetBits(xCreatedEventGroup, NET_EVEVT_DISCONNECTED);
			NET_MSG_INF("NET_CTRL_MSG_WLAN_DISCONNECTED\n");
			break;
		case NET_CTRL_MSG_WLAN_SCAN_SUCCESS:
			xEventGroupSetBits(xCreatedEventGroup, NET_EVEVT_SCAN_SUCCESS);
			NET_MSG_INF("NET_CTRL_MSG_WLAN_SCAN_SUCCESS\n");
			break;
		case NET_CTRL_MSG_WLAN_SCAN_FAILED:
			xEventGroupSetBits(xCreatedEventGroup, NET_EVEVT_SCAN_FAILED);
			NET_MSG_INF("NET_CTRL_MSG_NETWORK_DOWN\n");
			break;
		case NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED:
			xEventGroupSetBits(xCreatedEventGroup, NET_EVEVT_4WAY_HANDSHAKE_FAILED);
			NET_MSG_INF("NET_CTRL_MSG_NETWORK_DOWN\n");
			break;
		case NET_CTRL_MSG_WLAN_CONNECT_FAILED:
			xEventGroupSetBits(xCreatedEventGroup, NET_EVEVT_CONNECT_FAILED);
			NET_MSG_INF("NET_CTRL_MSG_WLAN_CONNECT_FAILED\n");
			break;
		case NET_CTRL_MSG_CONNECTION_LOSS:
			xEventGroupSetBits(xCreatedEventGroup, NET_EVEVT_CONNECT_LOSS);
			NET_MSG_INF("NET_CTRL_MSG_CONNECTION_LOSS\n");
			break;
		case NET_CTRL_MSG_NETWORK_UP:  
			xEventGroupSetBits(xCreatedEventGroup, NET_EVEVT_NETWORK_UP);
			NET_MSG_INF("NET_CTRL_MSG_NETWORK_UP\n");
			break;
		case NET_CTRL_MSG_NETWORK_DOWN:
			xEventGroupSetBits(xCreatedEventGroup, NET_EVEVT_NETWORK_DOWN);
			NET_MSG_INF("NET_CTRL_MSG_NETWORK_DOWN\n");
			break;
#if (!defined(__CONFIG_LWIP_V1) && LWIP_IPV6)
		case NET_CTRL_MSG_NETWORK_IPV6_STATE:
			break;
#endif
		default:
			NET_MSG_INF("unknown msg (%u, %u)\n", type, data);
			break;
	}
}


int net_service_init(void)
{
	observer_base *ob;

	ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK,
									  NET_CTRL_MSG_ALL,
									  net_event_ob,
									  NULL);
	if (ob == NULL)
		return -1;
	if (sys_ctrl_attach(ob) != 0)
		return -1;
		
	return 0;
}

int net_event_create(void)
{
	xCreatedEventGroup = xEventGroupCreate();
	if(xCreatedEventGroup == NULL) {
		NET_MSG_INF("net evevt create failed!\r\n");
		return -1;
	}

	return 0;
}


void net_service_task(void *pvParameters)
{
	EventBits_t uxBits;
	
	while (1) {
		uxBits = xEventGroupWaitBits(xCreatedEventGroup,				\
 									 NET_EVEVT_CONNECTED |				\
 									 NET_EVEVT_DISCONNECTED |			\
 									 NET_EVEVT_NETWORK_UP |				\
 									 NET_EVEVT_NETWORK_DOWN |			\
 									 NET_EVEVT_CONNECT_FAILED |			\
 									 NET_EVEVT_CONNECT_LOSS |			\
 									 NET_EVEVT_4WAY_HANDSHAKE_FAILED,	\
 									 pdTRUE,							\
 									 pdFALSE,							\
 									 10);
		     //与路由器连接成功,但是并不能说明路由器联网成功
		if ((uxBits & NET_EVEVT_CONNECTED) != 0){
			NET_MSG_INF("%s, recv net evevt connected happen!\r\n", __func__);

			//与路由器连接失败,该事件会在路由器账号密码错误时或路由器不存在时产生
		} else if ((uxBits & NET_EVEVT_DISCONNECTED) != 0){
			NET_MSG_INF("%s, recv net evevt disconnected happen!\r\n", __func__);
			xEventGroupSetBits(net_event_group, NET_CONNECT_FAIL);
//			socket_task_detele();

			//网络连接成功
		} else if ((uxBits & NET_EVEVT_NETWORK_UP) != 0) {
			NET_MSG_INF("%s, recv net evevt netup happen!\r\n", __func__);
			xEventGroupSetBits(net_event_group, NET_CONNECT_SUCCESS);
//			http_task_create();
//			socket_task_create();

            //局域网发现功能
//			wlan_airkiss_lan_discover_start(DEVICE_TYPE, DEVICE_ID, 1000);
//			OS_MSleep(30000);
//			wlan_airkiss_lan_discover_stop();
			
//            ImeekAudioPlayStart();
			
			//网络断开
		} else if ((uxBits & NET_EVEVT_NETWORK_DOWN) != 0) {
			NET_MSG_INF("%s, recv net evevt netdown happen!\r\n", __func__);
			xEventGroupSetBits(net_event_group, NET_DOWN);
//			socket_task_detele();

		} else if ((uxBits & NET_EVEVT_CONNECT_FAILED) != 0) {
			NET_MSG_INF("%s, recv net evevt connect failed happen!\r\n", __func__);

			//路由器断开事件
		} else if ((uxBits & NET_EVEVT_CONNECT_LOSS) != 0) {
			NET_MSG_INF("%s, recv net evevt connect loss happen!\r\n", __func__);
			//XR872与路由器4次握手失败，说明连接路由器出现问题
		} else if ((uxBits & NET_EVEVT_4WAY_HANDSHAKE_FAILED) != 0) {
			NET_MSG_INF("%s, recv net evevt handshake failed happen!\r\n", __func__);
		} 
	}
}

int net_service_task_create(void)
{
	net_event_create();
	net_service_init();
	
	if (OS_ThreadCreate(&net_service_handler,
						"net_service_task",
						net_service_task,
						NULL,
						OS_THREAD_PRIO_APP,
						NET_SERVICE_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
		NET_MSG_INF("ap websever thread create error\n");
		return -1;
	}

	NET_MSG_INF("net service thread create success, handler = 0x%x\n", (uint32_t)net_service_handler.handle);
	
	return 0;
}
