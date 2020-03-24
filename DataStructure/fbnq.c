/*
 * @Description: 
 * @Version: 1.0
 * @Author: Yuanbo Zhang
 * @Date: 2020-03-09 20:48:18
 * @LastEditor: 
 * @LastEditTime: 2020-03-09 21:00:45
 */
#include <stdio.h>

int fib(int n)
{
    if (n < 2)
    {
        return (n == 0) ? 0 : 1;
    }

    return fib(n - 1) + fib(n - 2);
}

int fac(int n)
{
    if (n == 0)
    {
        return 1;
    }
    return n * fac(n - 1);
}

int main()
{
    int i;

    i = fac(3);

    printf("%d\n", i);
    return 0;
}