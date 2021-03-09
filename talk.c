#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "list.h"

void *print_msg(void *ptr)
{
    printf("Hello world %d \n", *(int *)ptr);
}

int main(int argc, char const *argv[])
{
    pthread_t p1, p2, p3, p4;
    int arg1, arg2, arg3, arg4;

    arg1 = 1;
    int first = pthread_create(&p1, NULL, print_msg, (void *)&arg1);
    printf("thread first : %d\n", first);

    arg2 = 2;
    int second = pthread_create(&p2, NULL, print_msg, (void *)&arg2);
    printf("thread second : %d\n", second);


    List *list1 = List_create();

    List_append(list1, &p1);


    pthread_join(p1, NULL);
    

    
    return 0;
}
