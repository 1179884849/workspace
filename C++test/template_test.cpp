/*
 * @Description: C++模板学习
 * @Version: 1.0
 * @Author: Yuanbo Zhang
 * @Date: 2020-03-08 10:07:59
 * @LastEditor: 
 * @LastEditTime: 2020-04-12 15:05:49
 */
#include <iostream>

using namespace std;

/* 普通函数定义 */
void swap(int &a, int &b)
{
    int tmp;
    tmp = a;
    a = b;
    b = tmp;
}
/* 模板定义 */
template <typename Anytype>
void swap(Anytype &a, Anytype &b)
{
    Anytype tmp;
    tmp = a;
    a = b;
    b = tmp;
}

/* 具体化定义 */
typedef struct job
{
    char name[20];
    int age;
} job;

template <>
void swap<job>(job &a, job &b)
{
    job tmp;
    tmp.age = a.age;
    a.age = b.age;
    b.age = tmp.age;
}

/* 处理最后情况，相当于递归的退出 */
void printX()
{
}

/* 参数不定模板 */
/* 递归分解，每一次将参数分解为1和其他。知道最后为0，调用下面的递归退出程序 */
template <typename T, typename... Types>
void printX(const T &firstArg, const Types &... args)
{
    cout << firstArg << endl;
    printX(args...);
}

int main()
{
    printX("aaa", 123, "assss");
}