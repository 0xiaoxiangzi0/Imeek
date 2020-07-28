#include <stdio.h>
#include <stdlib.h>
#include "ycq_list.h"

/*
*******************************************************************************
 *Function:     NewListNode
 *Description:  �������
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       ���ؽ��ָ��
 *Others:       
*******************************************************************************
*/      
link NewListNode(void)
{
	link p;
	p = malloc(sizeof(*p));
	if(NULL == p){
		printf("out of memory");
	}
	p->next = NULL;
	return p;
}

/*
*******************************************************************************
 *Function:     ListInit
 *Description:  ��ʼ��һ�ű�
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       ���ر�ָ��
 *Others:       ��������ڴ�ʧ�ܣ�������ͣ��������
*******************************************************************************
*/      
List ListInit(void)
{
	List l;
	l = malloc(sizeof(*l));
	if(NULL == l){
		printf("out of memory");
		return NULL;
	}
	l->first = NULL;
	return l;
}

/*
*******************************************************************************
 *Function:     ListEmpty
 *Description:  �жϱ��Ƿ�Ϊ��
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:��Ϊ��
 *              FALSE:����
 *Others:       
*******************************************************************************
*/      
int ListEmpty(List l)
{
	return (l->first == NULL);
}

/*
*******************************************************************************
 *Function:     ListLength
 *Description:  �жϱ�
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       length������0��ʾ�ձ�
 *Others:       
*******************************************************************************
*/      
int ListLength(List l)
{
	link p;
	int length = 0;
	p = l->first;
	while(p){
		length++;
		p = p->next;
	}

	return length;
}

/*
*******************************************************************************
 *Function:     ListRetrieve
 *Description:  ���ر��е�k��Ԫ��
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
ListItem ListRetrieve(int k,List l)
{
	link p = l->first;
	int i;

	if(k < 1){
		printf("out of bounds\n");
		return 0;
	}

	//û��Ҫ�п��������Ϊ���Ϊ�ջ���ڱ������ﶼ�ܷ���NULL
	for(i=1; i < k&&p ;i++){
		p = p->next;
	}
	return p->element;
}

/*
*******************************************************************************
 *Function:     ListLocate
 *Description:  ����Ԫ��x���ڱ��е�λ��
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       0:��ʾ���в����ڸ�Ԫ��
 *Others:       
*******************************************************************************
*/      
int ListLocate(ListItem x,List l)
{
	int i;
	link p = l->first;

	for(i=1; p && (p->element != x);i++){
		p = p->next;
	}
	return p? i:0;
}

/*
*******************************************************************************
 *Function:     ListInsert
 *Description:  �ڱ�ĵ�k��λ�ú������Ԫ��x
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE������ɹ�
 *              FALSE������ʧ��
 *Others:       
*******************************************************************************
*/      
int ListInsert(int k,ListItem x,List l)
{
	int i;
	link p = l->first;
	link q;
	if(k<0){
		printf("out of bounds\n");
		return FALSE;
	}

	for(i=1; i<k&&p; i++){
		p = p->next;
	}
	
	q = NewListNode();
	q->element = x;
	//�����λ���Ƿ�Ϊ����
	if(k){
		q->next = p->next;
		p->next = q;
	}
	else{
		q->next = p;
		l->first = q;
	}
	return TRUE;
}

/*
*******************************************************************************
 *Function:     ListDelete
 *Description:  ɾ�����е�KΪλ��Ԫ�أ�����ɾ����Ԫ��
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       0:�ձ�����k����ȷ��������k���ڱ�
 *Others:       
*******************************************************************************
*/      
ListItem ListDelete(int k,List l)
{
	int i;
	ListItem x;
	link p = l->first;
	link q;
	if(k<1 || !l->first){
		printf("out of bounds\n");
		return 0;
	}

	if(k == 1)
		l->first = p->next;
	else{
		q=l->first;
		for(i = 1; i<k-1 && p;i++){
			p = p->next;
		}
		p=q->next;
		q->next = p->next;
	}
	x= p->element;
	free(p);
	return x;
}

/*
*******************************************************************************
 *Function:     FreeList
 *Description:  �ͷű�Ԫ�ؿռ�
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/
void FreeList(List l)
{
	link p = l->first;
	link q;
	
	while(p != NULL){
		q = p;
		free(q->element);
		q->element = NULL;
		free(q);
		q = NULL;
		p = p->next;
	}
	free(l);
	l = NULL;
}


/*
*******************************************************************************
 *Function:     PrintList
 *Description:  ��ӡ����Ԫ��
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       
 *Others:       
*******************************************************************************
*/      
void PrintList(List l)
{
	link p = l->first;
	int i;
	for(i = 0; p;i++){
		printf("%d:%s\n",i,p->element);
		p=p->next;
	}
}
