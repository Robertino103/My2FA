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

#define _2FA_CODE_LEN 6
#define MAX_BUFFER_LEN 200

extern int errno;
int port;

int main(int argc, char *argv[])
{
    printf("Connecting to additional server...\n");
    int sd;
    struct sockaddr_in server;
    char _2fa_opt[MAX_BUFFER_LEN];

    if (argc != 3)
    {
        printf("Correct usage : %s <server ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[add_client:] Error creating socket.\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[add_client:] Error occured while trying to connect.\n");
        return errno;
    }

    bzero(_2fa_opt, MAX_BUFFER_LEN);
    if (read(sd, _2fa_opt, MAX_BUFFER_LEN) < 0)
    {
        perror("[add_client:] Error reading 2FA options from server.\n");
        return errno;
    }
    printf("%s\n", _2fa_opt);

    char optc = fgetc(stdin);
    if(optc - '0' != 1 && optc - '0' != 2) printf("[add_client:] Option not recognized! Please select one of the available options [1/2]!\n");
    int opt;

    bool repeat;
    do
    {
        opt = optc - '0';
        repeat = false;
        switch (opt)
        {
        case 1:
        {
            if (write(sd, "1", 1) <= 0)
            {
                perror("[add_client:] Error writing 2FA option to server.\n");
                return errno;
            }
        }
        break;

        case 2:
        {
            if (write(sd, "2", 1) <= 0)
            {
                perror("[add_client:] Error writing 2FA option to server.\n");
                return errno;
            }
        }
        break;

        default:
        {
            optc = fgetc(stdin);
            repeat = true;
        }
        break;
        }

    } while (repeat);

    
    char authaccess[MAX_BUFFER_LEN];
    if(read(sd, &authaccess, MAX_BUFFER_LEN) <= 0)
    {
        perror("[add_client:] Error reading auth response from server.\n");
        return errno;
    }

    authaccess[strlen(authaccess)] = '\0';
    printf("%s\n", authaccess);

    if(strncmp(authaccess, "Enter 2FA code", 14) == 0)
    {
        char _2fa_try[_2FA_CODE_LEN];

        fflush(stdin); fflush(stdout);
        scanf("%s", _2fa_try);

        if(write(sd, &_2fa_try, _2FA_CODE_LEN) <= 0)
        {
            perror("[add_client:] Error writing 2FA code.\n");
            return errno;
        }
        
        char manualauthaccess[MAX_BUFFER_LEN];
        if(read(sd, &manualauthaccess, MAX_BUFFER_LEN) <= 0)
        {
            perror("[add_client:] Error writing 2FA code.\n");
            return errno;
        }
        manualauthaccess[strlen(manualauthaccess)] = '\0';
        printf("%s", manualauthaccess);
    }
    
}