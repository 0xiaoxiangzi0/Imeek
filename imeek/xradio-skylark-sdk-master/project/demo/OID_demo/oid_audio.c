/*
 *oid_audio.c文 件 共 完 成 以 下 一 些 内 容:
 *
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include "kernel/os/os.h"
#include "common/apps/player_app.h"
#include "common/framework/fs_ctrl.h"
#include "common/framework/platform_init.h"
#include "fs/fatfs/ff.h"
#include "kernel/os/os_mutex.h"

#include "audiofifo.h"
#include "oid_audio.h"
#include "imeek_ota.h"
#include "app.h"

#define OID_PLAYER_THREAD_STACK_SIZE    (1024 * 2)

STRUCT_PLAY_AUDIO OidPlayAudio = {IMEEK_PLAY,
                                    IMEEK_PLAY,
                                    FALSE,
                                    THIRD_LEVEL_AUDIO,
                                    NEW_PLAY_AUDIO,
#if (OID_PLAY_MODE == FS_AUDIO_PLAY)
		                            "file://music/Public/_30.mp3"
#elif (OID_PLAY_MODE == HTTP_AUDIO_PLAY)
		                            "http://api.linxbot.com/tmp/456.mp3"
#endif
                                    };

extern player_base *player_create();

static OS_Thread_t oid_play_thread = {0};
static player_base *oid_player;
static int isCompleted = 0;

OS_Mutex_t AudioPlayMutex;

static void audio_play_gpio_init(void)
{
	GPIO_InitParam param;
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F1_OUTPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(AUDIO_PLAY_PORT, AUDIO_PLAY_PIN, &param);
    
	AUDIO_PLAY_OFF();
}

static void oid_player_callback(player_events event, void *data, void *arg)
{
    switch (event) {
    case PLAYER_EVENTS_MEDIA_PREPARED:
        OID_PLAY_MSG_DBG("media is prepared. play media now.\n");
        OID_PLAY_MSG_DBG("you can use player to seek, get duration(by size), get current position(by tell) from now on.\n");
        break;
    case PLAYER_EVENTS_MEDIA_STOPPED:
        OID_PLAY_MSG_DBG("media is stopped by user.\n");
        isCompleted = 1;
        break;
    case PLAYER_EVENTS_MEDIA_ERROR:
        OID_PLAY_MSG_DBG("error occur\n");
        isCompleted = 1;
        break;
    case PLAYER_EVENTS_MEDIA_PLAYBACK_COMPLETE:
        OID_PLAY_MSG_DBG("media play is completed\n");
        isCompleted = 1;
        break;
    default:
        break;
    }
}

/*
*******************************************************************************
 *Function:     WaitAudioPlayOver
 *Description:  等待音频播放完成
 *Calls:       
 *Called By:   
 *Input:       flag:TRUE表示需要等待音频播放完成
 *                  FALSE表示不需要等待音频播放完成
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void WaitAudioPlayOver(MYBOOL flag)
{
    if(TRUE == flag){
        do{ 
		    OS_MSleep(200);
		}while(IMEEK_PLAY == OidPlayAudio.CurrentState);
	}
}

/*
*******************************************************************************
 *Function:     AudioPlayToEnd
 *Description:  播放音频阻塞到播放完成
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void AudioPlayToEnd(char *url,ENUM_PLAY_AUDIO_PRIORITY priority,ENUM_PLAY_TYPE play_type)
{
    MYBOOL l_bWaitPlayFlag = TRUE;
	PlayAudio(url,priority,play_type);
	WaitAudioPlayOver(l_bWaitPlayFlag);
}

/*
*******************************************************************************
 *Function:     PlayAudio
 *Description:  根据文件名播放音频
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       0:播放成功
 *              1:正在播放同名音频
 *              2:正在播放音频优先级更高
 *              3:暂停成功
 *Others:       
*******************************************************************************
*/
uint8_t PlayAudio(char *name, ENUM_PLAY_AUDIO_PRIORITY priority,ENUM_PLAY_TYPE play_type)
{
    int ret = 0xFF;
	switch(play_type){
		case IMEEK_PLAY:
			if(IMEEK_PLAY == OidPlayAudio.CurrentState){
				if(priority <= OidPlayAudio.Priority){
					if(strcmp(OidPlayAudio.play_audio_url_old,name)){
						sprintf(OidPlayAudio.play_audio_url_new,"%s",name);
						OidPlayAudio.Priority = priority;
						OidPlayAudio.new_play_audio_flag = NEW_PLAY_AUDIO;
						OidPlayAudio.NewState = IMEEK_NO_STATE;
						ret = 0;
					}
					else{
						//同名文件
						OidPlayAudio.NewState = IMEEK_NO_STATE;
						ret = 1;
					}
				}
				else{
					//优先级低
					OidPlayAudio.NewState = IMEEK_NO_STATE;
					ret = 2;
				}
			}
			else if(IMEEK_PAUSE == OidPlayAudio.CurrentState){
				if(strcmp(OidPlayAudio.play_audio_url_old,name)){
					sprintf(OidPlayAudio.play_audio_url_new,"%s",name);
					OidPlayAudio.Priority = priority;
					OidPlayAudio.new_play_audio_flag = NEW_PLAY_AUDIO;
					OidPlayAudio.NewState = IMEEK_NO_STATE;
					ret = 3;
				}
				else{
					//同名文件
					OidPlayAudio.NewState = IMEEK_PLAY;
					ret = 4;
				}
			}
			else{
				sprintf(OidPlayAudio.play_audio_url_new,"%s",name);
				OidPlayAudio.Priority = priority;
				OidPlayAudio.new_play_audio_flag = NEW_PLAY_AUDIO;
				OidPlayAudio.NewState = IMEEK_NO_STATE;
				ret = 5;
			}
			break;
		case IMEEK_PAUSE:
			if(IMEEK_PLAY == OidPlayAudio.CurrentState){
                OidPlayAudio.NewState = IMEEK_PAUSE;
				ret = 6;
			}
			else{
                ret = 7;
			}
			break;
		case IMEEK_STOP:
			if((IMEEK_PAUSE == OidPlayAudio.CurrentState)
			|| (IMEEK_PLAY == OidPlayAudio.CurrentState)){
                OidPlayAudio.NewState = IMEEK_STOP;
				ret = 8;
			}
			else{ 
                ret = 9;
			}
			break;
		default:
			break;
	}
	return ret;
}

static int oid_play_file_music()
{
    static uint32_t last_volume = 0;
    int ret;
	
    /*
     * play media in sd/tf card.
     * 1. in this example, you should create a folder named music in you sd/tf card
     * 2. add 1.mp3 to this folder
     */
    while(1){
		if((NEW_PLAY_AUDIO == OidPlayAudio.new_play_audio_flag) 
		&& (ota_end_flag != OTA_CONTINUE)){
		    OidPlayAudio.CurrentState = IMEEK_PLAY;
			OidPlayAudio.new_play_audio_flag = NO_PLAY_AUDIO;
			strcpy(OidPlayAudio.play_audio_url_old,
				   OidPlayAudio.play_audio_url_new);
			isCompleted = 0;
			oid_player->set_callback(oid_player, oid_player_callback, NULL);
			
			ret = oid_player->play(oid_player, OidPlayAudio.play_audio_url_old);
			if (ret != 0){
				OID_PLAY_MSG_DBG("music play fail.\n");
				printf("music play fail: %d\n",ret);
				return -1;
			}
			
			/* wait for playback complete */
			while ((!isCompleted) && (NO_PLAY_AUDIO == OidPlayAudio.new_play_audio_flag)){
				if((IMEEK_PAUSE == OidPlayAudio.CurrentState)
				&& (IMEEK_PLAY == OidPlayAudio.NewState)){
					OidPlayAudio.CurrentState = IMEEK_PLAY;
                    oid_player->resume(oid_player);
				}
				
				if((IMEEK_PLAY == OidPlayAudio.CurrentState)
				&& (IMEEK_PAUSE == OidPlayAudio.NewState)){
					OidPlayAudio.CurrentState = IMEEK_PAUSE;
                    oid_player->pause(oid_player);
				}
				
				if((IMEEK_PAUSE == OidPlayAudio.CurrentState)
				&& (IMEEK_STOP == OidPlayAudio.NewState)){
                    OidPlayAudio.CurrentState = IMEEK_STOP;
                    break;
				}

				if((IMEEK_PLAY == OidPlayAudio.CurrentState)
				&& (IMEEK_STOP == OidPlayAudio.NewState)){
                    OidPlayAudio.CurrentState = IMEEK_STOP;
                    break;
				}
				OS_MSleep(50);
			}

			/* stop it */
			oid_player->stop(oid_player);
			
            //看看是正常播放完毕结束的，还是被其他音频打断的
			if(NO_PLAY_AUDIO == OidPlayAudio.new_play_audio_flag){
                OidPlayAudio.IsPlayOver = TRUE;
			}
			else{
                OidPlayAudio.IsPlayOver = FALSE;
			}
			OidPlayAudio.CurrentState = IMEEK_STOP;
		}
		OS_MSleep(100);
		if(last_volume != g_strFlashState.VolumeLevel){
			last_volume = g_strFlashState.VolumeLevel;
			oid_player->setvol(oid_player, 12 + last_volume*3);            
		}
	}
    return 0;
}

static void oid_play_task(void *arg)
{
//#if (OID_PLAY_MODE == FS_AUDIO_PLAY)
//    if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
//        OID_PLAY_MSG_DBG("mount fail\n");
//        goto exit;
//    }
//#endif
    
    oid_player = player_create();
    if (oid_player == NULL) {
        OID_PLAY_MSG_DBG("player create fail.\n");
        goto exit;
    }
    
    OID_PLAY_MSG_DBG("player create success.\n");
    OID_PLAY_MSG_DBG("you can use it to play, pause, resume, set volume and so on.\n");

    OID_PLAY_MSG_DBG("player set volume to 25. valid volume value is from 0~31\n");
    oid_player->setvol(oid_player, 12 + g_strFlashState.VolumeLevel*3);

    while (1) {
        OID_PLAY_MSG_DBG("===try to play media in sd/tf card===\n");
        oid_play_file_music();
    }
	
    player_destroy(oid_player);
	OID_PLAY_MSG_DBG("player destroy success.\n");

exit:
    OS_ThreadDelete(&oid_play_thread);
}

int ImeekAudioPlayStart(void)
{
    //互斥信号量创建后默认可用
//    if(OS_MutexCreate(&AudioPlayMutex) != OS_OK){
//		printf("MutexCreate error");
//	}
//	OS_MutexLock(&AudioPlayMutex,OS_WAIT_FOREVER);
//	OS_MutexUnlock(&AudioPlayMutex);

    if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
        OID_PLAY_MSG_DBG("mount fail\n");
        return -1;
    }

	if (oid_play_thread.handle) {
		OID_PLAY_MSG_DBG("oid_play_thread is already\r\n");
		return -2;
	}

    if (OS_ThreadCreate(&oid_play_thread,
                        "oid_play_task",
                        oid_play_task,
                        NULL,
                        OS_THREAD_PRIO_APP,
                        OID_PLAYER_THREAD_STACK_SIZE) != OS_OK) {
        OID_PLAY_MSG_DBG("thread create fail.exit\n");
        return -1;
    }
    return 0;
}

