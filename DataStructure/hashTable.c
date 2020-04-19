/*
 * @Description: 
 * @Version: 1.0
 * @Author: Yuanbo Zhang
 * @Date: 2020-04-04 12:31:19
 * @LastEditor: Yuanbo Zhang
 * @LastEditTime: 2020-04-04 20:41:19
 */

#include <stdio.h>

int main()
{
    int nums[] = {5, 4, 3, 21, 2, 3};
    int numCount = sizeof(nums) / 4;
    int tmp;
    int i;
    int m, n;

    for (m = 0; m < numCount; m++)
    {
        for (n = 0; n < numCount; n++)
        {
            if (nums[n] > nums[m])
            {
                tmp = nums[m];
                nums[m] = nums[n];
                nums[n] = tmp;
            }
        }
    }
    for (i = 0; i < numCount; i++)
    {
        printf("%d\n", nums[i]);
    }
    return 0;
}