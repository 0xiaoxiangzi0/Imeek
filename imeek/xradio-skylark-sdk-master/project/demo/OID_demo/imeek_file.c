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
 *Description:  获得文件夹下的所有文件名
 *Calls:       
 *Called By:   
 *Input:        l:输出文件名的列表头
 *              l_path:文件路径
 *Output:       
 *Return:      RETURN_TURE: 存在指定文件
 *             RETURN_ERROR_1:不存在指定文件夹或文件
 *Others:      注意:路径下的最后一个文件夹名称后面不需要带 / ，如要传入根
 *             目录下的music文件夹，则传入格式为 :  "music" 而不是 "music/"
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

    //打开一个文件夹
    res = f_opendir(&dir, l_path);
    if (res == FR_OK){
        for (;;){
			//读取一个文件信息
            res = f_readdir(&dir, &fno);
            //读取文件返回错误或读取结束
            if (res != FR_OK || fno.fname[0] == 0){
				break;  
			} 

			//这是一个文件夹则跳过
			if (!(fno.fattrib & AM_DIR)){
				//文件名入队
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
 *Description:  寻找某文件夹下是否有指定的文件
 *Calls:       
 *Called By:   
 *Input:        *path:     文件夹路径
 *              file_name: 欲寻找的文件名称
 *Output:       
 *Return:      RETURN_TURE: 存在指定文件
 *             RETURN_ERROR:不存在指定文件
 *Others:      注意:路径下的最后一个文件夹名称后面不需要带 / ，如要传入根
 *             目录下的music文件夹，则传入格式为 :  "music" 而不是 "music/"
 *             
*******************************************************************************
*/
int imeek_cmp_file_name(char* path,char *file_name)
{
    int result = RETURN_ERROR_1;
    FRESULT res;
    DIR dir;
    static FILINFO fno;

    //打开一个文件夹
    res = f_opendir(&dir, path);
    if (res == FR_OK){
        for (;;){
			//读取一个文件信息
            res = f_readdir(&dir, &fno);
            //读取文件返回错误或读取结束
            if (res != FR_OK || fno.fname[0] == 0){
				break;  
			} 

			//这是一个文件夹则跳过
            if (!(fno.fattrib & AM_DIR)){
//				printf("%s\n",fno.fname);
				//找到同名文件
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
 *Description:  寻找某文件夹下字符串最大的文件名,用于OTA寻找最新的版本
 *Calls:       
 *Called By:   
 *Input:        *path:     文件夹路径
 *              file_name: 寻找到的文件名称
 *Output:       
 *Return:       RETURN_TURE: 存在指定文件
 *              RETURN_ERROR:不存在指定文件
 *Others:       注意:路径下的最后一个文件夹名称后面不需要带 / ，如要传入根
 *              目录下的music文件夹，则传入格式为 :  "music" 而不是 "music/"
 *             
*******************************************************************************
*/
int imeek_get_max_file(char* path,char *file_name)
{
    int result = RETURN_ERROR_1;
    FRESULT res;
    DIR dir;
    static FILINFO fno;

    //打开一个文件夹
    res = f_opendir(&dir, path);
    if (res == FR_OK){
        for (;;){
			//读取一个文件信息
            res = f_readdir(&dir, &fno);
            //读取文件返回错误或读取结束
            if (res != FR_OK || fno.fname[0] == 0){
				break;  
			} 

			//这是一个文件夹则跳过
            if (!(fno.fattrib & AM_DIR)){
				//找到字符串比较大的文件
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
 *Description:  删除文件夹，及其路径下的所有文件
 *Calls:       
 *Called By:   
 *Input:        *dir_path:目录路径
 *Output:       
 *Return:       RETURN_TURE: 存在指定文件
 *              RETURN_ERROR:不存在指定文件
 *Others:       注意:路径下的最后一个文件夹名称后面不需要带 / ，如要传入根
 *              目录下的music文件夹，则传入格式为 :  "music" 而不是 "music/"
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
 *Description:  添加一个新文件
 *Calls:       
 *Called By:   
 *Input:        *file_name: 文件名
 *              pack_num:   当前要存的是第几包数据
 *              *buf:       待保存的数据首地址
 *              buf_num:    待写入数据的大小
 *Output:       
 *Return:      RETURN_TRUE:    写入成功
 *             RETURN_ERROR_1: 写入失败
 *Others:      新增文件，要求当前已经在对应的文件夹下
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
	//如果是第一包则需要创建新文件，不是直接打开文件
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
			/* 查找文件的结尾 */
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
 *Description:  删除文件
 *Calls:       
 *Called By:   
 *Input:        *file_path:文件路径
 *Output:       
 *Return:       RETURN_TRUE: 删除成功
 *              RETURN_ERROR:删除失败
 *Others:       注意:路径下的最后一个文件夹名称后面不需要带 / ，如要传入根
 *              目录下的music文件夹，则传入格式为 :  "music" 而不是 "music/"
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
 *Description:  读取指定文件第一个有效ID
 *Calls:       
 *Called By:   
 *Input:        *file_path:文件路径
 *Output:       
 *Return:       
 *Others:       什么是有效ID:开头为8200318xxxxx 
 *              什么是无效ID:开头为FFFFFFFxxxxx
 *              使用该函数有两个注意事项:
 *                  1、ID必须为12个字符组成
 *                  2、每个ID个占一行
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
		//获取到数据
		if(f_gets(buff,buf_num,&fp)){
			i++;
			//获取到的数据第一个数据为F
			if('F' == buff[0]){
			    //无效数据丢弃，并获取下一行数据
			    continue;
			}
			else{
                //有效数据，读取该数据，并令该行数据无效
                f_lseek(&fp,(i-1)*14);
				f_write(&fp,"FFFFFFF",7,&buf_num);
				f_close(&fp);
				return TRUE;
			}
		}
		//没有数据了
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
 *Description:  读取指定文件第一个有效ID
 *Calls:       
 *Called By:   
 *Input:        *dir_dst:文件夹路径
 *Output:       
 *Return:       TRUE：文件夹存在或创建成功
 *              FALSE：文件夹不存在，且创建失败
 *Others:       注意：该函数不能递归创建文件夹
 *              如果文件夹内本身有文件，该函数不会破坏原文件夹文件；
*******************************************************************************
*/
MYBOOL SureDir(char *dir_dst)
{
	FRESULT res;

	res = f_mkdir(dir_dst);
	
	if (res == FR_OK || res == FR_EXIST) {
		//创建文件夹成功，或者文件夹本身就存在
		return TRUE;
	}

	return FALSE;
}


///*
//*******************************************************************************
// *Function:     ImeekDeleteFileLine
// *Description:  删除文件内容
// *Calls:       
// *Called By:   
// *Input:        *file_path:文件路径
// *Output:       
// *Return:       RETURN_TRUE: 删除成功
// *              RETURN_ERROR:删除失败
// *Others:       注意:路径下的最后一个文件夹名称后面不需要带 / ，如要传入根
// *              目录下的music文件夹，则传入格式为 :  "music" 而不是 "music/"
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
