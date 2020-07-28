#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common/framework/net_ctrl.h"
#include "kernel/os/os.h"
#include "net/wlan/wlan.h"
#include "lwip/sockets.h"
#include "lwip/netif.h"
#include "socket_connect.h"


#define SOCKET_TASK_STACK_SIZE		(2 * 1024)
#define LOCAL_PORT					10001
#define MTU_SIZE_MAX				1460

typedef struct Timer Timer;
struct Timer
{
	unsigned int end_time;
};

static OS_Thread_t g_task_ctrl_thread;
static uint8_t socket_revbuf[MTU_SIZE_MAX];
static int client_sock = -1;

/** countdown_ms - set timeout value in mil seconds
 * @param timer - timeout timer where the timeout value save
 * @param timeout_ms - timeout in timeout_ms mil seconds
 */
static void countdown_ms(Timer* timer, unsigned int timeout_ms)
{
	timer->end_time = OS_TicksToMSecs(OS_GetTicks()) + timeout_ms;
}

/** countdown - set timeout value in seconds
 * @param timer - timeout timer where the timeout value save
 * @param timeout - timeout in timeout seconds
 */
 /*
static void countdown(Timer* timer, unsigned int timeout)
{
	countdown_ms(timer, timeout * 1000);
}
*/

/** left_ms - calculate how much time left before timeout
 * @param timer - timeout timer
 * @return the time left befor timeout, or 0 if it has expired
 */
static int left_ms(Timer* timer)
{
	int diff = (int)(timer->end_time) - (int)(OS_TicksToMSecs(OS_GetTicks()));
	return (diff < 0) ? 0 : diff;
}

/** expired - has it already timeouted
 * @param timer - timeout timer
 * @return 0 if it has already timeout, or otherwise.
 */
static char expired(Timer* timer)
{
	return 0 <= (int)OS_TicksToMSecs(OS_GetTicks()) - (int)(timer->end_time); /* is time_now over than end time */
}

static int socket_tcp_create(int sock_type, short local_port, int do_bind)
{
	int local_sock;
	struct sockaddr_in local_addr;
	int ret, tmp;

	local_sock = socket(AF_INET, sock_type, 0);
	if (local_sock < 0) {
		SOCKET_MSG_ERR("socket() return %d\n", local_sock);
		return local_sock;
	}

	if (!do_bind)
		return local_sock;

	    tmp = 1;
	    ret = setsockopt(local_sock, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int));
	    if (ret != 0) {
		SOCKET_MSG_ERR("setsockopt(SO_REUSEADDR) failed, err \n");
		closesocket(local_sock);
		return -1;
	}


	if (sock_type == SOCK_STREAM) {
		tmp = 1;
		ret = setsockopt(local_sock, SOL_SOCKET, SO_KEEPALIVE, &tmp, sizeof(int));
		if (ret != 0) {
			SOCKET_MSG_ERR("setsockopt(SO_KEEPALIVE) failed, err \n");
			closesocket(local_sock);
			return -1;
		}
	}

	/* bind socket to port */
	memset(&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_addr.s_addr	= htonl(INADDR_ANY);
	local_addr.sin_port			= htons(local_port);
	local_addr.sin_family		= AF_INET;
	ret = bind(local_sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		SOCKET_MSG_ERR("Failed to bind socket %d, err \n", local_sock);
		closesocket(local_sock);
		return -1;
	}
	return local_sock;
}

static int socket_read(int socketfd, unsigned char *buffer, int len, int timeout_ms)
{
	int recvLen = 0;
	int leftms;
	int rc = -1;
	struct timeval tv;
	Timer timer;
	fd_set fdset;

	countdown_ms(&timer, timeout_ms);

	do {
		leftms = left_ms(&timer);
		tv.tv_sec = leftms / 1000;
		tv.tv_usec = (leftms % 1000) * 1000;

		FD_ZERO(&fdset);
		FD_SET(socketfd, &fdset);

		rc = select(socketfd + 1, &fdset, NULL, NULL, &tv);
		if (rc > 0) {
			rc = recv(socketfd, buffer + recvLen, len - recvLen, 0);
			if (rc > 0) {
				/* received normally */
				recvLen += rc;
			} else if (rc == 0) {
				/* has disconnected with server */
				recvLen = -1;
				break;
			} else {
				/* network error */
				SOCKET_MSG_ERR("recv return %d, errno = %d\n", rc, errno);
				recvLen = -2;
				break;
			}
		} else if (rc == 0) {
			if (recvLen != 0)
				SOCKET_MSG_ERR("received timeout and length had received is %d\n", recvLen);
			/* timeouted and return the length received */
			break;
		} else {
			/* network error */
			SOCKET_MSG_ERR("select return %d, errno = %d\n", rc, errno);
			recvLen = -2;
			break;
		}
	} while (recvLen < len && !expired(&timer)); /* expired() is redundant? */

	return recvLen;
}


static int socket_write(int socketfd, unsigned char* buffer, int len, int timeout_ms)
{
	int rc = -1;
	int sentLen = 0;
	fd_set fdset;
	struct timeval tv;

	FD_ZERO(&fdset);
	FD_SET(socketfd, &fdset);

	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;

	rc = select(socketfd + 1, NULL, &fdset, NULL, &tv);
	if (rc > 0) {
		if ((rc = send(socketfd, buffer, len, 0)) > 0)
			sentLen = rc;
		else if (rc == 0)
			sentLen = -1; /* disconnected with server */
		else {
			SOCKET_MSG_ERR("send return %d, errno = %d\n", rc, errno);
			sentLen = -2; /* network error */
		}
	} else if (rc == 0)
		sentLen = 0; /* timeouted and sent 0 bytes */
	 else {
	 	SOCKET_MSG_ERR("select return %d, errno = %d\n", rc, errno);
		sentLen = -2; /* network error */
	 }

	return sentLen;
}

void socket_task(void *arg)
{
	struct sockaddr_in remote_addr;
	char *remote_ip = REMOTE_IP;
	uint32_t remote_port = REMOTE_PORT;
	int ret;

	client_sock = socket_tcp_create(SOCK_STREAM, LOCAL_PORT, 0);
	if (client_sock < 0) {
		SOCKET_MSG_ERR("socket() return %d\n", client_sock);
	} else {
		SOCKET_MSG_DBG("socket create success, fd = %d\r\n", client_sock);
	}
	
	memset(socket_revbuf, 0, MTU_SIZE_MAX);
	
	memset(&remote_addr, 0, sizeof(struct sockaddr_in));
	remote_addr.sin_addr.s_addr = inet_addr(remote_ip);
	if (remote_port != 0)
		remote_addr.sin_port = htons(remote_port);
	else
		remote_addr.sin_port = htons(80);
	remote_addr.sin_family = AF_INET;
	
	//待修改:这里当网络发生连接错误时，需要有处理机制，如闪灯提示等
	ret = connect(client_sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
	if (ret < 0){
		SOCKET_MSG_ERR("connect to %s:%d return %d\n", remote_ip, remote_addr.sin_port, ret);
		goto socket_error;
	}
	
	while (1){
		int revlen;
		
		revlen = socket_read(client_sock, socket_revbuf, MTU_SIZE_MAX, 100);
		if (revlen > 0){
			socket_revbuf[revlen] = 0;
			socket_write(client_sock, socket_revbuf, revlen, 100);
			SOCKET_MSG_INFO("socket rec data = %s\r\n", socket_revbuf);
		} else if (revlen < 0){
			break;
		}
		OS_MSleep(100);
	}

socket_error:
	SOCKET_MSG_ERR("socket error exit!\r\n");
	socket_task_detele();
}


void socket_task_init(uint8_t *ssid, uint8_t ssid_len, uint8_t *psk)
{
	SOCKET_MSG_DBG("creat socket task init\r\n");

	g_task_ctrl_thread.handle = 0;
	net_switch_mode(WLAN_MODE_STA);
	wlan_sta_set(ssid, ssid_len, psk);
	wlan_sta_enable();

#if 0
	while (1) {
		SOCKET_MSG_DBG("net is connectting...\r\n");
		OS_Sleep(2);
		if (g_wlan_netif && netif_is_link_up(g_wlan_netif) && netif_is_up(g_wlan_netif))
			break;
		SOCKET_MSG_DBG("g_wlan_netif = 0x%x, netif_is_link_up = %d, netif_is_up = %d\r\n", (uint32_t)g_wlan_netif, netif_is_link_up(g_wlan_netif), netif_is_up(g_wlan_netif));
	}
#endif
}

int socket_task_create(void)
{
	if (g_task_ctrl_thread.handle) {
		SOCKET_MSG_WRN("socket_task is already\r\n");
		return -2;
	}
	
	if (OS_ThreadCreate(&g_task_ctrl_thread,
						"socket_task",
						socket_task,
						NULL,
						OS_THREAD_PRIO_APP,
						SOCKET_TASK_STACK_SIZE) != OS_OK) {
		SOCKET_MSG_ERR("socket thread create error\n");
		return -1;
	}
	
	SOCKET_MSG_INFO("socket create task success\r\n");
	return 0;

}

void socket_task_detele(void)
{
	if (client_sock >= 0) {
		closesocket(client_sock);
		client_sock = -1;
	}
	if (g_task_ctrl_thread.handle) {
		OS_ThreadDelete(&g_task_ctrl_thread);
		g_task_ctrl_thread.handle = 0;
	}
	SOCKET_MSG_INFO("socket_task_deinit\r\n");
}

