#ifndef _IMEEK_RECORD_H_
#define _IMEEK_RECORD_H_

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

typedef struct{
    MYBOOL isRecord;   //�Ƿ�����¼��
	char   Index;      //�ڼ���¼��
}STRUCT_RECORD; 
extern STRUCT_RECORD ImeekRecord;

int RecordTaskCreate(void);

#endif
