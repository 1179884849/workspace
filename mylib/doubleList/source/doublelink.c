
#include <string.h>
#include <stdlib.h>
#include "doublelink.h"

ptStdListDef StdListInit(ptStdListDef ptList)
{
    ptList->ptHead = NULL;
    ptList->ptTail = NULL;
    return ptList;
}

ptStdNodeDef StdListGetHeadNode(ptStdListDef ptList)
{
    return ptList->ptHead;
}

ptStdNodeDef StdListGetTailNode(ptStdListDef ptList)
{
    return ptList->ptTail;
}

ptStdNodeDef StdListGetNextNode(ptStdNodeDef ptNode)
{
    return ptNode->ptNext;
}

ptStdNodeDef StdListGetPrevNode(ptStdNodeDef ptNode)
{
    return ptNode->ptPrev;
}

sint32 StdListPushFront(ptStdListDef ptList, ptStdNodeDef ptNode)
{
    ptNode->ptPrev = NULL;
    ptNode->ptNext = ptList->ptHead;
    if (ptList->ptHead)
    {
        ptList->ptHead->ptPrev = ptNode;
    }
    else
    {
        ptList->ptTail = ptNode;
    }
    ptList->ptHead = ptNode;

    return 0;
}

sint32 StdListPushBack(ptStdListDef ptList, ptStdNodeDef ptNode)
{
    ptNode->ptNext = NULL;
    ptNode->ptPrev = ptList->ptTail;
    if (ptList->ptTail)
    {
        ptList->ptTail->ptNext = ptNode;
    }
    else
    {
        ptList->ptHead = ptNode;
    }
    ptList->ptTail = ptNode;

    return 0;
}

sint32 StdListInsert(ptStdListDef ptList, ptStdNodeDef ptAfter, ptStdNodeDef ptNode)
{
    if (ptAfter)
    {
        if (ptAfter->ptNext)
        {
            ptAfter->ptNext->ptPrev = ptNode;
        }
        else
        {
            ptList->ptTail = ptNode;
        }
        ptNode->ptPrev = ptAfter;
        ptNode->ptNext = ptAfter->ptNext;
        ptAfter->ptNext = ptNode;
    }
    else
    {
        StdListPushFront(ptList, ptNode);
    }

    return 0;
}

ptStdNodeDef StdListPopFront(ptStdListDef ptList)
{
    if (ptList->ptHead)
    {
        ptStdNodeDef ptNode = ptList->ptHead;
        if (ptList->ptHead->ptNext)
        {
            ptList->ptHead->ptNext->ptPrev = NULL;
        }
        else
        {
            ptList->ptTail = NULL;
        }
        ptList->ptHead = ptList->ptHead->ptNext;
        ptNode->ptPrev = ptNode->ptNext = NULL;
        return ptNode;
    }
    else
    {
        return NULL;
    }
}

ptStdNodeDef StdListPopBack(ptStdListDef ptList)
{
    if (ptList->ptTail)
    {
        ptStdNodeDef ptNode = ptList->ptTail;
        if (ptList->ptTail->ptPrev)
        {
            ptList->ptTail->ptPrev->ptNext = NULL;
        }
        else
        {
            ptList->ptHead = NULL;
        }
        ptList->ptTail = ptList->ptTail->ptPrev;
        ptNode->ptPrev = ptNode->ptNext = NULL;
        return ptNode;
    }
    else
    {
        return NULL;
    }
}

ptStdNodeDef StdListRemove(ptStdListDef ptList, ptStdNodeDef ptNode)
{
    if (ptNode->ptPrev)
    {
        ptNode->ptPrev->ptNext = ptNode->ptNext;
    }
    else
    {
        ptList->ptHead = ptNode->ptNext;
    }
    if (ptNode->ptNext)
    {
        ptNode->ptNext->ptPrev = ptNode->ptPrev;
    }
    else
    {
        ptList->ptTail = ptNode->ptPrev;
    }
    return ptNode;
}

ptStdNodeDef StdListFindNextFront(ptStdListDef ptList, ptStdNodeDef ptNode)
{
    if (NULL != ptNode->ptNext)
    {
        return ptNode->ptNext;
    }
    else
    {
        return ptList->ptHead;
    }
}

ptStdNodeDef StdListFindNextBack(ptStdListDef ptList, ptStdNodeDef ptNode)
{
    if (NULL != ptNode->ptPrev)
    {
        return ptNode->ptPrev;
    }
    else
    {
        return ptList->ptTail;
    }
}

sint32 StdListJudgeHandle(ptStdListDef ptList, uint32 u32Handle)
{
    if (NULL == ptList)
    {
        return -1;
    }
    ptStdNodeDef pHead = ptList->ptHead;

    while (NULL != pHead)
    {
        if (u32Handle == (uint32)pHead)
        {
            return 0;
        }
        pHead = pHead->ptNext;
    }

    return -1;
}

sint32 StdListIsEmpty(ptStdListDef ptList)
{
    if (ptList->ptHead || ptList->ptTail)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

uint32 StdListGetSize(ptStdListDef ptList)
{
    uint32 u32LnSize = 0;

    ptStdNodeDef ptNode = StdListGetHeadNode(ptList);
    while (ptNode)
    {
        ++u32LnSize;
        ptNode = StdListGetNextNode(ptNode);
    }
    return u32LnSize;
}

ptStdListDef StdListCombine(ptStdListDef ptList1, ptStdListDef ptList2)
{
    if (!StdListIsEmpty(ptList2))
    {
        if (!StdListIsEmpty(ptList1))
        {
            ptList1->ptTail->ptNext = ptList2->ptHead;
            ptList2->ptHead->ptPrev = ptList1->ptTail;
            ptList1->ptTail = ptList2->ptTail;
        }
        else
        {
            ptList1->ptHead = ptList2->ptHead;
            ptList1->ptTail = ptList2->ptTail;
        }
        ptList2->ptHead = ptList2->ptTail = NULL;
    }
    return ptList1;
}
