#ifndef _IMEEK_FILE_H_
#define _IMEEK_FILE_H_

#include "imeek_ota.h"

#ifndef _BOOL_
#define _BOOL_
typedef unsigned char MYBOOL;
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

int imeek_add_file(const char *file_name, uint32_t pack_num,
                   const char *buf, uint32_t buf_num);
int imeek_rm_file(char *file_path);
int imeek_cmp_file_name(char* path,char *file_name);
int imeek_get_max_file(char* path,char *file_name);
int imeek_rm_dir(char *dir_path);
void GetAllMusicFile(List l,char *l_path);
MYBOOL ImeekReadFileLine(char *file_path,char *buff,uint32_t buf_num);
MYBOOL SureDir(char *dir_dst);


#endif
