/*
 * @Description: 
 * @Version: 1.0
 * @Author: Yuanbo Zhang
 * @Date: 2020-03-06 11:05:18
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2020-03-06 11:12:42
 */
#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

// �̵߳�5��״̬: idle, used, delete, exit, dead
#define TP_THREAD_IDLE (1)	 // idle, �߳̿���, ����ʱ�����û�����
#define TP_THREAD_USED (2)	 // used, �߳��Ѽ����û�����
#define TP_THREAD_FUNC_DEL (3) // delete, ɾ���û�����, �û���⵽�ñ�־, �������˳��û�����
#define TP_THREAD_EXIT (4)	 // exit, �߳����˳�, ����ɾ��/�����߳���Դ
#define TP_THREAD_DEAD (0)	 // dead, �߳�����ȫ�˳�, ����δ����

#define MAX_USE_THREAD_NUM (20)
#define THREAD_STACK_SIZE (0x200000) // 0x200000 - 2Mbyte; 0x800000 - 8MByte

// �̳߳����̵߳�����
typedef struct tThrdAttrDef
{
	unsigned int u32StackSize; // �߳�ջ��С
	signed int s32ThrdPri;	 // �߳����ȼ�
	signed int s32ThrdSched;   // �̵߳��Ȳ���
} tThrdAttrDef;

// �̳߳ض���ṹ��ӿ�
typedef void tThreadPoolDef;

extern tThreadPoolDef *CreateThrdPool(unsigned int u32ThrdNum, tThrdAttrDef *ptAttr);
extern signed int DeleteThrdPool(tThreadPoolDef *ptThreadPool);
extern signed int AddWork(tThreadPoolDef *ptThreadPool,
						  void (*pUserFunc)(void *pUserArg, const unsigned int *pu32IsDel),
						  void *pUserarg,
						  unsigned int *pu32UserID);
extern signed int DeleteWork(tThreadPoolDef *ptThreadPool, const unsigned int u32UserID);
extern signed int DeleteWorkWithoutWait(tThreadPoolDef *ptThreadPool, unsigned int u32UserID);

extern unsigned int GetIdleThrdNum(tThreadPoolDef *ptThreadPool);
extern tThreadPoolDef *InitTheadPool(void);

#endif
