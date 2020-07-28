#include <stdio.h>
#include <string.h>

#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"

#include "kernel/os/os.h"
#include "common/framework/net_ctrl.h"
#include "common/framework/platform_init.h"

#include "smartlink/sc_assistant.h"
#include "smartlink/airkiss/wlan_airkiss.h"
#include "oid_airkiss.h"

#define AK_TIME_OUT_MS 20000 //120s

static char airkiss_key[17] = "pd7rHfYO9h60agU1"; // airkiss aes key

void oid_airkiss_link(STRUCT_SSID_PWD *l_ssid_pwd)
{
	wlan_airkiss_status_t ak_status;
	wlan_airkiss_result_t ak_result;
	sc_assistant_fun_t sca_fun;
	sc_assistant_time_config_t config;

	memset(&ak_result, 0, sizeof(wlan_airkiss_result_t));

	/*
	 * Initialize the distribution network assistant,
	 * which can improve the success rate of distribution network
	 */
	sc_assistant_get_fun(&sca_fun);
	config.time_total = AK_TIME_OUT_MS;
	config.time_sw_ch_long = 400;
	config.time_sw_ch_short = 100;
	sc_assistant_init(g_wlan_netif, &sca_fun, &config);

	/* start airkiss */
	ak_status = wlan_airkiss_start(g_wlan_netif, airkiss_key);
//	ak_status = wlan_airkiss_start(g_wlan_netif, NULL);
	if (ak_status != WLAN_AIRKISS_SUCCESS) {
		printf("airkiss start fiald!\n");
		l_ssid_pwd->flag = SSID_PWD_NOVALID;
		goto out;
	}

	printf("%s getting ssid and psk...\n", __func__);

	/* Blocking waiting for distribution network to complete */
	if (wlan_airkiss_wait(AK_TIME_OUT_MS) == WLAN_AIRKISS_TIMEOUT) {
		printf("%s get ssid and psk timeout\n", __func__);
		l_ssid_pwd->flag = SSID_PWD_NOVALID;
		goto out;
	}
	printf("%s get ssid and psk finished\n", __func__);

	/* if complete, then get the airkiss result */
	if (wlan_airkiss_get_status() == AIRKISS_STATUS_COMPLETE && \
	    wlan_airkiss_get_result(&ak_result) == WLAN_AIRKISS_SUCCESS) {
	    l_ssid_pwd->flag = SSID_PWD_VALID;
		strcpy((char *)l_ssid_pwd->ssid, (char *)ak_result.ssid);
		strcpy((char *)l_ssid_pwd->pwd, (char *)ak_result.passphrase);
		printf("ssid:%s psk:%s random:%d\n", (char *)ak_result.ssid,
		       (char *)ak_result.passphrase, ak_result.random_num);

		/* if get result, then connect ap and send the airkiss connect ack */
		if (!wlan_airkiss_connect_ack(g_wlan_netif, AK_TIME_OUT_MS, &ak_result)) {
			printf("connect and ack ok\n");
		}
	}

out:
	/* stop airkiss and deinit distribution network assistant */
	wlan_airkiss_stop();
	sc_assistant_deinit(g_wlan_netif);
}
