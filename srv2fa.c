#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

#define _2FA_PORT 2050
#define MAX_2FA_BUFFER 10

extern int errno;

int main()
{
    struct sockaddr_in server;
    struct sockaddr_in from;

    int _2fa_sd;

    if ((_2fa_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[2fa_server:] Error creating socket.\n");
        return errno;
    }

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

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
    
    while(1)
    {
        int add_srv;
        int length = sizeof(from);
        fflush(stdout);
        add_srv = accept(_2fa_sd, (struct sockaddr *)&from, &length);
        if(add_srv < 0)
        {
            perror("[2fa_server:] Error accepting additional server");
            continue;
        }

        char add_srv_message[MAX_2FA_BUFFER];
        bzero(add_srv_message, MAX_2FA_BUFFER);
        if (read(add_srv, add_srv_message, MAX_2FA_BUFFER) <= 0)
        {
            perror("[2fa_server:] Error reading from additional server");
            close(add_srv);
            continue;
        }
        printf("COAIE : %s", add_srv_message);
        close(add_srv);
    }
    
}