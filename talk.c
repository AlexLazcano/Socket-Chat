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
int status = false; // 0 = offline, 1 = online

sem_t mutexSend;
sem_t mutexRec;

const char *machine;

List *IncommingMSGs;
List *OutgoingMSGs;
char MESSAGE_OUT[MAXLINE];
char MESSAGE_IN[MAXLINE];
int EXITCODE = false;
char *ONLINE = "11-11\n";
char *OFFLINE = "10-10\n";

int isCode(char *string)
{
    if (strcmp(string, ONLINE) == 0)
    {
        status = true;
        return true;
    }

    else if (strcmp(string, OFFLINE) == 0)
    {
        status = false;
        return true;
    }
    else
    {
        return false;
    }
}

char *encrypt(char *message)
{

    char *newstring = malloc(strlen(message));

    int key = 20;
    // printf("Before: %s\n", message);

    for (int i = 0; i < strlen(message); i++)
    {
        char letter = message[i];
        letter = (letter + key) % 256;
        newstring[i] = letter;
    }

    // printf("After: %s\n", newstring);
    return newstring;
}

char *decrypt(char *message)
{
    char *newstring = malloc(strlen(message));

    int key = 256 - 20;

    for (int i = 0; i < strlen(message); i++)
    {
        char letter = message[i];
        letter = (letter + key) % 256;
        newstring[i] = letter;
    }

    return newstring;
}

void *input(void *ptr)
{

    // printf("Input Thread started\n");
    // printf("%d\n", List_count(keyboardList));

    char *lineBuffer = malloc(sizeof(char) * MAXLINE);
    // printf("LOCKING at INPUT\n");

    while (sem_wait(&mutexSend) == 0 && fgets(lineBuffer, MAXLINE, stdin))
    {
        // printf("LOCKED at INPUT\n");
        // printf("%s", lineBuffer);
        if (strcmp(lineBuffer, "!exit\n") == 0)
        {
            EXITCODE = true;
            sem_post(&mutexSend);
            break;
        }

        if (strcmp(lineBuffer, "!status\n") == 0)
        {
            if (status == true)
            {
                printf("Online\n");
            }
            else
            {
                printf("Offline\n");
            }

            strcpy(MESSAGE_OUT, "\0");
        }
        else
        {
            strcpy(MESSAGE_OUT, lineBuffer);
        }

        // printf("UNLOCKED at INPUT\n");
        //release resources
        sem_post(&mutexSend);
        sleep(1);
    }

    printf("exiting input\n");

    free(lineBuffer);
}

void *sending(void *ClientPort) // client
{

    // printf("Sending thread started\n");

    int PORT = *(int *)ClientPort;
    // printf("Sending to: %d\n",PORT );
    int sockfd;
    char buffer[MAXLINE];
    char *message;
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
    servaddr.sin_addr.s_addr = inet_addr(machine);

    //server settings are done.
    sleep(2);

    sendto(sockfd, (const char *)ONLINE, strlen(ONLINE), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    while (EXITCODE != true && sem_wait(&mutexSend) == 0)
    {
        // printf("LOCKED at SEND\n");
        //waiting to send

        // printf("Message to be sent: %s", MESSAGE_OUT);

        if (strcmp(MESSAGE_OUT, "") != 0)
        {

            message = encrypt(MESSAGE_OUT);
            sendto(sockfd, (const char *)message, strlen(MESSAGE_OUT), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        }

        // printf("UNLOCKED in SEND \n");
        sem_post(&mutexSend);

        sleep(2);
        // realesed the resource
    }

    printf("Exiting Sending\n");
    sendto(sockfd, (const char *)OFFLINE, strlen(OFFLINE), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

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
    while (EXITCODE != true && sem_wait(&mutexRec) == 0)
    {
        // printf("LOCKED AT PRINT\n");

        ////////////////////// crit

        char *ptr;
        char *word;
        if (List_count(IncommingMSGs) > 0)
        {
            // printf("count: |%d|\n", List_count(IncommingMSGs));
            List_first(IncommingMSGs);
            ptr = List_remove(IncommingMSGs); // valgrind ?
            List_last(IncommingMSGs);
            // printf("Pointer: |%s|\n", ptr);
            memccpy(word, ptr, '\0', MAXLINE);
            printf("client: %s\n", word);

            free(ptr);
        }
        fflush(stdout);

        ////////////////////
        // printf("UNLOCKED AT PRINT\n");

        sem_post(&mutexRec);
        sleep(2);
    }
    sem_post(&mutexRec);
    printf("Exiting Printing\n");
}

void storeMsg(char *string)
{

    if (strcmp(string, "\0") != 0)
    {

        char *decrypted = decrypt(string);

        // printf("storing new string: %s\n", string);

        List_append(IncommingMSGs, decrypted);
    }

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
        // printf("Binded Succesfully\n");
    }
    //finished socket set up

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        printf("Error in the making timeout\n");
    }

    int len, n;
    len = sizeof(cliaddr);
    // printf("Listening.. \n");
    // n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr*) &cliaddr, &len);
    // printf("Message received..\n");
    // buffer[n] = '\0';
    // printf("client: %s\n", buffer);

    // printf("WANT LOCK AT Rec\n");
    while (EXITCODE != true)
    {
        // printf("LOCKED at Rec\n");

        sem_wait(&mutexRec);
        n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        // printf("Receiving message\n");
        buffer[n] = '\0';
        buffer[n - 1] = '\0';

        if (EXITCODE == true)
        {
            sem_post(&mutexRec);
            break;
        }
        if (strcmp(buffer, "\0") == 0)
        {
            sem_post(&mutexRec);
            continue;
        }


        // printf("%s|\n", buffer);
        if (strcmp(buffer, "11-11") == 0)
        {
            status = true;
            // printf("iscode\n");
        }
        else if (strcmp(buffer, "10-10") == 0)
        {
            status = false;
            // printf("iscode\n");
        }

        else
        {
            // printf("Not code\n");
            storeMsg(buffer);
            buffer[0] = '\0';
        }

        // printf("UNLOCKING at REC\n");
        // int val;
        // sem_getvalue(&mutexRec, &val);
        // printf("%d\n", val);
        // printf("UNLOCKED at REC\n");

        sem_post(&mutexRec);

        // sem_getvalue(&mutexRec, &val);
        // printf("%d\n", val);
        sleep(1);
    }
    printf("Exiting Receiving\n");

    return 0;
}

int main(int argc, char const *argv[])
{
    sem_init(&mutexSend, 0, 1);
    sem_init(&mutexRec, 0, 1);

    pthread_t sender, recieve, keyboard, printer;
    int myPort, theirPort, arg3, arg4;

    IncommingMSGs = List_create();
    OutgoingMSGs = List_create();

    if (argc < 3)
    {
        printf("Error, no sockets given\n");
        exit(-1);
    }

    myPort = atoi(argv[1]);
    if (strcmp("localhost", argv[2]) == 0)
    {
        machine = "127.0.0.1";
    }
    else
    {
        machine = argv[2];
    }

    theirPort = atoi(argv[3]);

    // printf("Server Port: %d\n", arg1); // server will recieve
    //printf("Client Port: %d\n", arg2); // client will send
    printf("Talk program started\n");
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

    printf("Exit\n");
    return 0;
}
