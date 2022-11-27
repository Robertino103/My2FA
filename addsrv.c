#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 3103

extern int errno;

int main()
{
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
    server.sin_addr.s_addr = htonl;
    server.sin_port = htons;

    if (bind (sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server:] Error occured while binding to socket descriptor.\n");
        return errno;
    }

    if (listen (sd, 2) == -1)
    {
        perror("[server:] Error occured at listen() call.\n");
        return errno;
    }
    
}