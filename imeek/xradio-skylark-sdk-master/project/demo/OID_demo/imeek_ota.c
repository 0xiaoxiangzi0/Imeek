#include <stdio.h>
#include <string.h>
#include "kernel/os/os.h"
#include "ota/ota.h"
#include "common/framework/platform_init.h"
#include "common/framework/fs_ctrl.h"
#include "fs/fatfs/ff.h"
#include "kernel/os/os_mutex.h"
#include "oid_network_connect.h"
#include "app.h"
#include "imeek_ota.h"
#include "oid_flash.h"
#include "imeek_file.h"
#include "imeek_http.h"
#include "imeek_md5.h"
#include "oid_audio.h"
#include "ycq_list.h"


extern void FreeAudioOtaStruct(STRUCT_AUDIO_OTA *AudioStruct);
extern STRUCT_AUDIO_OTA HttpcGetAudioData(char *addr,int version,int index,char *audio_type);

#define AUDIO_NAME_OFFSET 3

#define OTA_TASK_CTRL_THREAD_STACK_SIZE		(5 * 1024)
static OS_Thread_t ota_handler;

ENUM_OTA_STATE ota_end_flag = OTA_DETECT;
STRUCT_HTTP_OTA CodeOtaStruct = {NOT_COMMUNICATE,
	                             4000,
	                             NULL,
	                             NULL,
	                             001};
STRUCT_AUDIO_OTA AudioOtaStruct = {NOT_COMMUNICATE,
	                               0,
	                               0,
	                               NULL,
	                               0,
	                               0,
	                              {NULL},
	                              {NULL}};
char *g_pu8HttpImeekIdMsg = "NULL";

STRUCT_AUDIO_TABLE LocalAudioTable = {NULL,NULL};
ENUM_OTA_MODE g_strOtaMode = SD_OTA;


/*
*******************************************************************************
 *Function:     string_to_int
 *Description:  将字符串转变为int型数据
 *Calls:       
 *Called By:   
 *Input:        *p:字符串首地址
 *Output:       
 *Return:       sum:转换完成后的数据
 *Others:       注意:该函数没有验证传入的参数是否有误，所以传参时的使用人员保证
*******************************************************************************
*/
int string_to_int(char *p)
{    
    int sum=0;  
    while(*p!='\0'){        
        sum=10*sum+*p-'0';   
        p++;   
    }   
    return sum;
}

/*
*******************************************************************************
 *Function:     StringOrdering
 *Description:  字符串数组排序
 *Calls:       
 *Called By:   
 *Input:        *p:音频类型结构体首地址
 *Output:       
 *Return:       
 *Others:       将整个链表分为“有序段”和“无序段”两段。设立一个虚拟的头指针p，
 *              其next域指向“有序段”的第一个节点；再设定一个指针q直接指向无序
 *              链表的第一个节点。排序思路是将无序链表中的第一个节点q与有序链表
 *              中的所有节点挨个比较（通过不停移动r指针实现），找到合适的位置
 *              插入p即可。在刚开始的时候有序链表中只有原链表中的第一个节点，
 *              无序链表则有剩下的全部节点。由此可以看出总的时间复杂度为O(n2)
 *
*******************************************************************************
*/
void StringOrdering(STRUCT_AUDIO_TABLE *p)
{    
	int i = 0;
	link p_name;
	link r_name;
	link q_name;
	link record_name = NewListNode();
	
//	link p_md5;
//	link q_md5;
//	link r_md5;
//	link record_md5 = NewListNode();

	printf("------------StringOrdering--------------");
	if(p->L_name->first == NULL){
		//链表为空，直接退出
		return;
	}
	
	record_name->next = p->L_name->first;
	p_name = p->L_name->first->next;
	r_name = p_name;
	record_name->next->next = NULL;

//	record_md5->next = p->L_md5->first;
//	p_md5 = p->L_md5->first->next;
//	r_md5 = p_md5;
//	record_md5->next->next = NULL;
	
	while(p_name != NULL){
		i++;
		printf("i = %d\n",i);
		r_name = p_name->next;
//		r_md5 = p_md5->next;
		
		q_name = record_name;
//		q_md5 = record_md5;
		
		while((q_name !=NULL)
		   && (q_name->next != NULL)
		   && (strcmp(q_name->next->element,p_name->element)<0)){
			q_name = q_name->next;
//			q_md5 = q_md5->next;
		}
		p_name->next = q_name->next;
//		p_md5->next = q_md5->next;
		
		q_name->next = p_name;
//		q_md5->next = p_md5;
		
		p_name = r_name;
//		p_md5 = r_md5;
	}
	p->L_name->first = record_name->next;
//	p->L_md5->first = record_md5->next;
	free(record_name);
//	free(record_md5);

	PrintList(p->L_name);
//	PrintList(p->L_md5);
}

/*
*******************************************************************************
 *Function:     GetAllMusicMD5
 *Description:  获取所有音频的MD5值
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       注意:路径下的最后一个文件夹名称后面不需要带 / ，如要传入根
 *             目录下的music文件夹，则传入格式为 :  "music" 而不是 "music/",
 *             本函数会加上“/”
*******************************************************************************
*/
int GetAllMusicMD5(List p_file,List q_md5,char *l_path)
{
	int index;
	char path[100];
	char *l_md5_temp;
	link p = p_file->first;
	
	for(index = 0; p; index++){
	    l_md5_temp = malloc(33);
		sprintf(path,"%s%s%s",l_path,"/",p->element);
		ImeekMD5File(l_md5_temp, path, 32);
		printf("MD5.%d  = %s\n",index,l_md5_temp);
		ListInsert(index, l_md5_temp, q_md5);
		p = p->next;
	}
	return 0;
}

/*
*******************************************************************************
 *Function:     get_ota_version
 *Description:  从sd卡获取最新的软件版本号
 *Calls:       
 *Called By:   
 *Input:        *buf:获取到的版本号存放的首地址
 *Output:       
 *Return:       res:RETURN_TURE: 获取成功
 *                  RETURN_ERROR:获取失败
 *Others:       
*******************************************************************************
*/
static int get_ota_version(char *buf)
{
	int res = RETURN_ERROR_1;   /* FatFs return code */
    
	res = imeek_get_max_file("system_dir",buf);
    
    return res;
}

/*
*******************************************************************************
 *Function:     GetCodeVersion
 *Description:  获取当前软件版本号
 *Calls:       
 *Called By:   
 *Input:        *version:获取到的版本号存放的首地址
 *Output:       
 *Return:      
 *Others:       
*******************************************************************************
*/
void GetCodeVersion(uint32_t *version)
{
    flash_int_read(FLASH_CODE_VERSION_ADDR, version);
}

/*
*******************************************************************************
 *Function:     SetCodeVersion
 *Description:  存储当前软件版本号
 *Calls:       
 *Called By:   
 *Input:        *version:要存储的版本号存放的首地址
 *Output:       
 *Return:      
 *Others:       
*******************************************************************************
*/
void SetCodeVersion(uint32_t *version)
{
	flash_int_write(FLASH_CODE_VERSION_ADDR, version);
}

/*
*******************************************************************************
 *Function:     GetAudioVersion
 *Description:  获取音频版本号
 *Calls:       
 *Called By:   
 *Input:        *version:获取到的版本号存放的首地址
 *Output:       
 *Return:      
 *Others:       
*******************************************************************************
*/
void GetAudioVersion(uint32_t *version)
{
    flash_int_read(FLASH_AUDIO_VERSION_ADDR, version);
}

/*
*******************************************************************************
 *Function:     GetAudioVersion
 *Description:  获取音频版本号
 *Calls:       
 *Called By:   
 *Input:        *version:获取到的版本号存放的首地址
 *Output:       
 *Return:      
 *Others:       
*******************************************************************************
*/
void GetALLAudioVersion(uint32_t *version)
{
    flash_int_read(FLASH_AUDIO_VERSION_ADDR, version);
}

/*
*******************************************************************************
 *Function:     SetAudioVersion
 *Description:  存储当前音频版本号
 *Calls:       
 *Called By:   
 *Input:        *version:要存储的版本号存放的首地址
 *Output:       
 *Return:      
 *Others:       
*******************************************************************************
*/
void SetAudioVersion(uint32_t *version)
{
	flash_int_write(FLASH_AUDIO_VERSION_ADDR, version);
}


/*
*******************************************************************************
 *Function: 	file_ota
 *Description:	使用文件系统进行OTA
 *Calls:	   
 *Called By:   
 *Input:		*version:用于升级的文件名
 *Output:		
 *Return:	   
 *Others:		OTA注意事项:
 *				1.要用到fs_init()得开启MMC
 *				2.url的格式:
 *				  (1)文件系统的url格式: "file://system_dir/imeek_ota_20191212.img"
 *				  (2)http的url格式: 	"http://api.linxbot.com/system_dir/imeek_ota_20191212.img"
*******************************************************************************
*/
int file_ota(char *file_name)
{
    char url_temp[] = "file://system_dir/";
 
    char *url = (char *)malloc(100);
	if (!url) {
		printf("malloc fail\n");
		return RETURN_ERROR_1;
	}
		
	sprintf(url,"%s%s",url_temp,file_name);

    printf("url = %s \n",url);
	//通过文件系统下载固件
    if (ota_get_image(OTA_PROTOCOL_FILE, url) != OTA_STATUS_OK) {
       printf("ota_get_image error");
	   free(url);
	   return RETURN_ERROR_1;
    }

    //校验固件
	if (ota_verify_image(OTA_VERIFY_NONE, NULL) != OTA_STATUS_OK) {
			printf("ota_verify_image error");//校验固件失败
			free(url);
			return RETURN_ERROR_1;			
    }
	else{
			printf("ota_verify_image success");//校验固件成功
			free(url);
			return RETURN_TRUE;
	}
	
	free(url);
	return RETURN_TRUE;
}

/*
*******************************************************************************
 *Function:     http_ota
 *Description:  使用http进行OTA
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:      
 *Others:       待修改:此函数还未完善，验证
*******************************************************************************
*/
int http_ota(void)
{
    uint8_t res;
		
	//通过http下载固件
    if (ota_get_image(OTA_PROTOCOL_HTTP, CodeOtaStruct.url) != OTA_STATUS_OK){
        printf("http_get_image error");
	    res = RETURN_ERROR_1;
    }

    //校验固件
	if (ota_verify_image(OTA_VERIFY_NONE, NULL) != OTA_STATUS_OK){
	    printf("http_verify_image error");//校验固件失败
        res = RETURN_ERROR_1;
    }
	else{
		printf("http_verify_image success");//校验固件成功
		res = RETURN_TRUE;
	}
	
	return res;
}

/*
*******************************************************************************
 *Function:     WaitAudioPlayEnd
 *Description:  等待升级语音播报完成，如果播报语音同时升级，将导致语音播报卡顿；
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void WaitAudioPlayEnd(void)
{
    char url_temp[]= "file://music/Public/_31.mp3";
	PlayAudio(url_temp,FIRST_LEVEL_AUDIO,IMEEK_PLAY);
	OS_MSleep(2000);
	
	//等待音频播放完成再进行升级，因为升级与音频播放不能同时进行
	ota_end_flag = OTA_CONTINUE;
	while(IMEEK_PLAY == OidPlayAudio.CurrentState){
//		printf("OidPlayAudio.CurrentState = %d\n",OidPlayAudio.CurrentState);
		OS_MSleep(200);
	}
}


/*
*******************************************************************************
 *Function:     jdg_ota_or_not
 *Description:  判断是否需要升级
 *Calls:       
 *Called By:   
 *Input:        *sdk_data:当前使用的软件版本号(int型数据)
 *              *imeek_ota_path:最新的软件路径(字符串形式)
 *Output:       
 *Return:       RETURN_TURE: 需要升级
 *              RETURN_ERROR:不需要升级
 *Others:       
*******************************************************************************
*/
uint8_t jdg_ota_or_not(uint32_t *sdk_data, char *imeek_ota_path)
{
    char new_ota_string[10];
    int new_sdk_data;
	int start_num = 10;
	int end_num = 18;

	//截取imeek_ota_path中的日期
	for(; start_num < end_num; start_num++){
		new_ota_string[start_num - 10] = imeek_ota_path[start_num];
	}
	new_ota_string[start_num - 10] = '\0';
    printf("new_ota_string: %s\n", new_ota_string);
	
	new_sdk_data = string_to_int(new_ota_string);
    printf("new_sdk_data: %d\n", new_sdk_data);

	//数字比较大，才需要更新
	if(new_sdk_data <= *sdk_data){
		return RETURN_ERROR_1;
	}

	*sdk_data = new_sdk_data;
	return RETURN_TRUE;
}

/*
*******************************************************************************
 *Function:     CodeOta
 *Description:  处理代码升级的相关内容
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void CodeOta(void)
{
	uint32_t res = RETURN_ERROR_1;
	char *img_version = "imeek_ota_00000001";
	char imeek_ota_path[30];

	sprintf(imeek_ota_path,"%s",img_version);

	//SD卡有新镜像
	if(SD_OTA == g_strOtaMode){
		get_ota_version(imeek_ota_path);
		printf("newest_sdk_version = %s\n", imeek_ota_path);
		
		//获取路径中的日期，并与当前SDK日期进行比对
		if(!jdg_ota_or_not(&g_strFlashState.LocalCodeVersion, imeek_ota_path)){
			printf("g_strFlashState.LocalCodeVersion = %d\n",g_strFlashState.LocalCodeVersion);
			//需要进行升级，先通过语音告知客户，然后再进行升级
			WaitAudioPlayEnd();
			
			printf("Start Ota\n");
			//开始升级
			res = file_ota(imeek_ota_path);

			//如果采用SD卡升级，从服务器获取的信息就要删除，不然会一直占用空间
			if(CodeOtaStruct.url != NULL){
				free(CodeOtaStruct.url);
				free(CodeOtaStruct.md5);
			}
		}
	}
	else if(HTTP_OTA == g_strOtaMode){
		while(NOT_COMMUNICATE == CodeOtaStruct.DetectFlag){
			OS_MSleep(1500);
		}

		//服务器有新版本信息
		if(CodeOtaStruct.version > g_strFlashState.LocalCodeVersion){
			//需要进行升级，先通过语音告知客户，然后再进行升级
			WaitAudioPlayEnd();

			printf("http_ota\n");
			res = http_ota();
			g_strFlashState.LocalCodeVersion = CodeOtaStruct.version;
			free(CodeOtaStruct.url);
			free(CodeOtaStruct.md5);
		}	

	}
	
	//升级成功
	if(RETURN_TRUE == res){
		SetCodeVersion(&g_strFlashState.LocalCodeVersion);
		printf("g_strFlashState.LocalCodeVersion = %d\n",g_strFlashState.LocalCodeVersion);
		ota_reboot();
	}
	
	//发布OTA结束事件
	ota_end_flag = OTA_END;
}


/*
*******************************************************************************
 *Function:     DeleteAudio
 *Description:  删除音频
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
int DeleteAudio(char *local_audio_path,char *file_name)
{
    char path[100];
    int ret;
	
	sprintf(path,"%s%s",local_audio_path,file_name);
	printf("DeleteAudio file path: %s\n",path);
	ret = imeek_rm_file(path); 
	if(ret == RETURN_TRUE){
		printf("delete file success\n");
	}
	else{
		printf("delete file fail\n");
	}
 	return ret;
}

/*
*******************************************************************************
 *Function:     AddAudio
 *Description:  添加音频
 *Calls:        
 *Called By:    
 *Input:        http_path:服务器音频路径
 *              local_audio_path:存入的本地音频路径
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
int AddAudio(char *http_path,char* local_audio_path,char *file_name)
{
    #define HTTPC_GET_AUDIO_NUM 1410
	uint32_t size = 0;
	char l_pu8LocatPath[100];
	int num = 0;
	char *buf;
	int ret;
	MYBOOL l_s32AddAudioRet = FALSE;

//	printf("add audio path: %s\n",http_path);

	sprintf(l_pu8LocatPath,"%s%s",local_audio_path,file_name);
	printf("l_pu8LocatPath = %s\n",l_pu8LocatPath);
	buf = (char*)malloc(HTTPC_GET_AUDIO_NUM);
	if(buf == NULL){
		printf("malloc AddAudio buf_temp error\n");
		return l_s32AddAudioRet;
	}
	
	while(1){
		ret = GetAudioFile(buf,&size, http_path, 0,num++);
		printf("size= %d\n",size);
		//通信结束，最后一包写入后，就退出
		if(0 == ret){
			ret = imeek_add_file(l_pu8LocatPath, num, buf, size);
			if(RETURN_TRUE == ret){
				printf("write audio OK\n");
				l_s32AddAudioRet = TRUE;
			}
			else{
				printf("write audio error\n");
			}
			break;
		}
		else if(1 == ret){
			//成功获取到一包数据，但是没有结束
			ret = imeek_add_file(l_pu8LocatPath, num, buf, size);
			if(RETURN_TRUE != ret){
				printf("write audio error\n");
				break;
			}
		}
		else{
			//从服务器获取数据出错，这里应该继续获取服务器数据而不是退出吧？
			printf("http get add audio error\n");
			break;
		}
	}
	free(buf);
	return l_s32AddAudioRet;
}

/*
*******************************************************************************
 *Function:     IsSameAudio
 *Description:  判断服务器与本地音频名之间的关系
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       0:服务器与本地音频相同，当前音频不需要更新
 *              1:服务器与本地音频名相同，MD5值不同，当前音频需要更新
 *              2:当前本地音频服务器不存在，删除当前本地音频
 *              3:当前服务器音频本地不存在，添加当前服务器音频
 *Others:       
*******************************************************************************
*/
int IsSameAudio(char* sever_audio_name,char* sever_audio_md5, 
                link local_audio_name,link local_audio_md5)
{
    int ret;

	if(local_audio_name == NULL){
		//本地音频已经全部对比完毕，剩余的服务器音频都需要添加;
		return 3;
	}

	printf("sever_audio_name = %s\n",sever_audio_name);
	printf("sever_audio_md5 = %s\n",sever_audio_md5);
	printf("local_audio_name = %s\n",local_audio_name->element);
	printf("local_audio_md5 = %s\n",local_audio_md5->element);
	
	
	ret = strcmp(sever_audio_name,local_audio_name->element);
	if(!ret){
		//服务器音频名等于本地音频名，比较MD5
		ret = strcmp(sever_audio_md5,local_audio_md5->element);
		ret = ret ? 1:0;
	}
	else if(ret>0){
		//服务器音频名大于本地音频名，说明本地当前音频名已经不需要了，可以删除
		ret = 2;
	}
	else{
		//服务器音频名小于本地音频名，说明服务器的当前音频本地没有，要添加
		ret = 3;
	}
	return ret;
}

/*
*******************************************************************************
 *Function:     GetUpdateAudioType
 *Description:  获得需要升级的音频的类型名
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       1、具体有哪些类型名可查看服务器
 *              2、audio_type为NULL表示不需要更新或米小克还没有联网
*******************************************************************************
*/
uint32_t GetUpdateAudioType(STRUCT_ALL_AUDIO_UPDATE_DATA *AllAudioData,
                            uint32_t num,
                            uint32_t SeverAudioVersion,
                            char *local_audio_path)
{
	uint32_t i;
	for(i=0;i<num;i++){
		if(TRUE == AllAudioData[i].Flag){
			break;
		}
	}

	printf("SeverAudioVersion = %d\n",SeverAudioVersion);
	printf("AllAudioData[ENGLISH_AUDIO].AudioVersion = %d\n",AllAudioData[ENGLISH_AUDIO].AudioVersion);
	switch(i){
		case PUBLIC_AUDIO:
			if(SeverAudioVersion > AllAudioData[PUBLIC_AUDIO].AudioVersion){
				sprintf(local_audio_path,"%s","music/Public");
				SureDir("music/Public");
			}
			else{
				AllAudioData[i].Flag = FALSE;
				i = num;
			}
			break;
		case PAINT1_AUDIO:
			break;
		case PAINT2_AUDIO:
			break;
		case PAINT3_AUDIO:
			break;
		case PAINT4_AUDIO:
			break;
		case PAINT5_AUDIO:
			break;
		case PAINT6_AUDIO:
			break;
		case PAINT7_AUDIO:
			break;
		case PAINT8_AUDIO:
			break;
		case PAINT9_AUDIO:
			break;
		case PAINT10_AUDIO:
			break;
		case MATH1_AUDIO:
			break;
		case MATH2_AUDIO:
			break;
		case MATH3_AUDIO:
			break;
		case MATH4_AUDIO:
			break;
		case MATH5_AUDIO:
			break;
		case MATH6_AUDIO:
			break;
		case MATH7_AUDIO:
			break;
		case MATH8_AUDIO:
			break;
		case MATH9_AUDIO:
			break;
		case MATH10_AUDIO:
			break;
		case BADGE_AUDIO:
			break;
		case STICKER_AUDIO:
			break;
		case ENGLISH_AUDIO:
			if(SeverAudioVersion > AllAudioData[ENGLISH_AUDIO].AudioVersion){
				sprintf(local_audio_path,"%s","music/English/Spell");
				SureDir("music/English");
				SureDir("music/English/Spell");
			}
			else{
				AllAudioData[i].Flag = FALSE;
				i = num;
			}
			break;
		case CHINESE_AUDIO:
			if(SeverAudioVersion > AllAudioData[CHINESE_AUDIO].AudioVersion){
				sprintf(local_audio_path,"%s","music/Chinese");
				SureDir("music/Chinese");
			}
			else{
				AllAudioData[i].Flag = FALSE;
				i = num;
			}
			break;
		default:
			break;
	}
	return i;
}


/*
*******************************************************************************
 *Function:     AudioOta
 *Description:  处理音频升级相关内容
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       函数默认:服务中音频名与本地音频名是按照字符串大小从上到下排列的
 *              待修改:  在需要升级时不能一个个音频求MD5,体验会很不好，应该要有
 *                       一个文件对应着音频名与MD5值，需要升级时，直接获取就好
*******************************************************************************
*/
void AudioOta(void)
{
	char *l_pu8PlayUpdateAudio = "file://music/Public/_1.mp3";
	uint32_t l_u32AudioType;
	char l_pu8LocalAudioPath[30]="NULL";
	char l_pu8SeverAudioPath[100];
	char mac_addr[13];
	uint32_t index = 1;
	uint32_t p=0;
	uint32_t ret;
	link l_pLocalAudioName,l_pLocalAudioMd5;

	GetMacAddr(mac_addr);
	printf("mac_addr =%s\n",mac_addr);

	if(HTTP_OTA == g_strOtaMode){
		while(AudioOtaStruct.DetectFlag == NOT_COMMUNICATE){
			OS_MSleep(1500);
		}

		while(1){
			//获得当前音频类型名与音频路径，NULL表示没有音频要更新
			l_u32AudioType = GetUpdateAudioType(g_aAudioUpdateData,AUDIO_TOTAL,
												AudioOtaStruct.versionCode,
												l_pu8LocalAudioPath);
			printf("l_u32AudioType = %d\n",l_u32AudioType);
			printf("l_pu8LocalAudioPath = %s\n",l_pu8LocalAudioPath);
			
			//有音频需要更新
			if(l_u32AudioType < AUDIO_TOTAL){
				set_imeek_light_struct(LIGHT_ON ,LIGHT_RED ,LIGHT_BRIGHT,LIGHT_CONTINUE,LIGHT_FOREVER,
									   LIGHT_ON ,LIGHT_BLUE,LIGHT_FORM  ,LIGHT_CONTINUE,LIGHT_FOREVER);
				PlayAudio(l_pu8PlayUpdateAudio,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
				WaitAudioPlayOver(TRUE);

				//建立本地文件名列表与MD5值列表
				LocalAudioTable.L_name = ListInit();
				LocalAudioTable.L_md5 = ListInit();

				//获得音频名文件列表
				GetAllMusicFile(LocalAudioTable.L_name,l_pu8LocalAudioPath);
				PrintList(LocalAudioTable.L_name);
				
				//将获得的本地文件名按字符串排序
				StringOrdering(&LocalAudioTable);
				
				//获得文件MD5值列表
				printf("---------------MD5-------------------------------\n");
				GetAllMusicMD5(LocalAudioTable.L_name,LocalAudioTable.L_md5,l_pu8LocalAudioPath);
				PrintList(LocalAudioTable.L_md5);

				//开始更新音频的前期准备工作
				sprintf(l_pu8LocalAudioPath,"%s%s",l_pu8LocalAudioPath,"/");
				l_pLocalAudioName = LocalAudioTable.L_name->first;
				l_pLocalAudioMd5 = LocalAudioTable.L_md5->first;
				index = 1;
				while(1){
					p = 0;
					FreeAudioOtaStruct(&AudioOtaStruct);
					AudioOtaStruct = HttpcGetAudioData(mac_addr,g_aAudioUpdateData[l_u32AudioType].AudioVersion,
					                                   index++,g_aAudioUpdateData[l_u32AudioType].AudioType);
					//将本包音频数据存入内存卡中
					while(p<AudioOtaStruct.audio_num){
						ret = IsSameAudio(AudioOtaStruct.audio_name[p],AudioOtaStruct.md5[p],
										  l_pLocalAudioName,l_pLocalAudioMd5);
						switch(ret){
							case 0:
								p++;
								l_pLocalAudioName = l_pLocalAudioName->next;
								l_pLocalAudioMd5 = l_pLocalAudioMd5->next;
								break;
							case 1:
								printf("Begin Add Audio:%s\n",AudioOtaStruct.audio_name[p]);
								sprintf(l_pu8SeverAudioPath,"%s%s",AudioOtaStruct.prefix,AudioOtaStruct.audio_path[p]);
								AddAudio(l_pu8SeverAudioPath,l_pu8LocalAudioPath,AudioOtaStruct.audio_name[p]);
								p++;
								l_pLocalAudioName = l_pLocalAudioName->next;
								l_pLocalAudioMd5 = l_pLocalAudioMd5->next;
								break;
							case 2:
								DeleteAudio(l_pu8LocalAudioPath,l_pLocalAudioName->element);
								printf("delete Audio:%s\n",l_pLocalAudioName->element);
								l_pLocalAudioName = l_pLocalAudioName->next;
								l_pLocalAudioMd5 = l_pLocalAudioMd5->next;
								break;
							case 3:
								printf("Begin Add Audio:%s\n",AudioOtaStruct.audio_name[p]);
								sprintf(l_pu8SeverAudioPath,"%s%s",AudioOtaStruct.prefix,AudioOtaStruct.audio_path[p]);
								AddAudio(l_pu8SeverAudioPath,l_pu8LocalAudioPath,AudioOtaStruct.audio_name[p]);
								p++;
								break;
							default:
								break;
						}
					}
					//服务器数据已经加载完毕
					if(AudioOtaStruct.next == 0){
						break;
					}
				}
				for(;l_pLocalAudioName;l_pLocalAudioName = l_pLocalAudioName->next){
					DeleteAudio(l_pu8LocalAudioPath,l_pLocalAudioName->element);
					printf("delete Audio:%s\n",l_pLocalAudioName->element);
				}
				//这里要free掉链表与服务器获得的音频数据
				FreeList(LocalAudioTable.L_name);
				FreeList(LocalAudioTable.L_md5);
				//本次更新标志位清除，版本号修改
				g_aAudioUpdateData[l_u32AudioType].Flag = FALSE;
				g_aAudioUpdateData[l_u32AudioType].AudioVersion = (uint32_t)AudioOtaStruct.versionCode;
				taskENTER_CRITICAL();
				WaitAudioPlayOver(TRUE);
				flash_int_write(g_aAudioUpdateData[l_u32AudioType].Addr,&g_aAudioUpdateData[l_u32AudioType].AudioVersion);
				taskEXIT_CRITICAL();
				set_imeek_light_struct(LIGHT_ON ,LIGHT_RED ,LIGHT_FORM,LIGHT_CONTINUE,LIGHT_FOREVER,
									   LIGHT_ON ,LIGHT_BLUE,LIGHT_FORM,LIGHT_CONTINUE,LIGHT_FOREVER);
			}
			OS_MSleep(500);
		}
	}
}

/*
*******************************************************************************
 *Function:     GetOtaMode
 *Description:  读取SD卡数据，决定升级方式
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       不能同时升级程序与播放音频，当同时触发时得用信号量隔开
*******************************************************************************
*/
void GetOtaMode(ENUM_OTA_MODE *mode)
{
 	char imeek_ota_path[30];

	//如果SD卡中有新的程序，则设置为sd卡更新模式
    if(RETURN_TRUE == get_ota_version(imeek_ota_path)){
		if(RETURN_TRUE == jdg_ota_or_not(&g_strFlashState.LocalCodeVersion, imeek_ota_path)){
			flash_state_read(&g_strFlashState);
			*mode = SD_OTA;
		}
		else{
			*mode = HTTP_OTA;
		}
	}
	else{
        *mode = HTTP_OTA;
	}
//	printf("ota mode = %d\n",*mode);
}

/*
*******************************************************************************
 *Function:     WaitOtaEnd
 *Description:  判断是否需要升级
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       不能同时升级程序与播放音频，当同时触发时得用信号量隔开
*******************************************************************************
*/
void WaitOtaEnd(void)
{
	while(((OTA_DETECT == ota_end_flag) 
		|| (OTA_CONTINUE == ota_end_flag))
		 &&(SD_OTA == g_strOtaMode)){
		OS_MSleep(100);
	}
}

void AudioFucTest(void)
{
//	MYBOOL ret;
	int l_listlength;
	LocalAudioTable.L_name = ListInit();
	LocalAudioTable.L_md5 = ListInit();

	//获得音频名文件列表
	GetAllMusicFile(LocalAudioTable.L_name,"music/English/Spell");
	l_listlength = ListLength(LocalAudioTable.L_name);
	printf("l_listlength = %d\n",l_listlength);
	PrintList(LocalAudioTable.L_name);

	//获得文件MD5值列表
	printf("---------------MD5-------------------------------\n");
	GetAllMusicMD5(LocalAudioTable.L_name,LocalAudioTable.L_md5,"music/English/Spell");
	l_listlength = ListLength(LocalAudioTable.L_md5);
	printf("l_listlength = %d\n",l_listlength);
	PrintList(LocalAudioTable.L_md5);

	StringOrdering(&LocalAudioTable);

//	//确定文件夹存在
//	ret = SureDir("music/Test");
//	printf("ret = %d\n",ret);
}


/*
*******************************************************************************
 *Function:     imeek_ota_detect
 *Description:  判断是否需要升级
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       不能同时升级程序与播放音频，当同时触发时得用信号量隔开
*******************************************************************************
*/
void imeek_ota_detect(void *pvParameters)
{
	//软件升级，只在开机时检测一次
	CodeOta();
	AudioOta();
//	AudioFucTest();
	OS_ThreadDelete(&ota_handler);
}

int OtaTaskCreate(void)
{
	if (OS_ThreadCreate(&ota_handler,
						"imeek_ota_detect",
						imeek_ota_detect,
						NULL,
						OS_THREAD_PRIO_APP,
						OTA_TASK_CTRL_THREAD_STACK_SIZE) != OS_OK) {
		APP_MSG_DBG("ota thread create error\n");
		return -1;
	}
	
	return 0;
}



