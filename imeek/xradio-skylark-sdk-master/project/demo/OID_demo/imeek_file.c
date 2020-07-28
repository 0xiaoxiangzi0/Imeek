#include <stdio.h>
#include <string.h>
#include "kernel/os/os.h"
#include "ota/ota.h"
#include "common/framework/platform_init.h"
#include "common/framework/fs_ctrl.h"
#include "fs/fatfs/ff.h"
#include "imeek_file.h"
#include "imeek_ota.h"
#include "app.h"

#define MAX_AUDIO_NAME 60

/*
*******************************************************************************
 *Function:     GetAllMusicFile
 *Description:  ����ļ����µ������ļ���
 *Calls:       
 *Called By:   
 *Input:        l:����ļ������б�ͷ
 *              l_path:�ļ�·��
 *Output:       
 *Return:      RETURN_TURE: ����ָ���ļ�
 *             RETURN_ERROR_1:������ָ���ļ��л��ļ�
 *Others:      ע��:·���µ����һ���ļ������ƺ��治��Ҫ�� / ����Ҫ�����
 *             Ŀ¼�µ�music�ļ��У������ʽΪ :  "music" ������ "music/"
 *             
*******************************************************************************
*/
void GetAllMusicFile(List l,char *l_path)
{
	FRESULT res;
	DIR dir;
	static FILINFO fno;
	int index = 0;
	char *l_path_add;
	int len;

    //��һ���ļ���
    res = f_opendir(&dir, l_path);
    if (res == FR_OK){
        for (;;){
			//��ȡһ���ļ���Ϣ
            res = f_readdir(&dir, &fno);
            //��ȡ�ļ����ش�����ȡ����
            if (res != FR_OK || fno.fname[0] == 0){
				break;  
			} 

			//����һ���ļ���������
			if (!(fno.fattrib & AM_DIR)){
				//�ļ������
				len = (int)strlen(fno.fname);
//				printf("fname = %s\n",fno.fname);
//				printf("len = %d\n",len);
				l_path_add = malloc(len+1);
				strcpy(l_path_add,fno.fname);
//				printf("l_path_add = %s\n",l_path_add);
				ListInsert(index,l_path_add,l);
				index++;
 			}
        }
        f_closedir(&dir);
    }
//    printf("index = %d\n",index);
}

/*
*******************************************************************************
 *Function:     imeek_get_file_name
 *Description:  Ѱ��ĳ�ļ������Ƿ���ָ�����ļ�
 *Calls:       
 *Called By:   
 *Input:        *path:     �ļ���·��
 *              file_name: ��Ѱ�ҵ��ļ�����
 *Output:       
 *Return:      RETURN_TURE: ����ָ���ļ�
 *             RETURN_ERROR:������ָ���ļ�
 *Others:      ע��:·���µ����һ���ļ������ƺ��治��Ҫ�� / ����Ҫ�����
 *             Ŀ¼�µ�music�ļ��У������ʽΪ :  "music" ������ "music/"
 *             
*******************************************************************************
*/
int imeek_cmp_file_name(char* path,char *file_name)
{
    int result = RETURN_ERROR_1;
    FRESULT res;
    DIR dir;
    static FILINFO fno;

    //��һ���ļ���
    res = f_opendir(&dir, path);
    if (res == FR_OK){
        for (;;){
			//��ȡһ���ļ���Ϣ
            res = f_readdir(&dir, &fno);
            //��ȡ�ļ����ش�����ȡ����
            if (res != FR_OK || fno.fname[0] == 0){
				break;  
			} 

			//����һ���ļ���������
            if (!(fno.fattrib & AM_DIR)){
//				printf("%s\n",fno.fname);
				//�ҵ�ͬ���ļ�
				if(0 == strcmp(file_name,fno.fname)){
				    result = RETURN_TRUE;
					break;
				}
			}
        }
        f_closedir(&dir);
    }
    return result;
}

/*
*******************************************************************************
 *Function:     imeek_get_max_file
 *Description:  Ѱ��ĳ�ļ������ַ��������ļ���,����OTAѰ�����µİ汾
 *Calls:       
 *Called By:   
 *Input:        *path:     �ļ���·��
 *              file_name: Ѱ�ҵ����ļ�����
 *Output:       
 *Return:       RETURN_TURE: ����ָ���ļ�
 *              RETURN_ERROR:������ָ���ļ�
 *Others:       ע��:·���µ����һ���ļ������ƺ��治��Ҫ�� / ����Ҫ�����
 *              Ŀ¼�µ�music�ļ��У������ʽΪ :  "music" ������ "music/"
 *             
*******************************************************************************
*/
int imeek_get_max_file(char* path,char *file_name)
{
    int result = RETURN_ERROR_1;
    FRESULT res;
    DIR dir;
    static FILINFO fno;

    //��һ���ļ���
    res = f_opendir(&dir, path);
    if (res == FR_OK){
        for (;;){
			//��ȡһ���ļ���Ϣ
            res = f_readdir(&dir, &fno);
            //��ȡ�ļ����ش�����ȡ����
            if (res != FR_OK || fno.fname[0] == 0){
				break;  
			} 

			//����һ���ļ���������
            if (!(fno.fattrib & AM_DIR)){
				//�ҵ��ַ����Ƚϴ���ļ�
				if(strcmp(file_name,fno.fname) < 0){
				    strcpy(file_name,fno.fname);
				    result = RETURN_TRUE;
				}
			}
        }
        f_closedir(&dir);
    }
    return result;
}

/*
*******************************************************************************
 *Function:     imeek_rm_dir
 *Description:  ɾ���ļ��У�����·���µ������ļ�
 *Calls:       
 *Called By:   
 *Input:        *dir_path:Ŀ¼·��
 *Output:       
 *Return:       RETURN_TURE: ����ָ���ļ�
 *              RETURN_ERROR:������ָ���ļ�
 *Others:       ע��:·���µ����һ���ļ������ƺ��治��Ҫ�� / ����Ҫ�����
 *              Ŀ¼�µ�music�ļ��У������ʽΪ :  "music" ������ "music/"
 *             
*******************************************************************************
*/
int imeek_rm_dir(char *dir_path)
{
	FRESULT res;
    DIR dir;
	UINT i;
    static FILINFO fno;

    res = f_opendir(&dir, dir_path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */

            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
				i = strlen(dir_path);
				sprintf(&dir_path[i], "/%s", fno.fname);
                res = imeek_rm_dir(dir_path);              /* Enter the directory */
                if (res != FR_OK) break;
				dir_path[i] = 0;
            } else {                                       /* It is a file. */
            	static char file_path[256];
				sprintf(file_path, "%s/%s", dir_path, fno.fname);
				res = f_unlink(file_path);
//				printf("delete file %s %s\n", file_path, (res != FR_OK) ? "failed" : "success");
				if (res != FR_OK) {
					break;
				}
            }
        }
        f_closedir(&dir);

		if (res == FR_OK) {
			res = f_unlink(dir_path);
//			printf("delete dir %s %s\n", dir_path, (res != FR_OK) ? "failed" : "success");
		}
    }

	return res;
}

/*
*******************************************************************************
 *Function:     imeek_add_file
 *Description:  ���һ�����ļ�
 *Calls:       
 *Called By:   
 *Input:        *file_name: �ļ���
 *              pack_num:   ��ǰҪ����ǵڼ�������
 *              *buf:       ������������׵�ַ
 *              buf_num:    ��д�����ݵĴ�С
 *Output:       
 *Return:      RETURN_TRUE:    д��ɹ�
 *             RETURN_ERROR_1: д��ʧ��
 *Others:      �����ļ���Ҫ��ǰ�Ѿ��ڶ�Ӧ���ļ�����
 *             
*******************************************************************************
*/
int imeek_add_file(const char *file_name, uint32_t pack_num,
                   const char *buf, uint32_t buf_num)
{
	int res = RETURN_TRUE;
	UINT bw;
	FIL fp;

	printf("pack_num = %d\n",pack_num);
	printf("file_name = %s\n",file_name);
	//����ǵ�һ������Ҫ�������ļ�������ֱ�Ӵ��ļ�
	switch(pack_num){
		case 1:
			res = f_open(&fp, file_name, FA_WRITE | FA_CREATE_ALWAYS);
			if (res != FR_OK) {
				printf("open file fail, %d\n", res);
				res = RETURN_ERROR_1;
				return res;
			}
			
			/* Write it to the destination file */
			res = f_write(&fp, buf, buf_num, &bw); 
			if (res || bw < buf_num){
				 /* error or disk full */
				 printf("write file fail, %d\n", res);
				 res = RETURN_ERROR_1;
			} 
			f_close(&fp);
			
			return res;
		default:
			res = f_open(&fp, file_name, FA_WRITE);
			if (res != FR_OK) {
				printf("open file fail, %d\n", res);
				res = RETURN_ERROR_1;
				return res;
			}
			/* �����ļ��Ľ�β */
			res=f_lseek(&fp,f_size(&fp));
			if (res != FR_OK) {
				printf("seek file fail, %d\n", res);
				res = RETURN_ERROR_1;
				f_close(&fp);
				return res;
			}
			
			/* Write it to the destination file */
			res = f_write(&fp, buf, buf_num, &bw); 
			if (res || bw < buf_num){
				 /* error or disk full */
				 printf("write file fail, %d\n", res);
				 res = RETURN_ERROR_1;
			} 
			f_close(&fp);
			
			return res;
	}

	return res;
}

/*
*******************************************************************************
 *Function:     imeek_rm_file
 *Description:  ɾ���ļ�
 *Calls:       
 *Called By:   
 *Input:        *file_path:�ļ�·��
 *Output:       
 *Return:       RETURN_TRUE: ɾ���ɹ�
 *              RETURN_ERROR:ɾ��ʧ��
 *Others:       ע��:·���µ����һ���ļ������ƺ��治��Ҫ�� / ����Ҫ�����
 *              Ŀ¼�µ�music�ļ��У������ʽΪ :  "music" ������ "music/"
 *             
*******************************************************************************
*/
int imeek_rm_file(char *file_path)
{
    int ret;
	ret = f_unlink(file_path);
	return ((ret == FR_OK) ? RETURN_TRUE : RETURN_ERROR_1);
}

/*
*******************************************************************************
 *Function:     ImeekReadFileLine
 *Description:  ��ȡָ���ļ���һ����ЧID
 *Calls:       
 *Called By:   
 *Input:        *file_path:�ļ�·��
 *Output:       
 *Return:       
 *Others:       ʲô����ЧID:��ͷΪ8200318xxxxx 
 *              ʲô����ЧID:��ͷΪFFFFFFFxxxxx
 *              ʹ�øú���������ע������:
 *                  1��ID����Ϊ12���ַ����
 *                  2��ÿ��ID��ռһ��
*******************************************************************************
*/
MYBOOL ImeekReadFileLine(char *file_path,char *buff,uint32_t buf_num)
{
    int i = 0;
	FRESULT res;
	FIL fp;
	
	res = f_open(&fp, file_path, FA_READ | FA_WRITE);
    if (res != FR_OK){
//		printf("open file fail, %d\n", res);
		return FALSE;
	}

    while(1){
		//��ȡ������
		if(f_gets(buff,buf_num,&fp)){
			i++;
			//��ȡ�������ݵ�һ������ΪF
			if('F' == buff[0]){
			    //��Ч���ݶ���������ȡ��һ������
			    continue;
			}
			else{
                //��Ч���ݣ���ȡ�����ݣ��������������Ч
                f_lseek(&fp,(i-1)*14);
				f_write(&fp,"FFFFFFF",7,&buf_num);
				f_close(&fp);
				return TRUE;
			}
		}
		//û��������
		else{
			f_close(&fp);
            return FALSE;
		}
	}
    f_close(&fp);
	return FALSE;
}

/*
*******************************************************************************
 *Function:     SureDir
 *Description:  ��ȡָ���ļ���һ����ЧID
 *Calls:       
 *Called By:   
 *Input:        *dir_dst:�ļ���·��
 *Output:       
 *Return:       TRUE���ļ��д��ڻ򴴽��ɹ�
 *              FALSE���ļ��в����ڣ��Ҵ���ʧ��
 *Others:       ע�⣺�ú������ܵݹ鴴���ļ���
 *              ����ļ����ڱ������ļ����ú��������ƻ�ԭ�ļ����ļ���
*******************************************************************************
*/
MYBOOL SureDir(char *dir_dst)
{
	FRESULT res;

	res = f_mkdir(dir_dst);
	
	if (res == FR_OK || res == FR_EXIST) {
		//�����ļ��гɹ��������ļ��б���ʹ���
		return TRUE;
	}

	return FALSE;
}


///*
//*******************************************************************************
// *Function:     ImeekDeleteFileLine
// *Description:  ɾ���ļ�����
// *Calls:       
// *Called By:   
// *Input:        *file_path:�ļ�·��
// *Output:       
// *Return:       RETURN_TRUE: ɾ���ɹ�
// *              RETURN_ERROR:ɾ��ʧ��
// *Others:       ע��:·���µ����һ���ļ������ƺ��治��Ҫ�� / ����Ҫ�����
// *              Ŀ¼�µ�music�ļ��У������ʽΪ :  "music" ������ "music/"
// *             
//*******************************************************************************
//*/
//int ImeekDeleteFileLine(char *file_path,int line)
//{
//	FRESULT res;
//	UINT br, bw;		 /* File read/write count */
//	FIL fsrc, fdst;
//    char buf[30];
//	
//	res = f_open(&fsrc, file_path, FA_READ);
//    if (res != FR_OK) {
//		printf("open file fail, %d\n", res);
//		return res;
//	}
//	printf("sizeof(buf) = %d\n",sizeof(buf));	
//}
