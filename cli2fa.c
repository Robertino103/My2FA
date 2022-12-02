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
#define ASCII_y 121
#define ASCII_n 110
#define ASCII_Y 89
#define ASCII_N 78

extern int errno; // Error code for specific function calls
int port; // Connection port

int main(int argc, char *argv[])
{
    printf("====================================\n2FA Client\n====================================\n");
    
    int sd; //srv2fa socket descriptor
    struct sockaddr_in server;
    char msg_from_2fa_srv[MAX_BUFFER_LEN];
    
    /* check no of arguments */
    if (argc != 3)
    {
        printf("Correct usage : %s <server ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* setting the port from args */
    port = atoi(argv[2]);

    /* creating socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[2fa_client:] Error creating socket.\n");
        return errno;
    }

    /* filling the sockaddr struct for connection */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    /* connection to srv2fa */
    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[2fa_client:] Error occured while trying to connect.\n");
        return errno;
    }

    bzero(msg_from_2fa_srv, MAX_BUFFER_LEN);

    /* writing successfull connection message to 2FA server */
    if(write(sd, "2FA Client connected", 20) <= 0)
    {
        perror("[2fa_client:] Error writing connect message.\n");
        return errno;
    }

    /* reading 1st message from 2FA server [ message for confirmation auth method / the 2FA code for manual auth method ] */
    if(read(sd, msg_from_2fa_srv, MAX_BUFFER_LEN) <= 0)
    {
        perror("[2fa_client:] Error reading from 2FA server\n");
        return errno;
    }
    printf("%s\n", msg_from_2fa_srv);

    /* if the confirmation auth method is chosen : */
    if(strncmp(msg_from_2fa_srv, "Are you trying to log in into", 29) == 0)
    {
        char optc_1 = fgetc(stdin); // read the yes/no message from stdin
        // 
        if ((optc_1 != ASCII_n) && (optc_1 != ASCII_y) && (optc_1 != ASCII_N) && (optc_1 != ASCII_Y))
        {
            printf("[2fa_client:] Option not recognized! Please select one of the available options [y/n/Y/N]!\n");
        }
        int opt_1;

        bool repeat;
        do
        {
            opt_1 = optc_1;
            repeat = false;

            /* handling yes/no response */
            switch (opt_1)
            {
                case ASCII_n: case ASCII_N: // no :
                {
                    if (write(sd, "N", 1) <= 0)
                    {
                        perror("[2fa_client:] Error responding with NO to 2FA server.\n");
                        return errno;
                    }
                }break;

                case ASCII_y: case ASCII_Y: // yes :
                {
                    if (write(sd, "Y", 1) <= 0)
                    {
                        perror("[2fa_client:] Error responding with YES to 2FA server.\n");
                        return errno;
                    }
                }break;

                default: // If selected response is invalid -> repeat :
                {
                    optc_1 = fgetc(stdin);
                    repeat = true;
                }break;
            }

        }while(repeat);
        
    }

    close(sd); // closing socket descriptor
}