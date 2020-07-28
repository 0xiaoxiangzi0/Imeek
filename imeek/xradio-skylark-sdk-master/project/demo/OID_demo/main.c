/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
     */

#include "common/framework/platform_init.h"
#include <stdio.h>
#include <string.h>
#include "kernel/os/os.h"
#include "net/wlan/wlan.h"
#include "oid_transf.h"
#include "oid_protocol.h"
#include "rgb_led.h"
#include "net_msg.h"
#include "socket_connect.h"
#include "oid_airkiss.h"
#include "app.h"
#include "oid_audio.h"
#include "oid_flash.h"
#include "imeek_ota.h"
#include "oid_network_connect.h"
#include "imeek_key.h"
#include "imeek_md5.h"
#include "imeek_file.h"
#include "imeek_http.h"
#include "imeek_record.h"
#include "stepper_motor.h"

extern STRUCT_SSID_PWD ssid_pwd;
extern void GetFlashState(STRUCT_FLASH_STATE* FlashState,STRUCT_IMEEK_ID* ImeekId,
                          STRUCT_SSID_PWD *SsidPwd,STRUCT_ALL_AUDIO_UPDATE_DATA *AllAudioVersion);
//extern int MotorTaskCreate(void);

int main(void)
{
	platform_init();

	ImeekAudioPlayStart();
	//GetFlashState函数要操作SD卡必须放在mount后
	GetFlashState(&g_strFlashState,&g_strImeekId,&ssid_pwd,g_aAudioUpdateData);
	//LightTaskCreate函数必须放在GetFlashState之后，因为关系到开机时亮灯情况
	LightTaskCreate();
	OS_Sleep(6);
	//OID模组在初始化过程中，不能做其他任务，否则可能导致OID模组初始化失败，那么本次开机就无法使用了
	OidDetectionTaskCreate();
	GetOtaMode(&g_strOtaMode);
	GetSsidPwdTaskCreate();
	network_task_create();
	http_task_create();
	//用SD卡方式OTA时只能有播放音频的函数开启，其他开启会导致OTA失败
	OtaTaskCreate();
	WaitOtaEnd();
	RecordTaskCreate();
	AppTaskCreate();
	TrailingModeCreate();
	GetKeyTaskCreat();
	MotorTaskCreate();
	return 0;
}
