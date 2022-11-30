#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>

#define MAX_BUFFER_LEN 200

extern int errno;

int port;

int main(int argc, char *argv[])
{
    int sd;
    struct sockaddr_in server;
    char msg_from_2fa_srv[MAX_BUFFER_LEN];
    
    if (argc != 3)
    {
        printf("Correct usage : %s <server ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

     port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[2fa_client:] Error creating socket.\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[2fa_client:] Error occured while trying to connect.\n");
        return errno;
    }

    bzero(msg_from_2fa_srv, MAX_BUFFER_LEN);
    if(read(sd, msg_from_2fa_srv, MAX_BUFFER_LEN) < 0)
    {
        perror("[2fa_client:] Error reading from 2FA server\n");
        return errno;
    }
    printf("A ajuns: %s", msg_from_2fa_srv);

}