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
#define _2FA_PORT 2050
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
        perror("[add_server:] Error creating socket.\n");
        return errno;
    }

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[add_server:] Error occured while binding to socket descriptor.\n");
        return errno;
    }

    if (listen(sd, 2) == -1)
    {
        perror("[add_server:] Error occured at listen() call.\n");
        return errno;
    }

    int _2fa_sd;
    struct sockaddr_in _2fa_server;
    if ((_2fa_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[add_server:] Error creating socket for communicating with 2FA server");
        return errno;
    }
    _2fa_server.sin_family = AF_INET;
    _2fa_server.sin_addr.s_addr = inet_addr("127.0.0.1");
    _2fa_server.sin_port = htons(_2FA_PORT);

    while (1)
    {
        int client;
        int length = sizeof(from);
        printf("[add_server:] Awaiting at %d port...\n", PORT);
        fflush(stdout);

        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[add_server:] Error accepting client connection.");
            return errno;
        }
        bzero(_2fa_opt, MAX_BUFFER_LEN);
        strcpy(_2fa_opt, "Auth into ");
        strcat(_2fa_opt, appname);
        strcat(_2fa_opt, " app.\n1. Send notification in 2FA app.\n2. Enter 2FA code manually.\n[1/2]\n");

        if (write(client, _2fa_opt, MAX_BUFFER_LEN) <= 0)
        {
            perror("[add_server:] Error writing to client.\n");
            continue;
        }

        char _2fa_response[MAX_BUFFER_LEN] = "";
        if (read(client, _2fa_response, 1) <= 0)
        {
            perror("[add_server:] Error reading 2FA response from client.\n");
            close(client);
            continue;
        }
        printf("Option %s chosen for 2FA auth\n", _2fa_response);

        printf("Sending option to 2FA server ... \n");
        /*struct sockaddr_in to_2fa_srv;
        bzero(&to_2fa_srv, sizeof(to_2fa_srv));
        to_2fa_srv.sin_family = AF_INET;
        to_2fa_srv.sin_addr.s_addr = htonl(INADDR_ANY);
        to_2fa_srv.sin_port = htons(_2FA_PORT);
        int s;
        sendto(s, _2fa_response, sizeof(_2fa_response), 0, (struct sockaddr*)&to_2fa_srv, sizeof(to_2fa_srv));
        */
        if (connect(_2fa_sd, (struct sockaddr *)&_2fa_server, sizeof(struct sockaddr)) == -1)
        {
            perror("[add_server:] Error connecting to 2FA server");
            return errno;
        }

        strcat(_2fa_response, "|");
        strcat(_2fa_response, appname);

        if(write(_2fa_sd, _2fa_response, sizeof(_2fa_response)) <= 0)
        {
            perror("[add_server:] Error writing to _2fa_sd.\n");
            return errno;
        }

    }
}