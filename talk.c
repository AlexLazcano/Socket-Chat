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

#define MAXLINE 40

sem_t mutexSend;
sem_t mutexRec;
List *keyList;
List *IncommingMSGs;
char MESSAGE_OUT[MAXLINE];
char MESSAGE_IN[MAXLINE];

void *input(void *ptr)
{

    printf("Input Thread started\n");
    // printf("%d\n", List_count(keyboardList));

    char *lineBuffer = malloc(sizeof(char) * MAXLINE);
    // printf("LOCKING at INPUT\n");

    while (sem_wait(&mutexSend) == 0 && strcmp(fgets(lineBuffer, MAXLINE, stdin), "quit\n") != 0)
    {

        // printf("LOCKED at INPUT\n");

        strcpy(MESSAGE_OUT, lineBuffer);

        // printf("UNLOCKED at INPUT\n");
        //release resources
        sem_post(&mutexSend);
        sleep(1);
    }
    free(lineBuffer);
}

void *sending(void *ClientPort) // client
{

    // printf("Sending thread started\n");

    int PORT = *(int *)ClientPort;
    // printf("Sending to: %d\n",PORT );
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
    sleep(2);
    // printf("LOCKING in SEND\n");

    while (sem_wait(&mutexSend) == 0 && strcmp(MESSAGE_OUT, "quit\n") != 0)
    {
        //waiting to send

        // printf("LOCKED in SEND\n");
        //message = List_remove(keyList);

        // printf("Message to be sent: %s", MESSAGE_OUT);
        sendto(sockfd, (const char *)MESSAGE_OUT, strlen(MESSAGE_OUT), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

        // printf("UNLOCKED in SEND \n");
        sem_post(&mutexSend);
        sleep(2);
        // realesed the resource
    }

    close(sockfd);
    return 0;
}

int getMsg()
{

    char *ptr;
    char *word;

    List_first(IncommingMSGs);
    ptr = List_remove(IncommingMSGs);
    List_last(IncommingMSGs);

    memcpy(word, ptr, MAXLINE);
    printf("%s\n", word);
    free(ptr);
    return 0;
}

void *printing(void *recList)
{
    sleep(2);

    // int val;
    // sem_getvalue(&mutexRec, &val);
    // printf("sem value: %d\n", val);
    // printf("WANT LOCK AT PRINT\n");
    while (sem_wait(&mutexRec) == 0)
    {
        // printf("LOCKED AT PRINT\n");

        ////////////////////// crit

        char *ptr;
        char* word;
        List_first(IncommingMSGs);
        ptr = List_remove(IncommingMSGs);
        List_last(IncommingMSGs);

        memcpy(word, ptr, MAXLINE);
        printf("client: %s\n", word);
        free(ptr);

        ////////////////////
        // printf("UNLOCKED AT PRINT\n");
        sem_post(&mutexRec);
        sleep(3);
    }
}

void storeMsg(char *string)
{

    int n = strlen(string);

    char *newString = malloc(n);

    memcpy(newString, string, n);
    // printf("storing new string: %s\n", newString);

    List_append(IncommingMSGs, newString);

    return;
}

void *receiving(void *ServerPort) // server
{
    int PORT = *(int *)ServerPort;
    // printf("Receive thread %d \n", PORT);
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
    //finished socket set up

    int len, n;
    len = sizeof(cliaddr);
    printf("Listening.. \n");
    // n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr*) &cliaddr, &len);
    // printf("Message received..\n");
    // buffer[n] = '\0';
    // printf("client: %s\n", buffer);
    int val;
    sem_getvalue(&mutexRec, &val);
    printf("sem value: %d\n", val);

    printf("WANT LOCK AT Rec\n");
    while (sem_wait(&mutexRec) == 0)
    {

        // printf("LOCKED at Rec\n");

        n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        // printf("Receiving message\n");
        buffer[n] = '\0';
        buffer[n-1] = '\0';

        storeMsg(buffer);
        // printf("UNLOCKING at REC\n");

        // printf("UNLOCKED at REC\n");
        sem_post(&mutexRec);
        

        sleep(2);

        if (strcmp(buffer, "quit") == 0)
        {
            break;
        }
    }
    printf("Exiting listener\n");

    return 0;
}

int main(int argc, char const *argv[])
{
    sem_init(&mutexSend, 0, 1);
    sem_init(&mutexRec, 0, 1);
    pthread_t sender, recieve, keyboard, printer;
    int myPort, theirPort, arg3, arg4;

    IncommingMSGs = List_create();
    keyList = List_create();

    if (argc < 2)
    {
        printf("Error, no sockets given\n");
        exit(-1);
    }

    myPort = atoi(argv[1]);
    theirPort = atoi(argv[2]);

    // printf("Server Port: %d\n", arg1); // server will recieve
    //printf("Client Port: %d\n", arg2); // client will send

    if (pthread_create(&keyboard, NULL, input, (void *)&arg3) == 0)
    {
        // printf("Created keyboard thread\n");
    }

    if (pthread_create(&recieve, NULL, receiving, (void *)&myPort) == 0)
    {
        // printf("Created receive thread\n");
    }

    if (pthread_create(&sender, NULL, sending, (void *)&theirPort) == 0)
    {
        // printf("Created sender thread\n");
    }
    if (pthread_create(&printer, NULL, printing, (void *)&theirPort) == 0)
    {
        // printf("Created sender thread\n");
    }

    pthread_join(sender, NULL);
    pthread_join(keyboard, NULL);
    pthread_join(recieve, NULL);
    pthread_join(printer, NULL);

    sem_destroy(&mutexSend);
    sem_destroy(&mutexRec);

    return 0;
}
