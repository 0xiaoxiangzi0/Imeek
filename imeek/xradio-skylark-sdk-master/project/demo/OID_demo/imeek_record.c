#include <stdio.h>
#include <string.h>
#include "common/framework/fs_ctrl.h"
#include "fs/fatfs/ff.h"
#include "common/apps/recorder_app.h"
#include "common/framework/platform_init.h"
#include "kernel/os/os_time.h"
#include "audio/pcm/audio_pcm.h"
#include "audio/manager/audio_manager.h"
#include "imeek_record.h"

#define RECORD_TASK_CTRL_THREAD_STACK_SIZE	(1 * 1024)

static OS_Thread_t RecordTaskHandler;

STRUCT_RECORD ImeekRecord = {FALSE,1};

static void ImeekCedarxRecord(char *path)
{
	recorder_base *recorder;
	rec_cfg cfg;

	recorder = recorder_create();
	if (recorder == NULL) {
		printf("recorder create fail, exit\n");
		return;
	}
    
	/* record a 10s amr media */
	cfg.type = XRECODER_AUDIO_ENCODE_AMR_TYPE;
	printf("===start record amr now, last for 10s===\n");
	recorder->start(recorder, path, &cfg);
	OS_Sleep(10);
	recorder->stop(recorder);
	printf("record amr over.\n");

	recorder_destroy(recorder);
}

/*
*******************************************************************************
 *Function:     RecordTaskCreate
 *Description:  创建录音任务
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:      
*******************************************************************************
*/
static void RecordTask(void *pvParameters)
{
    char file[] = "file://music/Record/";
	char path[50];
	/* set record volume */
	audio_manager_handler(AUDIO_SND_CARD_DEFAULT, AUDIO_MANAGER_SET_VOLUME_LEVEL, AUDIO_IN_DEV_AMIC, 5);
    while(1){
	    if(TRUE == ImeekRecord.isRecord){
			sprintf(path,"%s%d%s",file,ImeekRecord.Index,".amr");
//			printf("path =%s\n",path);
			ImeekCedarxRecord(path);
			ImeekRecord.isRecord = FALSE;
		}
		OS_MSleep(500);
	}
}

/*
*******************************************************************************
 *Function:     RecordTaskCreate
 *Description:  创建录音任务
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:      
*******************************************************************************
*/
int RecordTaskCreate(void)
{
    if (OS_ThreadCreate(&RecordTaskHandler,
						"RecordTask",
						RecordTask,
						NULL,
						OS_THREAD_PRIO_APP,
						RECORD_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK){
		printf("record task thread create error\n");
		return -1;
	}
	return 0;
}

