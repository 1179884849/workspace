/*
 * @Description: �̳߳�ʵ��
 * @Version: 1.0
 * @Author: Yuanbo Zhang
 * @Date: 2020-03-06 11:05:18
 * @LastEditors: Please set LastEditors
 * InitTheadPool ԭ������ֵΪ unsigned int�����ǿ��ת��Ϊ void* ��ַ��int ����Ϊ4�ֽڣ�
 * ǿ��ת���ĵ�ַҲΪ 4 �ֽڣ�������64λ�������ϣ�������ڴ���ʴ�����Ϊ64Ϊ��������ַΪ8�ֽ� 
 * ������н�����룬��Ŀ���豸��32λ�Ļ�����Ҫת����unsigned int ����
 * @LastEditTime: 2020-03-29 13:48:21
 */
/*****************************************************************************
 *
 * Thread Pool / �̳߳�, �ṩ���º����ӿ�
 *   CreateThrdPool : �����̳߳�, ��ϵͳ�޷�����������Ŀ�߳�ʱ, ����ʧ��
 *   DeleteThrdPool : ɾ���̳߳�
 *   AddWork : �����û�����
 *   DeleteWork : ɾ���û�����, �ȴ�ɾ����ɺ󷵻�
 *   DeleteWorkWithouWait : ɾ���û�����, �����û������˳�, �ɹ����̸߳������
 *
 *
 * - �����������߳̾��Ƿ�������(detach)
 * - ���л�����������ͨ������(PTHREAD_MUTEX_INITIALIZER)
 * - �̵߳��Ȳ���, ���ȼ�, �������û�����
 *
 *   PTHREAD_MUTEX_INITIALIZER : ��ͨ��, �ٴμ���-����; ����ʱ����-������; 
 *   PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP : �����, �ٴμ���-���ش���; ����ʱ����-���ش���;
 *   PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP : �ݹ���, �ٴμ���-����; ����ʱ����-���ش���;
 *
 *   SCHED_OTHER : ��ʱ���Ȳ���, ��֧�����ȼ�����
 *   SCHED_FIFO	: ʵʱ���Ȳ���, �ȵ��ȷ���, һ��ռ��cpu��һֱ����, ֱ�����������ȼ��߳���ռ���Լ��˳�
 *   SCHED_RR	: ʵʱ���Ȳ���, ʱ��Ƭ��ת, ���߳�ʱ��Ƭ����, ϵͳ���·���ʱ��Ƭ, �����߳����ھ�������β, ��֤�����о�����ͬ���ȼ��̵߳ĵ��ȹ�ƽ
 *
 *****************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "thread_pool.h"

#define TP_POOL_CREATE (0)	// created, �̳߳�״̬, �̳߳��Ѵ���, ״̬����
#define TP_POOL_DESTROY (1) // destroyed, �̳߳�״̬, �̳߳ؽ��˳�

#define TP_STACK_THRD_MAX_SIZE (0x01000000) // 16M, �߳����ջ��С
#define TP_STACK_THRD_MIN_SIZE (0x00020000) // 100K, �߳���Сջ��С

// �߳���Ϣ
typedef struct tThrdInfoDef
{
	void *pUserArg;			// �û�����ָ��
	unsigned int u32UserID; // �û�ID, �û��������ʶ��ID
	void (*pUserFunc)(void *pUserArg, const unsigned int *pu32IsDel);
	// �û�����, UserFunc(void *pUserarg, const unsigned int *pu32IsDel)
	// void *pUserArg, �û�����ָ��
	// const unsigned int *pu32IsDel, �û������˳���־,
	// if (TP_THREAD_FUNC_DEL == *pu32IsDel), �û������������˳�

	pthread_t tThrdID;

	unsigned int u32State; // �߳�״̬: idle, used, delete, exit, dead
	pthread_mutex_t tLock;
	pthread_cond_t tCond;
} tThrdInfoDef;

// �̳߳�
typedef struct tThrdPoolDef
{
	struct tThrdPoolDef *ptNext; // ָ��ָ����һ�̳߳�

	pthread_t tThrdID; // �����߳�ID

	unsigned int u32TotalNum; // �����߳�����
	unsigned int u32AddCntr;  // �̳߳ر������û������Ĵ���
	unsigned int u32RunState; // �̳߳�״̬: created or destroyed
	tThrdAttrDef tThrdAttr;	  // �����߳�����

	pthread_mutex_t tLock;

	tThrdInfoDef ptThrdQue[0]; // �̳߳�
} tThrdPoolDef;

// �̳߳�����ͷָ��, ��ʼ��ΪNULL
static tThrdPoolDef *ptTPLinkHead = NULL;

// �̳߳���������, ��ʼ��Ϊ��ͨ������, ����ʱ��������ѡ�� : -lpthread -D_GNU_SOURCE
static pthread_mutex_t tTPLinkLock = PTHREAD_MUTEX_INITIALIZER;

/*********************************************************************
 * �������������̳߳����������һ���̳߳ؽڵ�, ��ӵ�����ͷ��
 * ���������ptThrdPool - ������̳߳ؽڵ�ָ��
 * �����������
 * �� �� ֵ���ɹ� - ����0; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

static signed int PoolLinkAdd(tThrdPoolDef *ptThrdPool)
{
	signed int s32Rtn;

	if (NULL == ptThrdPool)
	{
		return -1;
	}

	s32Rtn = pthread_mutex_lock(&tTPLinkLock);
	if (0 != s32Rtn)
	{
		return -1;
	}

	// add to link
	ptThrdPool->ptNext = ptTPLinkHead;
	ptTPLinkHead = ptThrdPool;

	pthread_mutex_unlock(&tTPLinkLock);

	return 0;
}

/*********************************************************************
 * �������������̳߳�������ɾ��ָ���̳߳ؽڵ�
 * ���������ptPoolDel - ��ɾ���̳߳�ָ��
 * �����������
 * �� �� ֵ���ɹ� - ����0; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

static signed int PoolLinkDel(const tThrdPoolDef *ptPoolDel)
{
	signed int s32Rtn;
	tThrdPoolDef *ptPoolTmp;
	tThrdPoolDef *ptPoolPre;

	if (NULL == ptPoolDel)
	{
		return 0;
	}

	s32Rtn = pthread_mutex_lock(&tTPLinkLock);
	if (0 != s32Rtn)
	{
		return -1;
	}

	// find the pool, and delete from link
	for (ptPoolTmp = ptTPLinkHead, ptPoolPre = ptTPLinkHead;
		 ptPoolTmp != NULL;
		 ptPoolPre = ptPoolTmp, ptPoolTmp = ptPoolTmp->ptNext)
	{
		if (ptPoolDel == ptPoolTmp)
		{
			ptPoolPre->ptNext = ptPoolTmp->ptNext;
			break;
		}
	}

	pthread_mutex_unlock(&tTPLinkLock);

	if (NULL == ptPoolTmp)
	{
		return -1;
	}

	return 0;
}

/*********************************************************************
 * �������������̳߳������в���ָ���̳߳�
 * ���������ptPoolDel - �������̳߳�ָ��
 * �����������
 * �� �� ֵ���ɹ� - �����̳߳�ָ��; ʧ�� - ����NULL
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

static inline tThrdPoolDef *PoolLinkSearch(tThrdPoolDef *ptPoolSearch)
{
	tThrdPoolDef *ptPoolTmp;

	for (ptPoolTmp = ptTPLinkHead; ptPoolTmp != NULL; ptPoolTmp = ptPoolTmp->ptNext)
	{
		if (ptPoolSearch == ptPoolTmp)
		{
			break;
		}
	}

	return ptPoolTmp;
}

/*********************************************************************
 * ������������ʼ���߳���Ϣ�ṹ��
 * ���������ptThrdInfo - �߳���Ϣ�ṹ��
 * �����������
 * �� �� ֵ���ɹ� - ����0; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

static signed int InitThrdInfo(tThrdInfoDef *ptThrdInfo)
{
	if (NULL == ptThrdInfo)
	{
		return -1;
	}

	ptThrdInfo->pUserArg = NULL;
	ptThrdInfo->pUserFunc = NULL;
	ptThrdInfo->u32UserID = 0;

	pthread_mutex_init(&(ptThrdInfo->tLock), NULL);
	pthread_cond_init(&(ptThrdInfo->tCond), NULL);
	ptThrdInfo->u32State = TP_THREAD_IDLE;

	return 0;
}

/*********************************************************************
 * ��������������߳���Ϣ�ṹ��
 * ���������ptThrdInfo - �߳���Ϣ�ṹ��
 * �����������
 * �� �� ֵ���ɹ� - ����0; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

static signed int CleanThrdInfo(tThrdInfoDef *ptThrdInfo)
{
	if (NULL == ptThrdInfo)
	{
		return -1;
	}

	ptThrdInfo->pUserArg = NULL;
	ptThrdInfo->pUserFunc = NULL;
	ptThrdInfo->u32UserID = 0;

	pthread_mutex_destroy(&(ptThrdInfo->tLock));
	pthread_cond_destroy(&(ptThrdInfo->tCond));
	ptThrdInfo->u32State = TP_THREAD_DEAD;

	return 0;
}

/*********************************************************************
 * ������������ʼ���̳߳���Ϣ�ṹ��
 * ���������ptThrdPool - �̳߳���Ϣ�ṹ��
 * �����������
 * �� �� ֵ���ɹ� - ����0; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

static signed int InitThrdPool(tThrdPoolDef *ptThrdPool)
{
	if (NULL == ptThrdPool)
	{
		return -1;
	}

	ptThrdPool->ptNext = NULL;
	ptThrdPool->u32TotalNum = 0;
	ptThrdPool->u32AddCntr = 0;
	ptThrdPool->u32RunState = TP_POOL_CREATE;
	pthread_mutex_init(&(ptThrdPool->tLock), NULL);

	return 0;
}

/*********************************************************************
 * ��������������̳߳���Ϣ�ṹ��
 * ���������ptThrdPool - �̳߳���Ϣ�ṹ��
 * �����������
 * �� �� ֵ���ɹ� - ����0; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

static signed int CleanThrdPool(tThrdPoolDef *ptThrdPool)
{
	if (NULL == ptThrdPool)
	{
		return -1;
	}

	ptThrdPool->ptNext = NULL;
	ptThrdPool->u32TotalNum = 0;
	ptThrdPool->u32AddCntr = 0;
	pthread_mutex_destroy(&(ptThrdPool->tLock));

	return 0;
}

/*********************************************************************
 * ���������������߳�
 * ���������pvArg - �̳߳���Ϣ�ṹ��
 * �����������
 * �� �� ֵ����
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

static void ThrdManagement(void *pvArg)
{
	tThrdPoolDef *ptThrdPool;
	unsigned int i;
	unsigned int u32DeadCntr;

	if (NULL == pvArg)
	{
		goto POOL_OUT;
	}
	ptThrdPool = (tThrdPoolDef *)pvArg;

	while (1)
	{
		sleep(1);

		pthread_mutex_lock(&(ptThrdPool->tLock));
		if (TP_POOL_DESTROY == ptThrdPool->u32RunState)
		{
			for (i = 0, u32DeadCntr = 0; i < ptThrdPool->u32TotalNum; i++)
			{
				pthread_mutex_lock(&(ptThrdPool->ptThrdQue[i].tLock));
				switch (ptThrdPool->ptThrdQue[i].u32State)
				{
				case TP_THREAD_FUNC_DEL:
				case TP_THREAD_EXIT:
					break;
				case TP_THREAD_DEAD:
					u32DeadCntr++;
					break;
				case TP_THREAD_USED:
					ptThrdPool->ptThrdQue[i].u32State = TP_THREAD_FUNC_DEL;
					break;
				case TP_THREAD_IDLE:
				default:
					ptThrdPool->ptThrdQue[i].u32State = TP_THREAD_EXIT;
					break;
				}
				pthread_mutex_unlock(&(ptThrdPool->ptThrdQue[i].tLock));
				pthread_cond_broadcast(&(ptThrdPool->ptThrdQue[i].tCond));
			}

			if (u32DeadCntr == ptThrdPool->u32TotalNum)
			{
				pthread_mutex_unlock(&(ptThrdPool->tLock));
				goto POOL_OUT;
			}
		}
		pthread_mutex_unlock(&(ptThrdPool->tLock));
	}

POOL_OUT:

	for (i = 0; i < ptThrdPool->u32TotalNum; i++)
	{
		CleanThrdInfo(&(ptThrdPool->ptThrdQue[i]));
	}
	CleanThrdPool(ptThrdPool);
	free(ptThrdPool);

	return;
}

/*********************************************************************
 * ���������������߳�
 * ���������pvArg - �߳���Ϣ�ṹ��
 * �����������
 * �� �� ֵ����
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

static void ThrdWork(void *pvArg)
{
	tThrdInfoDef *ptThrdInfo;

	if (NULL == pvArg)
	{
		goto THRD_OUT;
	}
	ptThrdInfo = (tThrdInfoDef *)pvArg;

	while (1)
	{
		// waitting for add user function
		pthread_mutex_lock(&(ptThrdInfo->tLock));
		while (TP_THREAD_IDLE == ptThrdInfo->u32State)
		{
			pthread_cond_wait(&(ptThrdInfo->tCond), &(ptThrdInfo->tLock));

			// receive signal, check thread state
			switch (ptThrdInfo->u32State)
			{
			case TP_THREAD_USED:
				pthread_mutex_unlock(&(ptThrdInfo->tLock));
				goto THRD_WORK;
				break;
			case TP_THREAD_EXIT:
			case TP_THREAD_DEAD: // in normal, NERVER run here
				ptThrdInfo->u32State = TP_THREAD_DEAD;
				pthread_mutex_unlock(&(ptThrdInfo->tLock));
				goto THRD_OUT;
				break;
			case TP_THREAD_FUNC_DEL:
			default:
				pthread_mutex_unlock(&(ptThrdInfo->tLock));
				goto THRD_FUNC_DEL;
				break;
			}
		}
		pthread_mutex_unlock(&(ptThrdInfo->tLock));

	THRD_WORK:
		// do user function
		if (ptThrdInfo->pUserFunc != NULL)
		{
			ptThrdInfo->pUserFunc(ptThrdInfo->pUserArg, (const unsigned int *)&(ptThrdInfo->u32State));
		}

	THRD_FUNC_DEL:
		// user function finished, set thread idle
		pthread_mutex_lock(&(ptThrdInfo->tLock));
		ptThrdInfo->pUserArg = NULL;
		ptThrdInfo->pUserFunc = NULL;
		ptThrdInfo->u32UserID = 0;
		ptThrdInfo->u32State = TP_THREAD_IDLE;
		pthread_mutex_unlock(&(ptThrdInfo->tLock));
		pthread_cond_signal(&(ptThrdInfo->tCond));
	}

THRD_OUT:
	return;
}

/*********************************************************************
 * ���������������߳�
 * ���������ptAttr - �߳����Բ���ָ��
 *           ptThrdID - �������߳�IDָ��
 *           pvFunc - �������̺߳���
 *           pvPara - �������̺߳�����ڲ���
 * �����������
 * �� �� ֵ���ɹ� - ����0; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

static signed int CreateThrd(tThrdAttrDef *ptAttr, pthread_t *ptThrdID, void *pvFunc, void *pvPara)
{
	signed int s32Rtn;
	struct sched_param tSchPara;
	pthread_attr_t tThrdAttr;

	if ((NULL == ptAttr) || (NULL == ptThrdID) || (NULL == pvFunc) || (NULL == pvPara))
	{
		return -1;
	}

	// init thread attribute struct
	pthread_attr_init(&tThrdAttr);

	// set thread schedule, SCHED_OTHER/SCHED_RR/SCHED_FIFO
	pthread_attr_setschedpolicy(&tThrdAttr, ptAttr->s32ThrdSched);

	// set thread priority, 1 - 99
	if ((SCHED_FIFO == ptAttr->s32ThrdSched) || (SCHED_RR == ptAttr->s32ThrdSched))
	{
		tSchPara.sched_priority = ptAttr->s32ThrdPri;
		pthread_attr_setschedparam(&tThrdAttr, &tSchPara);
	}

	// set thread inherit sched, PTHREAD_EXPLICIT_SCHED/PTHREAD_INHERIT_SCHED
	pthread_attr_setinheritsched(&tThrdAttr, PTHREAD_EXPLICIT_SCHED);

	// set stack size
	pthread_attr_setstacksize(&tThrdAttr, ptAttr->u32StackSize);

	// set thread detached, PTHREAD_CREATE_DETACHED/PTHREAD_CREATE_JOINABLE
	pthread_attr_setdetachstate(&tThrdAttr, PTHREAD_CREATE_DETACHED);

	// create thread
	s32Rtn = pthread_create(ptThrdID, &tThrdAttr, pvFunc, pvPara);
	if (0 != s32Rtn)
	{
		perror("CreateThrd:");
	}

	// destroy pthread_attr_t
	pthread_attr_destroy(&tThrdAttr);

	return s32Rtn;
}

/*********************************************************************
 * ���������������̳߳�
 * ���������u32ThrdNum - �贴�����߳���
 *           ptAttr - �߳�����; ����ָ��ΪNULL, ��ʹ��Ĭ������
 * �����������
 * �� �� ֵ���ɹ� - ���ض�Ӧ�̳߳�ָ��; ʧ�� - ����NULL
 * ����˵����
 *           �߳����� :
 *               �߳�ջ��С : Ĭ��8 MByte, ���16MByte, ��С16KByte
 *               �̵߳��Ȳ��� : Ĭ��SCHED_RR, ��������ΪSCHED_OTHER, SCHED_FIFO
 *               �߳����ȼ� : SCHED_OTHER��Ϊ0; SCHED_RR/SCHED_FIFO, Ĭ��1, 1-99
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

tThreadPoolDef *CreateThrdPool(unsigned int u32ThrdNum, tThrdAttrDef *ptAttr)
{
	unsigned int i;
	unsigned int u32BufSize;
	signed int s32Rtn;
	tThrdPoolDef *ptThrdPool;
	tThrdAttrDef tThrdAttr;

	// check thread num
	if (0 == u32ThrdNum)
	{
		return NULL;
	}

	// initialize thread attribute
	if (NULL == ptAttr)
	{
		tThrdAttr.u32StackSize = THREAD_STACK_SIZE;
		tThrdAttr.s32ThrdSched = SCHED_RR;
		tThrdAttr.s32ThrdPri = 1;
	}
	else
	{
		// thread stack size
		if ((ptAttr->u32StackSize < TP_STACK_THRD_MIN_SIZE) ||
			(ptAttr->u32StackSize > TP_STACK_THRD_MAX_SIZE))
		{
			return NULL;
		}
		tThrdAttr.u32StackSize = ptAttr->u32StackSize;

		// thread schedule and priority
		switch (ptAttr->s32ThrdSched)
		{
		case SCHED_OTHER:
			tThrdAttr.s32ThrdSched = ptAttr->s32ThrdSched;
			tThrdAttr.s32ThrdPri = 0;
			break;
		case SCHED_RR:
		case SCHED_FIFO:
			if ((ptAttr->s32ThrdPri < 1) || (ptAttr->s32ThrdPri > 99))
			{
				return NULL;
			}
			tThrdAttr.s32ThrdSched = ptAttr->s32ThrdSched;
			tThrdAttr.s32ThrdPri = ptAttr->s32ThrdPri;
			break;
		default:
			return NULL;
		}
	}

	// allocate memory, once for all
	u32BufSize = sizeof(tThrdPoolDef) + sizeof(tThrdInfoDef) * u32ThrdNum;
	ptThrdPool = (tThrdPoolDef *)malloc(u32BufSize);
	if (NULL == ptThrdPool)
	{
		return NULL;
	}
	memset(ptThrdPool, 0, u32BufSize);

	// init thread pool info
	InitThrdPool(ptThrdPool);
	ptThrdPool->tThrdAttr.s32ThrdPri = tThrdAttr.s32ThrdPri;
	ptThrdPool->tThrdAttr.s32ThrdSched = tThrdAttr.s32ThrdSched;
	ptThrdPool->tThrdAttr.u32StackSize = tThrdAttr.u32StackSize;

	// create management thread
	s32Rtn = CreateThrd(&tThrdAttr, &(ptThrdPool->tThrdID), ThrdManagement, ptThrdPool);
	if (0 != s32Rtn)
	{
		goto ERR_OUT_NO_MANAGEMENT;
	}

	// creat all work thread
	for (i = 0; i < u32ThrdNum; i++)
	{
		// init thread info
		InitThrdInfo(&(ptThrdPool->ptThrdQue[i]));

		s32Rtn = CreateThrd(&tThrdAttr, &(ptThrdPool->ptThrdQue[i].tThrdID), ThrdWork, &(ptThrdPool->ptThrdQue[i]));
		if (0 != s32Rtn)
		{
			ptThrdPool->u32TotalNum = i + 1;
			ptThrdPool->ptThrdQue[i].u32State = TP_THREAD_DEAD;

			goto ERR_OUT_WITH_MANAGEMENT;
		}
	}
	ptThrdPool->u32TotalNum = i;

	// add to pool link
	s32Rtn = PoolLinkAdd(ptThrdPool);
	if (0 != s32Rtn)
	{
		goto ERR_OUT_WITH_MANAGEMENT;
	}

	return (tThreadPoolDef *)ptThrdPool;

ERR_OUT_NO_MANAGEMENT:
	CleanThrdPool(ptThrdPool);
	free(ptThrdPool);
	return NULL;

ERR_OUT_WITH_MANAGEMENT:
	// delete the pool by managemnet thread
	pthread_mutex_lock(&(ptThrdPool->tLock));
	ptThrdPool->u32RunState = TP_POOL_DESTROY;
	pthread_mutex_unlock(&(ptThrdPool->tLock));
	// return NULL, the destroy work will be finished by management thread
	return NULL;
}

/*********************************************************************
 * ����������ɾ���̳߳�
 * ���������ptThrdPool - ��ɾ���̳߳ص�ָ��
 * �����������
 * �� �� ֵ���ɹ� - ����0; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

signed int DeleteThrdPool(tThreadPoolDef *ptThreadPool)
{
	signed int s32Rtn;
	tThrdPoolDef *ptThrdPool;

	if (NULL == ptThreadPool)
	{
		return -1;
	}
	ptThrdPool = (tThrdPoolDef *)ptThreadPool;

	s32Rtn = PoolLinkDel(ptThrdPool);
	if (0 != s32Rtn)
	{
		return -1;
	}

	pthread_mutex_lock(&(ptThrdPool->tLock));
	ptThrdPool->u32RunState = TP_POOL_DESTROY;
	pthread_mutex_unlock(&(ptThrdPool->tLock));

	return 0;
}

/*********************************************************************
 * ���������������û�����
 * ���������ptThrdPool - �̳߳ص�ָ��
 *           pUserFunc - �û�����
 *           pUserarg - �û�����
 *           pu32UserID - �����û�������̵߳�ID
 * �����������
 * �� �� ֵ���ɹ� - ����0; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

signed int AddWork(tThreadPoolDef *ptThreadPool,
				   void (*pUserFunc)(void *pUserArg, const unsigned int *pu32IsDel),
				   void *pUserarg,
				   unsigned int *pu32UserID)
{
	tThrdPoolDef *ptThrdPool;
	signed int s32Rtn;
	unsigned int i;

	//if ((NULL == ptThreadPool) || (NULL == pUserFunc) || (NULL == pUserarg) || (NULL == pu32UserID))
	if ((NULL == ptThreadPool) || (NULL == pUserFunc) || (NULL == pu32UserID))
	{

		return -1;
	}
	ptThrdPool = (tThrdPoolDef *)ptThreadPool;

	s32Rtn = pthread_mutex_lock(&(ptThrdPool->tLock));
	if (0 != s32Rtn)
	{
		perror("pthread mutex lock error");
		return -1;
	}
	ptThrdPool = PoolLinkSearch(ptThrdPool);
	if (NULL == ptThrdPool)
	{

		pthread_mutex_unlock(&(ptThrdPool->tLock));
		return -1;
	}

	*pu32UserID = 0;
	// get a unique thread ID
	do
	{
		ptThrdPool->u32AddCntr++;
		if (0 == ptThrdPool->u32AddCntr)
		{
			ptThrdPool->u32AddCntr++;
		}

		for (i = 0; i < ptThrdPool->u32TotalNum; i++)
		{
			pthread_mutex_lock(&(ptThrdPool->ptThrdQue[i].tLock));
			if (ptThrdPool->u32AddCntr == ptThrdPool->ptThrdQue[i].u32UserID)
			{
				pthread_mutex_unlock(&(ptThrdPool->ptThrdQue[i].tLock));
				break;
			}
			pthread_mutex_unlock(&(ptThrdPool->ptThrdQue[i].tLock));
		}
	} while (i < ptThrdPool->u32TotalNum);

	// get a idle thread
	for (i = 0; i < ptThrdPool->u32TotalNum; i++)
	{
		s32Rtn = pthread_mutex_lock(&(ptThrdPool->ptThrdQue[i].tLock));
		if (TP_THREAD_IDLE == ptThrdPool->ptThrdQue[i].u32State)
		{
			ptThrdPool->ptThrdQue[i].pUserFunc = pUserFunc;
			ptThrdPool->ptThrdQue[i].pUserArg = pUserarg;
			ptThrdPool->ptThrdQue[i].u32UserID = ptThrdPool->u32AddCntr;
			*pu32UserID = ptThrdPool->u32AddCntr;
			ptThrdPool->ptThrdQue[i].u32State = TP_THREAD_USED;
			pthread_mutex_unlock(&(ptThrdPool->ptThrdQue[i].tLock));
			pthread_cond_signal(&(ptThrdPool->ptThrdQue[i].tCond));
			break;
		}
		pthread_mutex_unlock(&(ptThrdPool->ptThrdQue[i].tLock));
	}
	pthread_mutex_unlock(&(ptThrdPool->tLock));

	if (i == ptThrdPool->u32TotalNum)
	{
		s32Rtn = -1;
	}
	else
	{
		s32Rtn = 0;
	}

	return s32Rtn;
}

/*********************************************************************
 * ����������ж���û�����
 * ���������ptThrdPool - �̳߳ص�ָ��
 *           u32UserID - �����û�������̵߳�ID
 * �����������
 * �� �� ֵ���ɹ� - ����0; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

signed int DeleteWork(tThreadPoolDef *ptThreadPool, const unsigned int u32UserID)
{
	tThrdPoolDef *ptThrdPool;
	tThrdInfoDef *ptThrdInfo;
	signed int s32Rtn;
	unsigned int i;

	if ((NULL == ptThreadPool) || (0 == u32UserID))
	{
		return -1;
	}
	ptThrdPool = (tThrdPoolDef *)ptThreadPool;

	s32Rtn = pthread_mutex_lock(&(ptThrdPool->tLock));
	if (0 != s32Rtn)
	{
		return -1;
	}
	ptThrdPool = PoolLinkSearch(ptThrdPool);
	if (NULL == ptThrdPool)
	{
		pthread_mutex_unlock(&(ptThrdPool->tLock));
		return -1;
	}

	for (i = 0; i < ptThrdPool->u32TotalNum; i++)
	{
		ptThrdInfo = &(ptThrdPool->ptThrdQue[i]);
		pthread_mutex_lock(&(ptThrdInfo->tLock));
		if ((u32UserID == ptThrdInfo->u32UserID) && (TP_THREAD_USED == ptThrdInfo->u32State))
		{
			pthread_mutex_unlock(&(ptThrdInfo->tLock));
			break;
		}
		pthread_mutex_unlock(&(ptThrdInfo->tLock));
	}
	pthread_mutex_unlock(&(ptThrdPool->tLock));

	if (i != ptThrdPool->u32TotalNum)
	{
		s32Rtn = pthread_mutex_lock(&(ptThrdInfo->tLock));
		if (0 != s32Rtn)
		{
			return -1;
		}
		if ((u32UserID == ptThrdInfo->u32UserID) &&
			(TP_THREAD_USED == ptThrdInfo->u32State))
		{
			ptThrdInfo->u32State = TP_THREAD_FUNC_DEL;
			while (TP_THREAD_FUNC_DEL == ptThrdInfo->u32State)
			{
				pthread_cond_wait(&(ptThrdInfo->tCond), &(ptThrdInfo->tLock));
			}
		}
		pthread_mutex_unlock(&(ptThrdInfo->tLock));
	}

	return 0;
}

/*********************************************************************
 * ����������ж���û�����
 * ���������ptThrdPool - �̳߳ص�ָ��
 *           u32UserID - �����û�������̵߳�ID
 * �����������
 * �� �� ֵ���ɹ� - ����0; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

signed int DeleteWorkWithoutWait(tThreadPoolDef *ptThreadPool, unsigned int u32UserID)
{
	tThrdPoolDef *ptThrdPool;
	signed int s32Rtn;
	unsigned int i;

	if ((NULL == ptThreadPool) || (0 == u32UserID))
	{
		return -1;
	}
	ptThrdPool = (tThrdPoolDef *)ptThreadPool;

	s32Rtn = pthread_mutex_lock(&(ptThrdPool->tLock));
	if (0 != s32Rtn)
	{
		return -1;
	}
	ptThrdPool = PoolLinkSearch(ptThrdPool);
	if (NULL == ptThrdPool)
	{
		pthread_mutex_unlock(&(ptThrdPool->tLock));
		return -1;
	}

	for (i = 0; i < ptThrdPool->u32TotalNum; i++)
	{
		pthread_mutex_lock(&(ptThrdPool->ptThrdQue[i].tLock));
		if ((u32UserID == ptThrdPool->ptThrdQue[i].u32UserID) &&
			(TP_THREAD_USED == ptThrdPool->ptThrdQue[i].u32State))
		{
			ptThrdPool->ptThrdQue[i].u32State = TP_THREAD_FUNC_DEL;
			pthread_mutex_unlock(&(ptThrdPool->ptThrdQue[i].tLock));
			break;
		}
		pthread_mutex_unlock(&(ptThrdPool->ptThrdQue[i].tLock));
	}
	pthread_mutex_unlock(&(ptThrdPool->tLock));

	return 0;
}

/*********************************************************************
 * ������������ȡ�̳߳ؿ����߳���
 * ���������ptThrdPool - �̳߳ص�ָ��
 * �����������
 * �� �� ֵ���ɹ� - ���ؿ����߳���; ʧ�� - ����-1
 * ����˵����
 *
 * �޸ļ�¼1��
 *    �޸����ڣ�
 *    �� �� �ţ�
 *    �� �� �ˣ�
 *    �޸����ݣ� 
 *********************************************************************/

unsigned int GetIdleThrdNum(tThreadPoolDef *ptThreadPool)
{
	tThrdPoolDef *ptThrdPool;
	signed int s32Rtn;
	unsigned int u32IdleNum;
	unsigned int i;

	if (NULL == ptThreadPool)
	{
		return 0;
	}
	ptThrdPool = (tThrdPoolDef *)ptThreadPool;
	u32IdleNum = 0;

	s32Rtn = pthread_mutex_lock(&(ptThrdPool->tLock));
	if (0 != s32Rtn)
	{
		return 0;
	}
	ptThrdPool = PoolLinkSearch(ptThrdPool);
	if (NULL == ptThrdPool)
	{
		pthread_mutex_unlock(&(ptThrdPool->tLock));
		return 0;
	}

	for (i = 0, u32IdleNum = 0; i < ptThrdPool->u32TotalNum; i++)
	{
		pthread_mutex_lock(&(ptThrdPool->ptThrdQue[i].tLock));
		if (TP_THREAD_IDLE == ptThrdPool->ptThrdQue[i].u32State)
		{
			u32IdleNum++;
		}
		pthread_mutex_unlock(&(ptThrdPool->ptThrdQue[i].tLock));
	}
	pthread_mutex_unlock(&(ptThrdPool->tLock));

	return u32IdleNum;
}

tThreadPoolDef *InitTheadPool(void)
{
	tThrdAttrDef tThrdAttr;
	tThreadPoolDef *ptThrdPool;

	tThrdAttr.s32ThrdPri = 0;			  // 2;
	tThrdAttr.s32ThrdSched = SCHED_OTHER; // SCHED_RR;
	tThrdAttr.u32StackSize = THREAD_STACK_SIZE;

	ptThrdPool = CreateThrdPool(MAX_USE_THREAD_NUM, &tThrdAttr);
	if (NULL == ptThrdPool)
	{

		return 0;
	}

	return (tThreadPoolDef *)ptThrdPool;
}
