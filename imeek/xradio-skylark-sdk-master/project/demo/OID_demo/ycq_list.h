#ifndef _YCQ_LIST_H_
#define _YCQ_LIST_H_

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif


typedef char* ListItem;

typedef struct lnode *link;
typedef struct lnode{
	ListItem element;
	link next;
}ListNode;

typedef struct llist *List;
typedef struct llist{
	link first;
}Llist;

link NewListNode(void);
List ListInit(void);
int ListEmpty(List l);
int ListLength(List l);
ListItem ListRetrieve(int k,List l);
int ListLocate(ListItem x,List l);
int ListInsert(int k,ListItem x,List l);
ListItem ListDelete(int k,List l);
void PrintList(List l);
void FreeList(List l);

#endif
