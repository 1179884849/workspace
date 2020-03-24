/*
 * @Description: josephus problem
 * @Version: 1.0
 * @Author: Yuanbo Zhang
 * @Date: 2020-03-08 20:20:04
 * @LastEditor: 
 * @LastEditTime: 2020-03-08 22:14:49
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct Node
{
    int id;
    struct Node *next;
} tNode, *pNode;

/**
 * @Description: 创建一个循环链表
 * @Input Param: n - 链表节点数。
 * @Output Param: 循环链表头结点
 * @Return: 循环链表第一个节点指针
 * @Date: 2020-03-08 20:24:02
 * @Author: Yuanbo Zhang
 */
pNode CreatLoopList(int n)
{
    pNode head = NULL;
    pNode pCur = malloc(sizeof(tNode));
    int i = 1;

    head = pCur;
    for (i = 1; i <= n; i++)
    {
        pNode s = (pNode)malloc(sizeof(tNode));
        s->id = i;
        pCur->next = s;
        pCur = s;
    }
    pCur->next = head->next;
    free(head);
    head = NULL;
    return pCur->next;
}

int printList(pNode head)
{
    int i = 0;

    for (i = 0; i < 50; i++)
    {
        printf("%d ", head->id);
        head = head->next;
    }
    printf("\n\n");
    return 0;
}

int main(int argc, char *argv[])
{
    int i = 0;
    int m = 8;
    int n = 3;

    pNode temp = NULL;
    pNode head = CreatLoopList(m);
    printList(head);

    n %= m;

    while (head != head->next)
    {
        for (i = 1; i < n - 1; i++)
        {
            head = head->next;
        }

        temp = head->next;
        head->next = temp->next; //这里一定要注意， 如果错写为 head = temp->next 会造成断链
        printf("%d ", temp->id);
        free(temp);
        head = head->next;
    }
    printf("%d", head->id);
    free(head);

    return 0;
}