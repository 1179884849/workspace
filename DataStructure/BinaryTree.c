/*
 * @Description: Binary Tree Test
 * @Version: 1.0
 * @Author: Yuanbo Zhang
 * @Date: 2020-03-24 21:13:50
 * @LastEditor: 
 * @LastEditTime: 2020-03-24 22:23:08
 */

#include <stdio.h>

typedef struct BinaryTree
{
    char data;
    struct BinaryTree *lChild;
    struct BinaryTree *rChild;
} tBinaryTree, *ptBinaryTree;

void CreateBinaryTree(ptBinaryTree *tNode)
{
    char c;
    scanf("%c", &c);
    if (' ' == c)
    {
        *tNode = NULL;
    }
    else
    {
        *tNode = (ptBinaryTree)malloc(sizeof(tBinaryTree));
        (*tNode)->data = c;
        CreateBinaryTree(&(*tNode)->lChild);
        CreateBinaryTree(&(*tNode)->rChild);
    }
}

void TraverseBinaryTree(ptBinaryTree tNode, int level)
{
    if (tNode)
    {
        printf("%c %d\n", tNode->data, level);
        TraverseBinaryTree(tNode->lChild, level + 1);
        TraverseBinaryTree(tNode->rChild, level + 1);
    }
}

int main(void)
{
    ptBinaryTree T;
    int Level = 1;
    CreateBinaryTree(&T);
    TraverseBinaryTree(T, Level);
    return 0;
}
