/*
 * @Description: 线程池实现
 * @Version: 1.0
 * @Author: Yuanbo Zhang
 * @Date: 2020-03-06 11:05:18
 * @LastEditors: Please set LastEditors
 * InitTheadPool 原本返回值为 unsigned int，最后强制转换为 void* 地址，int 本身为4字节，
 * 强制转换的地址也为 4 字节，运行在64位处理器上，会产生内存访问错误，因为64为处理器地址为8字节 
 * 如果进行交叉编译，而目标设备是32位的话，需要转换成unsigned int 类型
 * @LastEditTime: 2020-03-29 13:48:21
 */
/*****************************************************************************
 *
 * Thread Pool / 线程池, 提供如下函数接口
 *   CreateThrdPool : 创建线程池, 当系统无法创建所需数目线程时, 返回失败
 *   DeleteThrdPool : 删除线程池
 *   AddWork : 加载用户程序
 *   DeleteWork : 删除用户程序, 等待删除完成后返回
 *   DeleteWorkWithouWait : 删除用户程序, 不等用户程序退出, 由管理线程负责管理
 *
 *
 * - 创建的所有线程均是分离属性(detach)
 * - 所有互斥锁均是普通互斥锁(PTHREAD_MUTEX_INITIALIZER)
 * - 线程调度策略, 优先级, 都可由用户设置
 *
 *   PTHREAD_MUTEX_INITIALIZER : 普通锁, 再次加锁-死锁; 无锁时解锁-无意义; 
 *   PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP : 检测锁, 再次加锁-返回错误; 无锁时解锁-返回错误;
 *   PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP : 递归锁, 再次加锁-允许; 无锁时解锁-返回错误;
 *
 *   SCHED_OTHER : 分时调度策略, 不支持优先级设置
 *   SCHED_FIFO	: 实时调度策略, 先到先服务, 一旦占用cpu则一直运行, 直到被更高优先级线程抢占或自己退出
 *   SCHED_RR	: 实时调度策略, 时间片轮转, 当线程时间片用完, 系统重新分配时间片, 并将线程置于就绪队列尾, 保证了所有具有相同优先级线程的调度公平
 *
 *****************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "thread_pool.h"

#define TP_POOL_CREATE (0)	// created, 线程池状态, 线程池已创建, 状态正常
#define TP_POOL_DESTROY (1) // destroyed, 线程池状态, 线程池将退出

#define TP_STACK_THRD_MAX_SIZE (0x01000000) // 16M, 线程最大栈大小
#define TP_STACK_THRD_MIN_SIZE (0x00020000) // 100K, 线程最小栈大小

// 线程信息
typedef struct tThrdInfoDef
{
	void *pUserArg;			// 用户参数指针
	unsigned int u32UserID; // 用户ID, 用户程序身份识别ID
	void (*pUserFunc)(void *pUserArg, const unsigned int *pu32IsDel);
	// 用户程序, UserFunc(void *pUserarg, const unsigned int *pu32IsDel)
	// void *pUserArg, 用户参数指针
	// const unsigned int *pu32IsDel, 用户程序退出标志,
	// if (TP_THREAD_FUNC_DEL == *pu32IsDel), 用户程序需主动退出

	pthread_t tThrdID;

	unsigned int u32State; // 线程状态: idle, used, delete, exit, dead
	pthread_mutex_t tLock;
	pthread_cond_t tCond;
} tThrdInfoDef;

// 线程池
typedef struct tThrdPoolDef
{
	struct tThrdPoolDef *ptNext; // 指针指向下一线程池

	pthread_t tThrdID; // 管理线程ID

	unsigned int u32TotalNum; // 池内线程总数
	unsigned int u32AddCntr;  // 线程池被加载用户函数的次数
	unsigned int u32RunState; // 线程池状态: created or destroyed
	tThrdAttrDef tThrdAttr;	  // 池内线程属性

	pthread_mutex_t tLock;

	tThrdInfoDef ptThrdQue[0]; // 线程池
} tThrdPoolDef;

// 线程池链表头指针, 初始化为NULL
static tThrdPoolDef *ptTPLinkHead = NULL;

// 线程池链表互斥锁, 初始化为普通互斥锁, 编译时需添加左边选项 : -lpthread -D_GNU_SOURCE
static pthread_mutex_t tTPLinkLock = PTHREAD_MUTEX_INITIALIZER;

/*********************************************************************
 * 功能描述：向线程池链表中添加一个线程池节点, 添加到链表头部
 * 输入参数：ptThrdPool - 待添加线程池节点指针
 * 输出参数：无
 * 返 回 值：成功 - 返回0; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：从线程池链表中删除指定线程池节点
 * 输入参数：ptPoolDel - 待删除线程池指针
 * 输出参数：无
 * 返 回 值：成功 - 返回0; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：在线程池链表中查找指定线程池
 * 输入参数：ptPoolDel - 待查找线程池指针
 * 输出参数：无
 * 返 回 值：成功 - 返回线程池指针; 失败 - 返回NULL
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：初始化线程信息结构体
 * 输入参数：ptThrdInfo - 线程信息结构体
 * 输出参数：无
 * 返 回 值：成功 - 返回0; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：清除线程信息结构体
 * 输入参数：ptThrdInfo - 线程信息结构体
 * 输出参数：无
 * 返 回 值：成功 - 返回0; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：初始化线程池信息结构体
 * 输入参数：ptThrdPool - 线程池信息结构体
 * 输出参数：无
 * 返 回 值：成功 - 返回0; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：清除线程池信息结构体
 * 输入参数：ptThrdPool - 线程池信息结构体
 * 输出参数：无
 * 返 回 值：成功 - 返回0; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：管理线程
 * 输入参数：pvArg - 线程池信息结构体
 * 输出参数：无
 * 返 回 值：无
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：工作线程
 * 输入参数：pvArg - 线程信息结构体
 * 输出参数：无
 * 返 回 值：无
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：创建线程
 * 输入参数：ptAttr - 线程属性参数指针
 *           ptThrdID - 待创建线程ID指针
 *           pvFunc - 待创建线程函数
 *           pvPara - 待创建线程函数入口参数
 * 输出参数：无
 * 返 回 值：成功 - 返回0; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：创建线程池
 * 输入参数：u32ThrdNum - 需创建的线程数
 *           ptAttr - 线程属性; 若该指针为NULL, 则使用默认属性
 * 输出参数：无
 * 返 回 值：成功 - 返回对应线程池指针; 失败 - 返回NULL
 * 其它说明：
 *           线程属性 :
 *               线程栈大小 : 默认8 MByte, 最大16MByte, 最小16KByte
 *               线程调度策略 : 默认SCHED_RR, 可以设置为SCHED_OTHER, SCHED_FIFO
 *               线程优先级 : SCHED_OTHER下为0; SCHED_RR/SCHED_FIFO, 默认1, 1-99
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：删除线程池
 * 输入参数：ptThrdPool - 待删除线程池的指针
 * 输出参数：无
 * 返 回 值：成功 - 返回0; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：加载用户程序
 * 输入参数：ptThrdPool - 线程池的指针
 *           pUserFunc - 用户程序
 *           pUserarg - 用户参数
 *           pu32UserID - 加载用户程序的线程的ID
 * 输出参数：无
 * 返 回 值：成功 - 返回0; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：卸载用户程序
 * 输入参数：ptThrdPool - 线程池的指针
 *           u32UserID - 加载用户程序的线程的ID
 * 输出参数：无
 * 返 回 值：成功 - 返回0; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：卸载用户程序
 * 输入参数：ptThrdPool - 线程池的指针
 *           u32UserID - 加载用户程序的线程的ID
 * 输出参数：无
 * 返 回 值：成功 - 返回0; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
 * 功能描述：获取线程池空闲线程数
 * 输入参数：ptThrdPool - 线程池的指针
 * 输出参数：无
 * 返 回 值：成功 - 返回空闲线程数; 失败 - 返回-1
 * 其它说明：
 *
 * 修改记录1：
 *    修改日期：
 *    版 本 号：
 *    修 改 人：
 *    修改内容： 
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
