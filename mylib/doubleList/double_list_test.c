/*
 * @Author: zyb
 * @Date: 2020-03-11 09:50:10
 * @LastEditTime: 2020-03-22 18:10:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /testlib/double_list_test.c
 */

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "errno.h"
#include <string.h>
#include "./include/doublelink.h"

typedef struct School
{
    char schoolName[20];
    tStdListDef pStdList;
} tSchool, *pSchool;

typedef struct Student
{
    DECLARELISTNODE();
    int age;
} tStudent, *pStudent;

tSchool school;

void testDoubleLsit()
{
    int i, j;
    int s32Ret;

    memset(&school, 0, sizeof(tStudent));
    sprintf(&school.schoolName, "faMengHighSchool");

    for (i = 0; i < 10; i++)
    {
        pStudent s;
        s = (pStudent)malloc(sizeof(tStudent));
        s->age = i;
        StdListPushBack(&school.pStdList, (ptStdNodeDef)s);
    }

    for (j = 0; j < 10; j++)
    {
        pStudent t;
        t = (pStudent)StdListPopBack(&school.pStdList);
        printf("%d\n", t->age);
        free(t);
    }
}

int main()
{
    int i;
    printf("hello\n");

    testDoubleLsit();
    return 0;
}