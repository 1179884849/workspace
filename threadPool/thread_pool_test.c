/*
 * @Description: 线程池测试主程序
 * @Version: 1.0
 * @Author: Yuanbo Zhang
 * @Date: 2020-03-06 08:49:56
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2020-03-06 11:24:24
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_pool.h"

tThreadPoolDef *pThreadPool = NULL;
void func1(void *pUserArg, const unsigned int *pu32IsDel)
{

    while (1)
    {
        printf("1111111111\n");
        sleep(1);
    }
}

void func2(void *pUserArg, const unsigned int *pu32IsDel)
{

    while (1)
    {
        printf("22222222\n");
        sleep(1);
    }
}

void func3(void *pUserArg, const unsigned int *pu32IsDel)
{
    unsigned int u32ThrdNum;

    while (1)
    {
        u32ThrdNum = GetIdleThrdNum(pThreadPool);
        printf("Idel thread number: %d\n", u32ThrdNum);
        sleep(1);
    }
}

int main(int argc, char *argv[])
{

    unsigned int userID1 = 0;
    signed int u32Ret;

    pThreadPool = (tThreadPoolDef *)InitTheadPool();
    if (NULL == pThreadPool)
    {
        return -1;
    }

    u32Ret = AddWork(pThreadPool, func1, NULL, &userID1);
    if (0 != u32Ret)
    {
        return -1;
    }
    u32Ret = AddWork(pThreadPool, func2, NULL, &userID1);
    if (0 != u32Ret)
    {
        return -1;
    }
    u32Ret = AddWork(pThreadPool, func3, NULL, &userID1);
    if (0 != u32Ret)
    {
        return -1;
    }

    while (1)
    {
        sleep(1);
    }

    return 0;
}
