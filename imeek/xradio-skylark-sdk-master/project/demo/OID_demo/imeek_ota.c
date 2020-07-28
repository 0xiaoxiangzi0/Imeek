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
 *Description:  ���ַ���ת��Ϊint������
 *Calls:       
 *Called By:   
 *Input:        *p:�ַ����׵�ַ
 *Output:       
 *Return:       sum:ת����ɺ������
 *Others:       ע��:�ú���û����֤����Ĳ����Ƿ��������Դ���ʱ��ʹ����Ա��֤
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
 *Description:  �ַ�����������
 *Calls:       
 *Called By:   
 *Input:        *p:��Ƶ���ͽṹ���׵�ַ
 *Output:       
 *Return:       
 *Others:       �����������Ϊ������Ρ��͡�����Ρ����Ρ�����һ�������ͷָ��p��
 *              ��next��ָ������Ρ��ĵ�һ���ڵ㣻���趨һ��ָ��qֱ��ָ������
 *              ����ĵ�һ���ڵ㡣����˼·�ǽ����������еĵ�һ���ڵ�q����������
 *              �е����нڵ㰤���Ƚϣ�ͨ����ͣ�ƶ�rָ��ʵ�֣����ҵ����ʵ�λ��
 *              ����p���ɡ��ڸտ�ʼ��ʱ������������ֻ��ԭ�����еĵ�һ���ڵ㣬
 *              ������������ʣ�µ�ȫ���ڵ㡣�ɴ˿��Կ����ܵ�ʱ�临�Ӷ�ΪO(n2)
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
		//����Ϊ�գ�ֱ���˳�
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
 *Description:  ��ȡ������Ƶ��MD5ֵ
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       ע��:·���µ����һ���ļ������ƺ��治��Ҫ�� / ����Ҫ�����
 *             Ŀ¼�µ�music�ļ��У������ʽΪ :  "music" ������ "music/",
 *             ����������ϡ�/��
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
 *Description:  ��sd����ȡ���µ�����汾��
 *Calls:       
 *Called By:   
 *Input:        *buf:��ȡ���İ汾�Ŵ�ŵ��׵�ַ
 *Output:       
 *Return:       res:RETURN_TURE: ��ȡ�ɹ�
 *                  RETURN_ERROR:��ȡʧ��
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
 *Description:  ��ȡ��ǰ����汾��
 *Calls:       
 *Called By:   
 *Input:        *version:��ȡ���İ汾�Ŵ�ŵ��׵�ַ
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
 *Description:  �洢��ǰ����汾��
 *Calls:       
 *Called By:   
 *Input:        *version:Ҫ�洢�İ汾�Ŵ�ŵ��׵�ַ
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
 *Description:  ��ȡ��Ƶ�汾��
 *Calls:       
 *Called By:   
 *Input:        *version:��ȡ���İ汾�Ŵ�ŵ��׵�ַ
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
 *Description:  ��ȡ��Ƶ�汾��
 *Calls:       
 *Called By:   
 *Input:        *version:��ȡ���İ汾�Ŵ�ŵ��׵�ַ
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
 *Description:  �洢��ǰ��Ƶ�汾��
 *Calls:       
 *Called By:   
 *Input:        *version:Ҫ�洢�İ汾�Ŵ�ŵ��׵�ַ
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
 *Description:	ʹ���ļ�ϵͳ����OTA
 *Calls:	   
 *Called By:   
 *Input:		*version:�����������ļ���
 *Output:		
 *Return:	   
 *Others:		OTAע������:
 *				1.Ҫ�õ�fs_init()�ÿ���MMC
 *				2.url�ĸ�ʽ:
 *				  (1)�ļ�ϵͳ��url��ʽ: "file://system_dir/imeek_ota_20191212.img"
 *				  (2)http��url��ʽ: 	"http://api.linxbot.com/system_dir/imeek_ota_20191212.img"
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
	//ͨ���ļ�ϵͳ���ع̼�
    if (ota_get_image(OTA_PROTOCOL_FILE, url) != OTA_STATUS_OK) {
       printf("ota_get_image error");
	   free(url);
	   return RETURN_ERROR_1;
    }

    //У��̼�
	if (ota_verify_image(OTA_VERIFY_NONE, NULL) != OTA_STATUS_OK) {
			printf("ota_verify_image error");//У��̼�ʧ��
			free(url);
			return RETURN_ERROR_1;			
    }
	else{
			printf("ota_verify_image success");//У��̼��ɹ�
			free(url);
			return RETURN_TRUE;
	}
	
	free(url);
	return RETURN_TRUE;
}

/*
*******************************************************************************
 *Function:     http_ota
 *Description:  ʹ��http����OTA
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:      
 *Others:       ���޸�:�˺�����δ���ƣ���֤
*******************************************************************************
*/
int http_ota(void)
{
    uint8_t res;
		
	//ͨ��http���ع̼�
    if (ota_get_image(OTA_PROTOCOL_HTTP, CodeOtaStruct.url) != OTA_STATUS_OK){
        printf("http_get_image error");
	    res = RETURN_ERROR_1;
    }

    //У��̼�
	if (ota_verify_image(OTA_VERIFY_NONE, NULL) != OTA_STATUS_OK){
	    printf("http_verify_image error");//У��̼�ʧ��
        res = RETURN_ERROR_1;
    }
	else{
		printf("http_verify_image success");//У��̼��ɹ�
		res = RETURN_TRUE;
	}
	
	return res;
}

/*
*******************************************************************************
 *Function:     WaitAudioPlayEnd
 *Description:  �ȴ���������������ɣ������������ͬʱ�����������������������٣�
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
	
	//�ȴ���Ƶ��������ٽ�����������Ϊ��������Ƶ���Ų���ͬʱ����
	ota_end_flag = OTA_CONTINUE;
	while(IMEEK_PLAY == OidPlayAudio.CurrentState){
//		printf("OidPlayAudio.CurrentState = %d\n",OidPlayAudio.CurrentState);
		OS_MSleep(200);
	}
}


/*
*******************************************************************************
 *Function:     jdg_ota_or_not
 *Description:  �ж��Ƿ���Ҫ����
 *Calls:       
 *Called By:   
 *Input:        *sdk_data:��ǰʹ�õ�����汾��(int������)
 *              *imeek_ota_path:���µ����·��(�ַ�����ʽ)
 *Output:       
 *Return:       RETURN_TURE: ��Ҫ����
 *              RETURN_ERROR:����Ҫ����
 *Others:       
*******************************************************************************
*/
uint8_t jdg_ota_or_not(uint32_t *sdk_data, char *imeek_ota_path)
{
    char new_ota_string[10];
    int new_sdk_data;
	int start_num = 10;
	int end_num = 18;

	//��ȡimeek_ota_path�е�����
	for(; start_num < end_num; start_num++){
		new_ota_string[start_num - 10] = imeek_ota_path[start_num];
	}
	new_ota_string[start_num - 10] = '\0';
    printf("new_ota_string: %s\n", new_ota_string);
	
	new_sdk_data = string_to_int(new_ota_string);
    printf("new_sdk_data: %d\n", new_sdk_data);

	//���ֱȽϴ󣬲���Ҫ����
	if(new_sdk_data <= *sdk_data){
		return RETURN_ERROR_1;
	}

	*sdk_data = new_sdk_data;
	return RETURN_TRUE;
}

/*
*******************************************************************************
 *Function:     CodeOta
 *Description:  ��������������������
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

	//SD�����¾���
	if(SD_OTA == g_strOtaMode){
		get_ota_version(imeek_ota_path);
		printf("newest_sdk_version = %s\n", imeek_ota_path);
		
		//��ȡ·���е����ڣ����뵱ǰSDK���ڽ��бȶ�
		if(!jdg_ota_or_not(&g_strFlashState.LocalCodeVersion, imeek_ota_path)){
			printf("g_strFlashState.LocalCodeVersion = %d\n",g_strFlashState.LocalCodeVersion);
			//��Ҫ������������ͨ��������֪�ͻ���Ȼ���ٽ�������
			WaitAudioPlayEnd();
			
			printf("Start Ota\n");
			//��ʼ����
			res = file_ota(imeek_ota_path);

			//�������SD���������ӷ�������ȡ����Ϣ��Ҫɾ������Ȼ��һֱռ�ÿռ�
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

		//���������°汾��Ϣ
		if(CodeOtaStruct.version > g_strFlashState.LocalCodeVersion){
			//��Ҫ������������ͨ��������֪�ͻ���Ȼ���ٽ�������
			WaitAudioPlayEnd();

			printf("http_ota\n");
			res = http_ota();
			g_strFlashState.LocalCodeVersion = CodeOtaStruct.version;
			free(CodeOtaStruct.url);
			free(CodeOtaStruct.md5);
		}	

	}
	
	//�����ɹ�
	if(RETURN_TRUE == res){
		SetCodeVersion(&g_strFlashState.LocalCodeVersion);
		printf("g_strFlashState.LocalCodeVersion = %d\n",g_strFlashState.LocalCodeVersion);
		ota_reboot();
	}
	
	//����OTA�����¼�
	ota_end_flag = OTA_END;
}


/*
*******************************************************************************
 *Function:     DeleteAudio
 *Description:  ɾ����Ƶ
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
 *Description:  �����Ƶ
 *Calls:        
 *Called By:    
 *Input:        http_path:��������Ƶ·��
 *              local_audio_path:����ı�����Ƶ·��
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
		//ͨ�Ž��������һ��д��󣬾��˳�
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
			//�ɹ���ȡ��һ�����ݣ�����û�н���
			ret = imeek_add_file(l_pu8LocatPath, num, buf, size);
			if(RETURN_TRUE != ret){
				printf("write audio error\n");
				break;
			}
		}
		else{
			//�ӷ�������ȡ���ݳ�������Ӧ�ü�����ȡ���������ݶ������˳��ɣ�
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
 *Description:  �жϷ������뱾����Ƶ��֮��Ĺ�ϵ
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       0:�������뱾����Ƶ��ͬ����ǰ��Ƶ����Ҫ����
 *              1:�������뱾����Ƶ����ͬ��MD5ֵ��ͬ����ǰ��Ƶ��Ҫ����
 *              2:��ǰ������Ƶ�����������ڣ�ɾ����ǰ������Ƶ
 *              3:��ǰ��������Ƶ���ز����ڣ���ӵ�ǰ��������Ƶ
 *Others:       
*******************************************************************************
*/
int IsSameAudio(char* sever_audio_name,char* sever_audio_md5, 
                link local_audio_name,link local_audio_md5)
{
    int ret;

	if(local_audio_name == NULL){
		//������Ƶ�Ѿ�ȫ���Ա���ϣ�ʣ��ķ�������Ƶ����Ҫ���;
		return 3;
	}

	printf("sever_audio_name = %s\n",sever_audio_name);
	printf("sever_audio_md5 = %s\n",sever_audio_md5);
	printf("local_audio_name = %s\n",local_audio_name->element);
	printf("local_audio_md5 = %s\n",local_audio_md5->element);
	
	
	ret = strcmp(sever_audio_name,local_audio_name->element);
	if(!ret){
		//��������Ƶ�����ڱ�����Ƶ�����Ƚ�MD5
		ret = strcmp(sever_audio_md5,local_audio_md5->element);
		ret = ret ? 1:0;
	}
	else if(ret>0){
		//��������Ƶ�����ڱ�����Ƶ����˵�����ص�ǰ��Ƶ���Ѿ�����Ҫ�ˣ�����ɾ��
		ret = 2;
	}
	else{
		//��������Ƶ��С�ڱ�����Ƶ����˵���������ĵ�ǰ��Ƶ����û�У�Ҫ���
		ret = 3;
	}
	return ret;
}

/*
*******************************************************************************
 *Function:     GetUpdateAudioType
 *Description:  �����Ҫ��������Ƶ��������
 *Calls:        
 *Called By:    
 *Input:        
 *Output:       
 *Return:       
 *Others:       1����������Щ�������ɲ鿴������
 *              2��audio_typeΪNULL��ʾ����Ҫ���»���С�˻�û������
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
 *Description:  ������Ƶ�����������
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       ����Ĭ��:��������Ƶ���뱾����Ƶ���ǰ����ַ�����С���ϵ������е�
 *              ���޸�:  ����Ҫ����ʱ����һ������Ƶ��MD5,�����ܲ��ã�Ӧ��Ҫ��
 *                       һ���ļ���Ӧ����Ƶ����MD5ֵ����Ҫ����ʱ��ֱ�ӻ�ȡ�ͺ�
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
			//��õ�ǰ��Ƶ����������Ƶ·����NULL��ʾû����ƵҪ����
			l_u32AudioType = GetUpdateAudioType(g_aAudioUpdateData,AUDIO_TOTAL,
												AudioOtaStruct.versionCode,
												l_pu8LocalAudioPath);
			printf("l_u32AudioType = %d\n",l_u32AudioType);
			printf("l_pu8LocalAudioPath = %s\n",l_pu8LocalAudioPath);
			
			//����Ƶ��Ҫ����
			if(l_u32AudioType < AUDIO_TOTAL){
				set_imeek_light_struct(LIGHT_ON ,LIGHT_RED ,LIGHT_BRIGHT,LIGHT_CONTINUE,LIGHT_FOREVER,
									   LIGHT_ON ,LIGHT_BLUE,LIGHT_FORM  ,LIGHT_CONTINUE,LIGHT_FOREVER);
				PlayAudio(l_pu8PlayUpdateAudio,SECOND_LEVEL_AUDIO,IMEEK_PLAY);
				WaitAudioPlayOver(TRUE);

				//���������ļ����б���MD5ֵ�б�
				LocalAudioTable.L_name = ListInit();
				LocalAudioTable.L_md5 = ListInit();

				//�����Ƶ���ļ��б�
				GetAllMusicFile(LocalAudioTable.L_name,l_pu8LocalAudioPath);
				PrintList(LocalAudioTable.L_name);
				
				//����õı����ļ������ַ�������
				StringOrdering(&LocalAudioTable);
				
				//����ļ�MD5ֵ�б�
				printf("---------------MD5-------------------------------\n");
				GetAllMusicMD5(LocalAudioTable.L_name,LocalAudioTable.L_md5,l_pu8LocalAudioPath);
				PrintList(LocalAudioTable.L_md5);

				//��ʼ������Ƶ��ǰ��׼������
				sprintf(l_pu8LocalAudioPath,"%s%s",l_pu8LocalAudioPath,"/");
				l_pLocalAudioName = LocalAudioTable.L_name->first;
				l_pLocalAudioMd5 = LocalAudioTable.L_md5->first;
				index = 1;
				while(1){
					p = 0;
					FreeAudioOtaStruct(&AudioOtaStruct);
					AudioOtaStruct = HttpcGetAudioData(mac_addr,g_aAudioUpdateData[l_u32AudioType].AudioVersion,
					                                   index++,g_aAudioUpdateData[l_u32AudioType].AudioType);
					//��������Ƶ���ݴ����ڴ濨��
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
					//�����������Ѿ��������
					if(AudioOtaStruct.next == 0){
						break;
					}
				}
				for(;l_pLocalAudioName;l_pLocalAudioName = l_pLocalAudioName->next){
					DeleteAudio(l_pu8LocalAudioPath,l_pLocalAudioName->element);
					printf("delete Audio:%s\n",l_pLocalAudioName->element);
				}
				//����Ҫfree���������������õ���Ƶ����
				FreeList(LocalAudioTable.L_name);
				FreeList(LocalAudioTable.L_md5);
				//���θ��±�־λ������汾���޸�
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
 *Description:  ��ȡSD�����ݣ�����������ʽ
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       ����ͬʱ���������벥����Ƶ����ͬʱ����ʱ�����ź�������
*******************************************************************************
*/
void GetOtaMode(ENUM_OTA_MODE *mode)
{
 	char imeek_ota_path[30];

	//���SD�������µĳ���������Ϊsd������ģʽ
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
 *Description:  �ж��Ƿ���Ҫ����
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       ����ͬʱ���������벥����Ƶ����ͬʱ����ʱ�����ź�������
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

	//�����Ƶ���ļ��б�
	GetAllMusicFile(LocalAudioTable.L_name,"music/English/Spell");
	l_listlength = ListLength(LocalAudioTable.L_name);
	printf("l_listlength = %d\n",l_listlength);
	PrintList(LocalAudioTable.L_name);

	//����ļ�MD5ֵ�б�
	printf("---------------MD5-------------------------------\n");
	GetAllMusicMD5(LocalAudioTable.L_name,LocalAudioTable.L_md5,"music/English/Spell");
	l_listlength = ListLength(LocalAudioTable.L_md5);
	printf("l_listlength = %d\n",l_listlength);
	PrintList(LocalAudioTable.L_md5);

	StringOrdering(&LocalAudioTable);

//	//ȷ���ļ��д���
//	ret = SureDir("music/Test");
//	printf("ret = %d\n",ret);
}


/*
*******************************************************************************
 *Function:     imeek_ota_detect
 *Description:  �ж��Ƿ���Ҫ����
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       ����ͬʱ���������벥����Ƶ����ͬʱ����ʱ�����ź�������
*******************************************************************************
*/
void imeek_ota_detect(void *pvParameters)
{
	//���������ֻ�ڿ���ʱ���һ��
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



