#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <semaphore.h>

#include "list.h"

#define MAXLINE 1024

sem_t mutexSend;
sem_t mutexRec;
List *keyList;
char MESSAGE_OUT[40];
char MESSAGE_IN[40];

void *input(void *ptr)
{

    printf("Input Thread started\n");
    // printf("%d\n", List_count(keyboardList));

    char *lineBuffer = malloc(sizeof(char) * 40);
    // printf("LOCKING at INPUT\n");
    printf("Input: ");
    while (sem_wait(&mutexSend) == 0 && strcmp(fgets(lineBuffer, 40, stdin), "quit\n") != 0)
    {

        // printf("LOCKED at INPUT\n");

        strcpy(MESSAGE_OUT, lineBuffer);

        // printf("UNLOCKED at INPUT\n");
        //release resources
        sem_post(&mutexSend);
        sleep(1);
        printf("Input: ");
    }
    free(lineBuffer);
}

void *sending(void *ptr) // client
{
    sleep(2);
    
    // printf("Sending thread started\n");

    int PORT = *(int *)ptr;
    int sockfd;
    char buffer[MAXLINE];
    char *message = "Test message\n";
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    //server settings are done.

    // printf("LOCKING in SEND\n");

    while (sem_wait(&mutexSend) == 0 && strcmp(MESSAGE_OUT, "quit\n") != 0)
    {
        //waiting to send

        // printf("LOCKED in SEND\n");
        //message = List_remove(keyList);
        printf("Message to be sent: %s", MESSAGE_OUT);

        // printf("UNLOCKED in SEND \n");
        sem_post(&mutexSend);
        sleep(2);
        // realesed the resource
    }

    close(sockfd);
    return 0;
}

void *printing(void *from){

    while (strcmp(MESSAGE_IN, "quit\n") != 0 && strcmp(MESSAGE_IN, "") != 0)
    {
        printf("%s: %s", (char*)from, MESSAGE_IN);
    }
    
}

void *receiving(void *ptr) // server
{
    // printf("Receive thread %d \n", *(int *)ptr);
    int PORT = *(int *)ptr;
    int sockfd;
    char buffer[MAXLINE];
    
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Binded Succesfully\n");
    }

    
    int len, n;

    len = sizeof(cliaddr); //len is value/resuslt
    
    printf("Listening.. \n");
    while (n = recv(sockfd, (char *)buffer, MAXLINE, 0) == 0)
    {
        printf("Receiving message\n");
        buffer[n] = '\0';
        printf("Client : %s\n", buffer);
        
    }

    return 0;
}

int main(int argc, char const *argv[])
{
    sem_init(&mutexSend, 0, 1);
    sem_init(&mutexRec, 0, 1);
    pthread_t sender, recieve, keyboard, p4;
    int arg1, arg2, arg3, arg4;

    keyList = List_create();

    if (argc < 2)
    {
        printf("Error, no sockets given\n");
        exit(-1);
    }

    
    arg1 = atoi(argv[1]);
    arg2 = atoi(argv[2]);

    

    // printf("Server Port: %d\n", arg1); // server will recieve
    //printf("Client Port: %d\n", arg2); // client will send

    if (pthread_create(&keyboard, NULL, input, (void *)&arg3) == 0)
    {
        // printf("Created keyboard thread\n");
    }

    if (pthread_create(&recieve, NULL, receiving, (void *)&arg2) == 0)
    {
         printf("Created receive thread\n");
    }
    
    if (pthread_create(&sender, NULL, sending, (void *)&arg1) == 0)
    {
        // printf("Created sender thread\n");
    }

    pthread_join(sender, NULL);
    pthread_join(keyboard, NULL);
    pthread_join(recieve, NULL);

    sem_destroy(&mutexSend);
    sem_destroy(&mutexRec);

    return 0;
}
