#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "list.h"

#define MAXLINE 1024



void *input(void *keyboardList)
{
    printf("%d\n", List_count(keyboardList));

    printf("list %p \n", keyboardList);

    printf("Input thread for keybaord\n");
    char lineBuffer[4000] = {};

    while (strcmp(fgets(lineBuffer, 4000, stdin), "quit\n") != 0)
    {

        printf("string size: %ld string: %s", strlen(lineBuffer), lineBuffer);
    }
}

void *receiving(void *ptr) // server
{
    printf("Reveive thread %d \n", *(int *)ptr);
    int PORT = *(int *)ptr;
    int sockfd;
    char buffer[MAXLINE];
    char *hello = "Hello from server";
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

    while (n = recv(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL) == 0)
    {
        buffer[n] = '\0';
        printf("Client : %s\n", buffer);
    }

    return 0;
}

void *sending(void *ptr) // client
{
    printf("Sender thread: %d \n", *(int *)ptr);

    int PORT = *(int *)ptr;
    int sockfd;
    char buffer[MAXLINE];
    char *hello = "Hello this is a test";
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

    int n, len;

    sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

    close(sockfd);
    return 0;
}

int main(int argc, char const *argv[])
{
    pthread_t sender, recieve, keyboard, p4;
    int arg1, arg2, arg3, arg4;
    
    List *KeyList = List_create();
    
    if (argc < 2)
    {
        printf("Error, no sockets given\n");
        exit(-1);
    }

    arg1 = atoi(argv[1]);
    arg2 = atoi(argv[2]);
    

    printf("Server Port: %d\n", arg1); // server will recieve
    printf("Client Port: %d\n", arg2); // client will send
  

    if (pthread_create(&keyboard, NULL, input, (void *)&KeyList) == 0)
    {
        printf("Created keyboard thread\n");
    }

    if (pthread_create(&sender, NULL, sending, (void *)&arg1) == 0)
    {
        printf("Created sender thread\n");
    }

    if (pthread_create(&recieve, NULL, receiving, (void *)&arg2) == 0)
    {
        printf("Created reveive thread\n");
    }

    pthread_join(keyboard, NULL);
    pthread_join(sender, NULL);
    pthread_join(recieve, NULL);

    return 0;
}
