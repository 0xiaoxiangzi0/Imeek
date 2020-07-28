#include <stdio.h>
#include <stdlib.h>
#include "ycq_list.h"

/*
*******************************************************************************
 *Function:     NewListNode
 *Description:  新增结点
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       返回结点指针
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
 *Description:  初始化一张表
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       返回表指针
 *Others:       如果申请内存失败，则永久停留在这里
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
 *Description:  判断表是否为空
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE:表为空
 *              FALSE:表不空
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
 *Description:  判断表长
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       length：表长，0表示空表
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
 *Description:  返回表中第k个元素
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

	//没必要判空与表长，因为如果为空或大于表长，这里都能返回NULL
	for(i=1; i < k&&p ;i++){
		p = p->next;
	}
	return p->element;
}

/*
*******************************************************************************
 *Function:     ListLocate
 *Description:  返回元素x，在表中的位置
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       0:表示表中不存在该元素
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
 *Description:  在表的第k个位置后面插入元素x
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       TRUE：插入成功
 *              FALSE：插入失败
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
	//插入的位置是否为表首
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
 *Description:  删除表中第K为位置元素，返回删除的元素
 *Calls:        
 *Called By:   
 *Input:        
 *Output:       
 *Return:       0:空表、输入k不正确、或输入k大于表长
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
 *Description:  释放表元素空间
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
 *Description:  打印表中元素
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
