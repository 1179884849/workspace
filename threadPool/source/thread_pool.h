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

// 线程的5个状态: idle, used, delete, exit, dead
#define TP_THREAD_IDLE (1)	 // idle, 线程空闲, 可随时加载用户程序
#define TP_THREAD_USED (2)	 // used, 线程已加载用户程序
#define TP_THREAD_FUNC_DEL (3) // delete, 删除用户程序, 用户检测到该标志, 需主动退出用户程序
#define TP_THREAD_EXIT (4)	 // exit, 线程已退出, 可以删除/回收线程资源
#define TP_THREAD_DEAD (0)	 // dead, 线程已完全退出, 或者未创建

#define MAX_USE_THREAD_NUM (20)
#define THREAD_STACK_SIZE (0x200000) // 0x200000 - 2Mbyte; 0x800000 - 8MByte

// 线程池内线程的属性
typedef struct tThrdAttrDef
{
	unsigned int u32StackSize; // 线程栈大小
	signed int s32ThrdPri;	 // 线程优先级
	signed int s32ThrdSched;   // 线程调度策略
} tThrdAttrDef;

// 线程池对外结构体接口
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
