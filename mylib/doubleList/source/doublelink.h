/*
 * @Description: 
 * @Version: 1.0
 * @Author: Yuanbo Zhang
 * @Date: 2020-03-22 18:02:57
 * @LastEditor: 
 * @LastEditTime: 2020-03-22 18:04:35
 */

#ifndef __DOUBLE_LINK_H__
#define __DOUBLE_LINK_H__

typedef signed int sint32;
typedef unsigned int uint32;

typedef struct tStdNodeDef
{
    struct tStdNodeDef *ptPrev;
    struct tStdNodeDef *ptNext;
} tStdNodeDef, *ptStdNodeDef;

typedef struct tStdListDef
{
    tStdNodeDef *ptHead;
    tStdNodeDef *ptTail;
} tStdListDef, *ptStdListDef;

//     ����������չ�ĺ�
#define NODE_INITIALIZER ((tStdNodeDef){ \
    .ptPrev = NULL,                      \
    .ptNext = NULL,                      \
})
#define LIST_INITIALIZER ((tStdListDef){ \
    .ptHead = NULL,                      \
    .ptTail = NULL,                      \
})
#define DECLARELISTNODE() tStdNodeDef __snStdNode
#define INIT_LISTNODE() __snStdNode = NODE_INITIALIZER

#ifdef __cplusplus
extern "C"
{
#endif

    ptStdListDef StdListInit(ptStdListDef ptList);

    ptStdNodeDef StdListGetHeadNode(ptStdListDef ptList);

    ptStdNodeDef StdListGetTailNode(ptStdListDef ptList);

    ptStdNodeDef StdListGetNextNode(ptStdNodeDef ptNode);

    ptStdNodeDef StdListGetPrevNode(ptStdNodeDef ptNode);

    sint32 StdListPushFront(ptStdListDef ptList, ptStdNodeDef ptNode);

    sint32 StdListPushBack(ptStdListDef ptList, ptStdNodeDef ptNode);

    sint32 StdListInsert(ptStdListDef ptList, ptStdNodeDef ptAfter, ptStdNodeDef ptNode);

    ptStdNodeDef StdListPopFront(ptStdListDef ptList);

    ptStdNodeDef StdListPopBack(ptStdListDef ptList);

    ptStdNodeDef StdListRemove(ptStdListDef ptList, ptStdNodeDef ptNode);

    ptStdNodeDef StdListFindNextFront(ptStdListDef ptList, ptStdNodeDef ptNode);

    ptStdNodeDef StdListFindNextBack(ptStdListDef ptList, ptStdNodeDef ptNode);

    sint32 StdListJudgeHandle(ptStdListDef ptList, uint32 u32Handle);

    sint32 StdListIsEmpty(ptStdListDef ptList);

    uint32 StdListGetSize(ptStdListDef ptList);

    ptStdListDef StdListCombine(ptStdListDef ptList1, ptStdListDef ptList2);

#ifdef __cplusplus
}
#endif

#endif //    #define __DOUBLE_LINK_H__
