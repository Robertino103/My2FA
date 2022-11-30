#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#define MAX_BUFFER_LEN 200
#define _2FA_PORT 2050
#define MAX_2FA_BUFFER 10
#define NO_THREADS 2

extern int errno;

static void *treat(void *);

typedef struct
{
    pthread_t idThread;
    int thCount;
} Thread;

Thread *threadsPool;
int _2fa_sd;
int nthreads;
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

void respond(int cl, int idThread);
int firstclient;
char firstclientmsg[MAX_BUFFER_LEN] = "";

int ok = 0;
int mok = 0;

int main()
{
    struct sockaddr_in server;

    void threadCreate(int);
    nthreads = NO_THREADS;
    threadsPool = calloc(sizeof(Thread), nthreads);

    if ((_2fa_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[2fa_server:] Error creating socket.\n");
        return errno;
    }

    int on = 1;
    setsockopt(_2fa_sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(_2FA_PORT);

    if (bind(_2fa_sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[2fa_server:] Error occured while binding to socket descriptor.\n");
        return errno;
    }

    if (listen(_2fa_sd, 2) == -1)
    {
        perror("[2fa_server:] Error occured at listen() call.\n");
        return errno;
    }

    printf("Creating %d threads...\n", nthreads);
    fflush(stdout);
    int i;
    for (i = 0; i < nthreads; i++)
    {
        threadCreate(i);
    }

    for (;;)
    {
        printf("[2fa_server:] Awaiting at port %d...\n", _2FA_PORT);
        pause();
    }

    if (write(_2fa_sd, &firstclientmsg, strlen(firstclientmsg)) <= 0)
    {
        perror("Error writing message to 2FA client\n");
        return errno;
    }
};

void threadCreate(int i)
{
    void *treat(void *);

    pthread_create(&threadsPool[i].idThread, NULL, &treat, (void *)i);
    return;
}

void *treat(void *arg)
{
    int client;
    struct sockaddr_in from;
    bzero(&from, sizeof(from));
    printf("[thread]- %d - pornit...\n", (int)arg);
    fflush(stdout);

    for (;;)
    {
        int length = sizeof(from);
        pthread_mutex_lock(&mlock);
        if ((client = accept(_2fa_sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[2fa_thread:]Error accepting.\n");
        }
        else
        {
            if(ok==0)
            {
                firstclient = client;
                ok = 1;
            }
        }
        pthread_mutex_unlock(&mlock);
        threadsPool[(int)arg].thCount++;
        respond(client, (int)arg);

        //close(client);
    }
}

char msg[MAX_BUFFER_LEN];

void respond(int cl, int idThread)
{
    if (read(cl, &msg, sizeof(msg)) <= 0)
    {
        perror("[2fa_thread:] Error reading from a client\n");
    }
    printf("[2fa_server:] Message received : %s\n", msg);

    if((strncmp(msg, "1", 1) == 0 || strncmp(msg, "2", 1) == 0) && strlen(msg) <= 2)
    {
        write(firstclient, &msg, strlen(msg));
    }

}