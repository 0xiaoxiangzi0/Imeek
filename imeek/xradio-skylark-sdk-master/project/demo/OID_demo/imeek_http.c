#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel/os/os.h"
#include "common/framework/platform_init.h"
#include "common/framework/sysinfo.h"
#include "net/wlan/wlan.h"
#include "common/framework/net_ctrl.h"
#include "net/HTTPClient/HTTPCUsr_api.h"
#include "mbedtls/mbedtls.h"
#include "oid_network_connect.h"

#include "imeek_http.h"
#include "cjson/cJSON.h"
#include "imeek_ota.h"
#include "app.h"
#include "oid_audio.h"
#include "oid_flash.h"

#define IMEEK_HTTPC_THREAD_STACK_SIZE (8 * 1024) /* ssl need more stack */
static OS_Thread_t imeek_httpc_thread;

#define IMEEK_HTTPC_URL0 "https://www.linxbot.com/api/v1/device/update?clientId=111&versionCode=111"
//#define IMEEK_HTTPC_URL0 "http://api.linxbot.com/tmp/http_test"
#define IMEEK_HTTPC_URL1 "https://tls.mbed.org"

static HTTPParameters imeek_httpc_param;
char imeek_httpc_buf[1024];

/* this CA only use in https://tls.mbed.org */
static char *httpc_demo_ca =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\r\n"
"A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\r\n"
"b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\r\n"
"MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\r\n"
"YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\r\n"
"aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\r\n"
"jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\r\n"
"xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\r\n"
"1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\r\n"
"snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\r\n"
"U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\r\n"
"9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\r\n"
"BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\r\n"
"AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\r\n"
"yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\r\n"
"38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\r\n"
"AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\r\n"
"DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\r\n"
"HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\r\n"
"-----END CERTIFICATE-----\r\n";

/*
*******************************************************************************
 *Function:     GetImeekIdMsg
 *Description:  ��ȡ�������Ƿ��¼����Ϣ
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
MYBOOL GetImeekIdMsg(char * sever_data,char *des)
{
    cJSON * pJson;
	cJSON * pSub;

	pJson = cJSON_Parse(sever_data);
	if(NULL == pJson)                                                                                         
    {
//        printf("cJSON_Parse fail");
		return FALSE;
    }

	pSub = cJSON_GetObjectItem(pJson, "msg");
    if(NULL == pSub)
    {
//        printf("cJSON_GetObjectItem msg fail\n");
	    cJSON_Delete(pJson);
	    return FALSE;
    }
	strcpy(des, pSub->valuestring);

	cJSON_Delete(pJson);
	return TRUE;
}


/*
*******************************************************************************
 *Function:     GetCodeOtaData
 *Description:  ����������ַ�����ȡ����С�������йص�����
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       ����STRUCT_HTTP_OTA�ṹ��
 *Others:       
*******************************************************************************
*/
STRUCT_HTTP_OTA GetCodeOtaData(char * sever_data)
{
    cJSON * pJson;
	cJSON * pSub;
	cJSON * pSubSub;

	STRUCT_HTTP_OTA ota_data={NOT_COMMUNICATE,
	                          4000,
	                          NULL,
	                          NULL,
	                          000};;
	
	ota_data.url = malloc(100);
	if(ota_data.url == NULL){
//		printf("ota_data.url malloc out of memory\n");
		ota_data.DetectFlag = COMMUNICATE_ERROR;		
		return ota_data;
	}

	ota_data.md5 = malloc(35);
	if(ota_data.md5 == NULL){
//		printf("ota_data.md5 malloc out of memory\n");
		ota_data.DetectFlag = COMMUNICATE_ERROR;		
		free(ota_data.url);
		ota_data.url = NULL;
		return ota_data;
	}

	pJson = cJSON_Parse(sever_data);
	if(NULL == pJson)
	{
//		printf("cJSON_Parse fail");
		ota_data.DetectFlag = COMMUNICATE_ERROR;		
		free(ota_data.url);
		free(ota_data.md5);
		ota_data.md5 = NULL;
		ota_data.url = NULL;
		return ota_data;
	}

	pSub = cJSON_GetObjectItem(pJson, "Ret");
	if(NULL == pSub)
	{
//		printf("cJSON_GetObjectItem msg fail\n");
		cJSON_Delete(pJson);
		ota_data.DetectFlag = COMMUNICATE_ERROR;		
		free(ota_data.url);
		free(ota_data.md5);
		return ota_data;
	}
	ota_data.Ret = pSub->valueint;

	pSub = cJSON_GetObjectItem(pJson, "data");
	if(NULL == pSub)
	{
//		printf("cJSON_GetObjectItem data fail\n");
		cJSON_Delete(pJson);
		ota_data.DetectFlag = COMMUNICATE_ONE_END;		
		free(ota_data.url);
		free(ota_data.md5);
		ota_data.url = NULL;
		ota_data.md5 = NULL;
		return ota_data;
	}

	pSubSub = cJSON_GetObjectItem(pSub, "url");
	if(NULL == pSubSub)
	{
//		printf("cJSON_GetObjectItem url fail\n");
		cJSON_Delete(pJson);
		ota_data.DetectFlag = COMMUNICATE_ERROR;		
		free(ota_data.url);
		free(ota_data.md5);
		ota_data.url = NULL;
		ota_data.md5 = NULL;
		return ota_data;
	}
	strcpy(ota_data.url, pSubSub->valuestring);

	pSubSub = cJSON_GetObjectItem(pSub, "md5");
	if(NULL == pSubSub)
	{
//		printf("cJSON_GetObjectItem md5 fail\n");
		cJSON_Delete(pJson);
		ota_data.DetectFlag = COMMUNICATE_ERROR;		
		free(ota_data.url);
		free(ota_data.md5);
		ota_data.url = NULL;
		ota_data.md5 = NULL;
		return ota_data;
	}
	strcpy(ota_data.md5, pSubSub->valuestring);

	pSubSub = cJSON_GetObjectItem(pSub, "versionCode");
	if(NULL == pSubSub)
	{
//		printf("cJSON_GetObjectItem versionCode fail\n");
		cJSON_Delete(pJson);
		ota_data.DetectFlag = COMMUNICATE_ERROR;		
		free(ota_data.url);
		free(ota_data.md5);
		ota_data.url = NULL;
		ota_data.md5 = NULL;
		return ota_data;
	}
	ota_data.version = pSubSub->valueint;	

	cJSON_Delete(pJson);
	ota_data.DetectFlag = COMMUNICATE_ONE_END;
	return ota_data;
}

/*
*******************************************************************************
 *Function:     GetAudioOtaData
 *Description:  ����������ַ�����ȡ��������Ƶ�йص�����
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       ����STRUCT_HTTP_OTA�ṹ��
 *Others:      
*******************************************************************************
*/
STRUCT_AUDIO_OTA GetAudioOtaData(char * sever_data)
{
    int i;
    cJSON * pJson;
	cJSON * pSub;
	cJSON * pSubSub;
	cJSON * array_list;
	cJSON * tasklist;
	int     array_size;

	STRUCT_AUDIO_OTA audio_ota_data={NOT_COMMUNICATE,
	                                 0,
	                                 0,
	                                 NULL,
	                                 0,
	                                 0,
	                                 {NULL},
	                                 {NULL}};
	
	audio_ota_data.prefix = malloc(100);
	if(audio_ota_data.prefix == NULL){
//		printf("audio_ota_data.prefix malloc out of memory\n");
	    audio_ota_data.DetectFlag = COMMUNICATE_ERROR;
		return audio_ota_data;
	}

	pJson = cJSON_Parse(sever_data);
	if(NULL == pJson)
	{
//		printf("cJSON_Parse fail\n");
		audio_ota_data.DetectFlag = COMMUNICATE_ERROR;
		free(audio_ota_data.prefix);
		audio_ota_data.prefix = NULL;
		return audio_ota_data;
    }

	pSub = cJSON_GetObjectItem(pJson, "ret");
	if(NULL == pSub)
	{
//		printf("cJSON_GetObjectItem ret fail\n");
		cJSON_Delete(pJson);
		audio_ota_data.DetectFlag = COMMUNICATE_ERROR;
		free(audio_ota_data.prefix);
		audio_ota_data.prefix = NULL;
		return audio_ota_data;
	}
	audio_ota_data.Ret = pSub->valueint;

	pSub = cJSON_GetObjectItem(pJson, "data");
	if(NULL == pSub)
	{
//	    printf("cJSON_GetObjectItem data fail\n");
		cJSON_Delete(pJson);
	    audio_ota_data.DetectFlag = COMMUNICATE_ERROR;		
		free(audio_ota_data.prefix);
		audio_ota_data.prefix = NULL;
		return audio_ota_data;
	}

    pSubSub = cJSON_GetObjectItem(pSub, "versionCode");
	if(NULL == pSubSub)
	{
//	    printf("cJSON_GetObjectItem versionCode fail\n");
		cJSON_Delete(pJson);
	    audio_ota_data.DetectFlag = COMMUNICATE_ERROR;		
		free(audio_ota_data.prefix);
		audio_ota_data.prefix = NULL;
		return audio_ota_data;
	}
	audio_ota_data.versionCode = pSubSub->valueint;

    pSubSub = cJSON_GetObjectItem(pSub, "prefix");
	if(NULL == pSubSub)
	{
//	    printf("cJSON_GetObjectItem prefix fail\n");
		cJSON_Delete(pJson);
	    audio_ota_data.DetectFlag = COMMUNICATE_ONE_END;		
		free(audio_ota_data.prefix);
		audio_ota_data.prefix = NULL;
		return audio_ota_data;
	}
	strcpy(audio_ota_data.prefix, pSubSub->valuestring);

    pSubSub = cJSON_GetObjectItem(pSub, "next");
	if(NULL == pSubSub)
	{
	    printf("cJSON_GetObjectItem next fail\n");
		cJSON_Delete(pJson);
	    audio_ota_data.DetectFlag = COMMUNICATE_ONE_END;		
		free(audio_ota_data.prefix);
		audio_ota_data.prefix = NULL;
		return audio_ota_data;
	}
	audio_ota_data.next = pSubSub->valueint;

	array_list = cJSON_GetObjectItem(pSub, "list");
	if(NULL == pSubSub){
//	    printf("cJSON_GetObjectItem versionCode fail\n");
		cJSON_Delete(pJson);
	    audio_ota_data.DetectFlag = COMMUNICATE_ONE_END;		
		free(audio_ota_data.prefix);
		audio_ota_data.prefix = NULL;
	    return audio_ota_data;
	}

	//�����С
	array_size = cJSON_GetArraySize(array_list);
	audio_ota_data.audio_num = array_size;
	tasklist = array_list->child;
	for(i=0; i<array_size; i++){
		audio_ota_data.audio_name[i] = malloc(35);
		strcpy(audio_ota_data.audio_name[i],cJSON_GetObjectItem(tasklist,"k")->valuestring);
		sprintf(audio_ota_data.audio_name[i],"%s%s",audio_ota_data.audio_name[i],".mp3");
		audio_ota_data.audio_path[i] = malloc(35);
		strcpy(audio_ota_data.audio_path[i],cJSON_GetObjectItem(tasklist,"v")->valuestring);
		audio_ota_data.md5[i] = malloc(35);
		strcpy(audio_ota_data.md5[i],cJSON_GetObjectItem(tasklist,"md5")->valuestring);
		tasklist = tasklist->next;
	}

	cJSON_Delete(pJson);
	audio_ota_data.DetectFlag = COMMUNICATE_ONE_END;
	return audio_ota_data;
}

/*
*******************************************************************************
 *Function:     GetMacAddr
 *Description:  ��ȡ��ǰ��С�˵�mac��ַ
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       ���ش����С��mac��ַ���׵�ַ
 *Others:      
*******************************************************************************
*/
void GetMacAddr(char *addr)
{
    int i,j;
    
	//�öδ������ڻ�ȡmac��ַ
	static struct sysinfo *sys_info = NULL;
	sys_info = sysinfo_get();

    for(i=0;i<6;i++){
		j = (sys_info->mac_addr[i]>>4) & 0x0F;
        if(j<10 && j>=0){
			addr[2*i] = j+'0';
		}
		else{
			addr[2*i] = j-10+'a';
		}
		
		j = sys_info->mac_addr[i] & 0x0F;
        if(j<10 && j>=0){
			addr[2*i+1] = j+'0';
		}
		else{
			addr[2*i+1] = j-10+'a';
		}
	}
	addr[12] = '\0';
}

static void* httpc_demo_set_client_cert(void)
{
	static security_client httpc_demo_cert;
	memset(&httpc_demo_cert, 0, sizeof(httpc_demo_cert));

	httpc_demo_cert.pCa = httpc_demo_ca;
	httpc_demo_cert.nCa = strlen(httpc_demo_ca) + 1;

	return &httpc_demo_cert;
}

/*
*******************************************************************************
 *Function:     HttpcGetData
 *Description:  ���������ȡ����
 *Calls:       
 *Called By:   
 *Input:        char* url:Ҫ���ʵ�url
 *              use_ssl:  ���ʷ�ʽ    0:http    1:https
 *              size:     ����ʱ��ʾ��Ҫ���뻺�����Ĵ�С
 *Output:       
 *Return:       
 *Others:       ���øú����ĺ����ǵ�����Ļ�����free
*******************************************************************************
*/
char* HttpcGetData(uint32_t size, char *url, int use_ssl)
{
    uint32_t download_len = 0;
	INT32 recv_len = 0;
	int32_t ret;
	uint32_t download_time = 0;
	char* buf;
	char* buf_temp;

	memset(&imeek_httpc_param, 0, sizeof(imeek_httpc_param));
	memcpy(imeek_httpc_param.Uri, url, strlen(url));
	imeek_httpc_param.nTimeout = 30; //timeout 30s for every get
	if (use_ssl) {
		/* set CA cert */
		HTTPC_Register_user_certs(httpc_demo_set_client_cert);
		/* set ssl verify mode */
		HTTPC_set_ssl_verify_mode(2);
	}

	download_time = OS_GetTicks();

	buf = (char*)malloc(size);
	if(buf == NULL){
//		printf("malloc httpc buf error\n");
		return NULL;
	}
	memset(buf, 0, size);

	buf_temp = (char*)malloc(1440);
	if(buf_temp == NULL){
//		printf("malloc httpc buf_temp error\n");
		return NULL;
	}
	memset(buf_temp, 0, 1440);
	
	while(1){
    	ret = HTTPC_get(&imeek_httpc_param, buf_temp, 1430, &recv_len);
		if (ret != HTTP_CLIENT_SUCCESS){
			break;
		}
	    download_len += (uint32_t)recv_len;
		sprintf(buf,"%s%s",buf,buf_temp);
		memset(buf_temp,0,1440);
	}
	
	if (ret == HTTP_CLIENT_EOS) {
		download_time = OS_GetTicks() - download_time;
		printf("\n\ndownload complete, download_len %d, time %ds\n",
			   download_len, download_time/1000);
	} 
	else {
		printf("\n\ndownload error, ret=%d\n", ret);
	}
	free(buf_temp);
    return buf;
}

/*
*******************************************************************************
 *Function:     GetAudioFile
 *Description:  ���������ȡ����
 *Calls:       
 *Called By:   
 *Input:        char* url:Ҫ���ʵ�url
 *              use_ssl:  ���ʷ�ʽ    0:http    1:https
 *              size:     ����ʱ��ʾ��Ҫ���뻺�����Ĵ�С
 *                        ���ʱ��ʾ��ȡ����ʵ�����ݵĴ�С
 *              num:    0 :��ͷ��ȡurl�µ��ļ�
 *                      >0:�����ϴ�url�ļ��Ļ�ȡ
 *Output:       
 *Return:       
 *Others:       ���øú����ĺ����ǵ�����Ļ�����free
 *              ���޸�:�˺���Ҫ�޸ģ������øú���Ҫ�ܿ����ǽ����ϴμ�����ȡ��
 *                     ������ͷ��ȡ
*******************************************************************************
*/
int GetAudioFile(char* buf, uint32_t *size,char *url, int use_ssl,int num)
{
	INT32 recv_len = 0;
	int32_t ret;

    if(0 == num){
		memset(&imeek_httpc_param, 0, sizeof(imeek_httpc_param));
		memcpy(imeek_httpc_param.Uri, url, strlen(url));
		imeek_httpc_param.nTimeout = 30; //timeout 30s for every get
		if (use_ssl) {
			/* set CA cert */
			HTTPC_Register_user_certs(httpc_demo_set_client_cert);
			/* set ssl verify mode */
			HTTPC_set_ssl_verify_mode(2);
		}
    }
	
	memset(buf,0,1410);
	ret = HTTPC_get(&imeek_httpc_param, buf, 1410, &recv_len);
	*size = (uint32_t)recv_len;
	if(ret == HTTP_CLIENT_EOS){
		printf("\n\ndownload complete, download_len %d\n",(uint32_t)recv_len);
		ret = 0;
	} 
	
	else if(ret == HTTP_CLIENT_SUCCESS){
		printf("\n\ndownload complete, download_len %d\n",(uint32_t)recv_len);
		ret = 1;
	} 
	else{
		printf("\n\ndownload error, ret=%d\n", ret);
		ret = 2;
	}

	return ret;
}

/*
*******************************************************************************
 *Function:     HttpcSendImeekId
 *Description:  ������С��ID
 *Calls:       
 *Called By:   
 *Input:        char* addr:��ǰ��С�˵�mac��ַ
 *              char* ImeekID:��С�˵�ID�׵�ַ
 *Output:       
 *Return:       TRUE�����ͳɹ�	 FALSE������ʧ��
 *Others:       
*******************************************************************************
*/
static MYBOOL HttpcSendImeekId(char *addr,STRUCT_IMEEK_ID* ImeekId)
{
	char* url;
    char* buf;
	char  msg[20];
    uint32_t size = 1440;    //һ��http���ݰ��Ĵ�С
	MYBOOL res = FALSE;

	//���޸�:���������汾��20200318��ʱ��ʱһ��Ҫ�滻�ɺ꣬���뵽ͬһ��ͷ�ļ��У�
	//       �����Ժ��޸�
	url = malloc(100 * sizeof(char));
	sprintf(url,"%s%s%s%s","https://www.linxbot.com/api/v1/device/init?clientId=",
		                   ImeekId->Id,
		                   "&MAC=",
		                   addr);
//    printf("url = %s\n",url);
	//��ȡ���������ص�����
	buf = HttpcGetData(size,url,0);

//    printf("buf = %s\n",buf);
	//����Ҫ�����ݽ���json��ʽ�Ľ���
	if((GetImeekIdMsg(buf,msg)) && (!strcmp(msg,"SUCCESS"))){
		res = TRUE;
	}
	free(url);
	free(buf);
	return res;
}


/*
*******************************************************************************
 *Function:     HttpcGetCodeVersion
 *Description:  ���������ȡ��������汾�ţ������°汾�������Ϣ
 *Calls:       
 *Called By:   
 *Input:        char* addr:��ǰ��С�˵�mac��ַ
 *              char* version:��ǰ��С�˵�����汾��
 *Output:       
 *Return:       STRUCT_OTA�ṹ������
 *Others:       
*******************************************************************************
*/
static STRUCT_HTTP_OTA HttpcGetCodeVersion(char *addr)
{
	char* url;
    char* buf;
    uint32_t size = 2048;    //һ��http���ݰ��Ĵ�С
	STRUCT_HTTP_OTA ota_data;

    //���޸�:���������汾��20200318��ʱ��ʱһ��Ҫ�滻�ɺ꣬���뵽ͬһ��ͷ�ļ��У�
    //       �����Ժ��޸�
    url = malloc(100 * sizeof(char));
	sprintf(url,"%s%s%s%d","https://www.linxbot.com/api/v1/device/update?clientId=",
		                   addr,
		                   "&versionCode=",
		                   g_strFlashState.LocalCodeVersion);

    //��ȡ���������ص�����
	buf = HttpcGetData(size,url,0);

    //����Ҫ�����ݽ���json��ʽ�Ľ���
    ota_data = GetCodeOtaData(buf);
	printf("get Code data\n");
	printf("flag: %d\n", ota_data.DetectFlag);
	if(COMMUNICATE_ERROR == ota_data.DetectFlag){
        printf("get code data conmunicate with sever error\n");
	}
	printf("Ret : %d\n", ota_data.Ret);
	printf("url : %s  \n", ota_data.url);
	printf("md5 : %s  \n", ota_data.md5);
	printf("versionCode : %d\n", ota_data.version);

	free(url);
	free(buf);
	return ota_data;
}

/*
*******************************************************************************
 *Function:     HttpcGetAudioVersion
 *Description:  ���������ȡ������Ƶ�汾�ţ������°汾�������Ϣ
 *Calls:       
 *Called By:   
 *Input:        char* addr:��ǰ��С�˵�mac��ַ
 *              char* version:��ǰ��С�˵���Ƶ�汾��
 *Output:       
 *Return:       STRUCT_AUDIO_OTA�ṹ������
 *Others:       
*******************************************************************************
*/
static STRUCT_AUDIO_OTA HttpcGetAudioVersion(char *addr)
{
//	int i;
	char* url;
	char* buf;
	uint32_t size = 1440;    //һ��http���ݰ��Ĵ�С
	STRUCT_AUDIO_OTA audio_ota_data;

	printf("-----------HttpcGetAudioVersion---------");
	url = malloc(200 * sizeof(char));
	sprintf(url,"%s%s%s","https://www.linxbot.com/api/v1/device/audio?clientId=",
		                 addr,
		                 "&versionCode=1&pn=1000");

	//��ȡ���������ص�����
	buf = HttpcGetData(size,url,0);
//	for(i = 0;buf[i] != '\0'; i++)
//		printf("%c",buf[i]);

		   
	//����Ҫ�����ݽ���json��ʽ�Ľ���
	audio_ota_data = GetAudioOtaData(buf);
//	printf("audio_ota_data.DetectFlag: %d\n", audio_ota_data.DetectFlag);
//	if(COMMUNICATE_ERROR == audio_ota_data.DetectFlag){
//		printf("get audio data conmunicate with sever error\n");
//	}
//	printf("audio_ota_data.Ret : %d\n", audio_ota_data.Ret);
//	printf("audio_ota_data.versionCode : %d\n", audio_ota_data.versionCode);
//	printf("audio_ota_data.prefix : %s\n", audio_ota_data.prefix);
//	printf("audio_ota_data.next : %d\n", audio_ota_data.next);
//	printf("audio_ota_data.audio_num : %d\n", audio_ota_data.audio_num);
//	for(i=0; i<audio_ota_data.audio_num; i++){
//		printf("    audio_ota_data.audio_name : %s\n", audio_ota_data.audio_name[i]);
//		printf("    audio_ota_data.audio_path : %s\n", audio_ota_data.audio_path[i]);
//		printf("    audio_ota_data.md5 : %s\n", audio_ota_data.md5[i]);
//	}

	free(url);
	free(buf);
	return audio_ota_data;
}

/*
*******************************************************************************
 *Function:     HttpcGetAudioData
 *Description:  ���������ȡһ����Ƶ����
 *Calls:       
 *Called By:   
 *Input:        char* addr:��ǰ��С�˵�mac��ַ
 *              version:����������Ƶ���͵���Ƶ�汾��
 *              index:�ڼ�������
 *              audio_type:����������Ƶ����
 *Output:       
 *Return:       STRUCT_AUDIO_OTA�ṹ������
 *Others:       
*******************************************************************************
*/
STRUCT_AUDIO_OTA HttpcGetAudioData(char *addr,uint32_t version,uint32_t index,char *audio_type)
{
	int i;
	char* url;
	char* buf;
	uint32_t size = 5120;    //һ��http���ݰ��Ĵ�С
	STRUCT_AUDIO_OTA audio_ota_data;

	printf("-----------HttpcGetAudioVersion---------");
	url = malloc(200 * sizeof(char));
	sprintf(url,"%s%s%s%d%s%d%s%s","https://www.linxbot.com/api/v1/device/audio?clientId=",
		                           addr,
		                           "&versionCode=",
		                           version,
		                           "&pn=",
		                           index,
		                           "&filter=",
		                           audio_type);
	printf("url = %s\n",url);
	//��ȡ���������ص�����
	buf = HttpcGetData(size,url,0);
//	for(i = 0;buf[i] != '\0'; i++)
//		printf("%c",buf[i]);

		   
	//����Ҫ�����ݽ���json��ʽ�Ľ���
	audio_ota_data = GetAudioOtaData(buf);
	printf("audio_ota_data.DetectFlag: %d\n", audio_ota_data.DetectFlag);
	if(COMMUNICATE_ERROR == audio_ota_data.DetectFlag){
		printf("get audio data conmunicate with sever error\n");
	}
	printf("audio_ota_data.Ret : %d\n", audio_ota_data.Ret);
	printf("audio_ota_data.versionCode : %d\n", audio_ota_data.versionCode);
	printf("audio_ota_data.prefix : %s\n", audio_ota_data.prefix);
	printf("audio_ota_data.next : %d\n", audio_ota_data.next);
	printf("audio_ota_data.audio_num : %d\n", audio_ota_data.audio_num);
	for(i=0; i<audio_ota_data.audio_num; i++){
		printf("    audio_ota_data.audio_name : %s\n", audio_ota_data.audio_name[i]);
		printf("    audio_ota_data.audio_path : %s\n", audio_ota_data.audio_path[i]);
		printf("    audio_ota_data.md5 : %s\n", audio_ota_data.md5[i]);
	}

	free(url);
	free(buf);
	return audio_ota_data;
}

/*
*******************************************************************************
 *Function:     FreeAudioOtaStruct
 *Description:  �ͷ����������У�����Ŀռ�
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:      
*******************************************************************************
*/
void FreeAudioOtaStruct(STRUCT_AUDIO_OTA *AudioStruct)
{
	int i;

	AudioStruct->audio_num = 0;
	if(AudioStruct->prefix != NULL){
		free(AudioStruct->prefix);
	}

	for(i=0;i<AUDIO_NUM;i++){
		if(AudioStruct->audio_name[i] != NULL){
			free(AudioStruct->audio_name[i]);
			AudioStruct->audio_name[i] = NULL;
		}
	}
	
	for(i=0;i<AUDIO_NUM;i++){
		if(AudioStruct->md5[i] != NULL){
			free(AudioStruct->md5[i]);
			AudioStruct->md5[i] = NULL;
		}
	}

	for(i=0;i<AUDIO_NUM;i++){
		if(AudioStruct->audio_path[i] != NULL){
			free(AudioStruct->audio_path[i]);
			AudioStruct->audio_path[i] = NULL;
		}
	}
}


/*
*******************************************************************************
 *Function:     SendImeekIDToSever
 *Description:  �����δ����С��ID���͸�����������ô���͸�������
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:      
*******************************************************************************
*/
void SendImeekIDToSever(char* mac_addr)
{
	//��һ��ʱ��������С�˵�ID��������
	if((FALSE == g_strImeekId.SendFlag)
	&& ('8' == g_strImeekId.Id[0])){
		if(HttpcSendImeekId(mac_addr,&g_strImeekId)){
			printf("send id to sever ok\n");
			g_strImeekId.SendFlag = TRUE;
			WaitAudioPlayOver(TRUE);
			taskENTER_CRITICAL();
			imeek_id_write(&g_strImeekId);
			taskEXIT_CRITICAL();
		}
	}
}


/*
*******************************************************************************
 *Function:     imeek_httpc_fun
 *Description:  ��������������OTA����Ƶ�ĸ���
 *Calls:       
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:      
*******************************************************************************
*/
static void imeek_httpc_fun(void *arg)
{
	char mac_addr[13];

	//�ȴ������ɹ�
	while(NETWORK_CONNECT != imeek_network_event.network_state){
		OS_MSleep(1500);
	}

	if(HTTP_OTA == g_strOtaMode){
		GetMacAddr(mac_addr);
		printf("addr = %s\n",mac_addr);
		
		SendImeekIDToSever(mac_addr);
		
		CodeOtaStruct = HttpcGetCodeVersion(mac_addr);
		AudioOtaStruct = HttpcGetAudioVersion(mac_addr);
	}
	OS_ThreadDelete(&imeek_httpc_thread);
}

int http_task_create(void){
	if(HTTP_OTA == g_strOtaMode){
		if (!OS_ThreadIsValid(&imeek_httpc_thread)){
			if(OS_ThreadCreate(&imeek_httpc_thread,
								"imeek_httpc_thread",
								imeek_httpc_fun,
								(void *)NULL,
								OS_THREAD_PRIO_APP,
								IMEEK_HTTPC_THREAD_STACK_SIZE) == OS_OK){
				HTTPC_MSG_INFO("httpc create task success\r\n");
				return HTTPC_TASK_CREAT_SUCCESS;
			}
			else{
				HTTPC_MSG_ERR("httpc thread create error\n");
				return HTTPC_TASK_CREAT_ERROR;
			}
		}
		else{
			HTTPC_MSG_WRN("httpc_task is already\r\n");
			return HTTPC_TASK_CREAT_ERROR;
		}
    }
	return 0;
}
