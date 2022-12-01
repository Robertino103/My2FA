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

    if(write(sd, "2FA Client connected", 20) <= 0)
    {
        perror("[2fa_client:] Error writing connect message.\n");
        return errno;
    }

    if(read(sd, msg_from_2fa_srv, MAX_BUFFER_LEN) <= 0)
    {
        perror("[2fa_client:] Error reading from 2FA server\n");
        return errno;
    }
    printf("%s\n", msg_from_2fa_srv);

    if(strncmp(msg_from_2fa_srv, "Are you trying to log in into", 29) == 0)
    {
        char optc_1 = fgetc(stdin);
        if ((optc_1 != 110) && (optc_1 != 121) && (optc_1 != 78) && (optc_1 != 89))
        {
            printf("[2fa_client:] Option not recognized! Please select one of the available options [y/n/Y/N]!\n");
        }
        int opt_1;

        bool repeat;
        do
        {
            opt_1 = optc_1;
            repeat = false;
            switch (opt_1)
            {
                case 110: case 78:
                {
                    if (write(sd, "N", 1) <= 0)
                    {
                        perror("[2fa_client:] Error responding with NO to 2FA server.\n");
                        return errno;
                    }
                }break;

                case 121: case 89:
                {
                    if (write(sd, "Y", 1) <= 0)
                    {
                        perror("[2fa_client:] Error responding with YES to 2FA server.\n");
                        return errno;
                    }
                }break;

                default:
                {
                    optc_1 = fgetc(stdin);
                    repeat = true;
                }break;
            }
        }while(repeat);
        
    }

    
    close(sd);

}