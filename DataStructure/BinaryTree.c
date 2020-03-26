/*
 * @Description: Binary Tree Test
 * @Version: 1.0
 * @Author: Yuanbo Zhang
 * @Date: 2020-03-24 21:13:50
 * @LastEditor: 
 * @LastEditTime: 2020-03-26 22:41:38
 */

#include <stdio.h>
#include <stdlib.h>

typedef enum
{
    ZERO,
    ONE,
} tTag;

typedef struct BinaryTree
{
    char data;
    tTag lTag;
    tTag rTag;
    struct BinaryTree *lChild;
    struct BinaryTree *rChild;
} tBinaryTree, *ptBinaryTree;

//全局变量 用于当前遍历节点的前驱。
ptBinaryTree pre;

/**
 * @Description: 递归建立二叉树
 * @Input Param: 
 * @Output Param: 
 * @Return: 
 * @Date: 2020-03-26 21:24:35
 * @Author: Yuanbo Zhang
 */
void CreateBinaryTree(ptBinaryTree *tNode)
{
    char c;
    scanf("%c", &c);
    if ('#' == c)
    {
        *tNode = NULL;
    }
    else
    {
        *tNode = (ptBinaryTree)malloc(sizeof(tBinaryTree));
        (*tNode)->data = c;
        (*tNode)->lTag = ZERO;
        (*tNode)->lTag = ZERO; // 将初始的左右节点的标志 置为 0
        CreateBinaryTree(&(*tNode)->lChild);
        CreateBinaryTree(&(*tNode)->rChild);
    }
}
/**
 * @Description: 对二叉树进行中序线索化
 * @Input Param: 
 * @Output Param: 
 * @Return: 
 * @Date: 2020-03-26 21:27:42
 * @Author: Yuanbo Zhang
 */
void InThread(ptBinaryTree T)
{
    if (T)
    {
        InThread(T->lChild);
        if (NULL == T->lChild)
        {
            T->lChild = pre;
            T->lTag = ONE;
        }

        if (NULL != pre && pre->rChild == NULL)
        {
            pre->rChild = T;
            pre->rTag = ONE;
        }
        pre = T;
        InThread(T->rChild);
    }
}
void visit(char date)
{
    printf("date is %c\n", date);
}

/**
 * @Description: 循环的方式遍历
 * @Input Param: 
 * @Output Param: 
 * @Return: 
 * @Date: 2020-03-26 22:35:49
 * @Author: Yuanbo Zhang
 */
void TraverseBinaryTree1(ptBinaryTree tNode)
{
    ptBinaryTree T = tNode;
    ptBinaryTree i;

    while (T->lTag != ONE)
    {
        T = T->lChild;
    }

    for (i = T; i != NULL; i = i->rChild)
    {
        visit(i->data);
    }
}

/**
 * @Description: 递归的形式遍历
 * @Input Param: 
 * @Output Param: 
 * @Return: 
 * @Date: 2020-03-26 22:36:14
 * @Author: Yuanbo Zhang
 */

void Traverse(ptBinaryTree tNode)
{
    if (tNode != NULL)
    {
        visit(tNode->data);
        Traverse(tNode->rChild);
    }
}

void TraverseBinaryTree2(ptBinaryTree tNode)
{
    ptBinaryTree T = tNode;
    ptBinaryTree i;

    while (T->lTag != ONE)
    {
        T = T->lChild;
    }
    Traverse(T);
}

int main(void)
{
    ptBinaryTree T;
    int Level = 1;
    CreateBinaryTree(&T);
    pre = T;
    InThread(T);
    TraverseBinaryTree2(T);
    return 0;
}
