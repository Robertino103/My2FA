#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

#define MAX_BUFFER_LEN 200
#define MAX_APP_LEN 50
#define PORT 3103
#define PREDEFAPP "Tinder"

extern int errno;

int main()
{
    char appname[MAX_APP_LEN] = " ";
    strcpy(appname, PREDEFAPP);
    char _2fa_opt[MAX_BUFFER_LEN] = " ";
    struct sockaddr_in server;
    struct sockaddr_in from;

    int sd;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server:] Error creating socket.\n");
        return errno;
    }

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server:] Error occured while binding to socket descriptor.\n");
        return errno;
    }

    if (listen(sd, 2) == -1)
    {
        perror("[server:] Error occured at listen() call.\n");
        return errno;
    }

    while (1)
    {
        int client;
        int length = sizeof(from);
        printf("[server:] Awaiting at %d port...", PORT);
        fflush(stdout);

        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[server:] Error accepting client connection.");
            return errno;
        }
        bzero(_2fa_opt, MAX_BUFFER_LEN);
        strcpy(_2fa_opt, "Auth into ");
        strcat(_2fa_opt, appname);
        strcat(_2fa_opt, " app.\n1. Send notification in 2FA app.\n2. Enter 2FA code manually.\n[1/2]\n");

        if (write(client, _2fa_opt, MAX_BUFFER_LEN) <= 0)
        {
            perror("[server:] Error writing to client.\n");
            continue;
        }

    }
}