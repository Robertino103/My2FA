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

#define _2FA_CODE_LEN 6 // Length of the 2FA code
#define MAX_BUFFER_LEN 200

extern int errno; // Error code for specific function calls
int port; // Connection port

int main(int argc, char *argv[])
{
    printf("====================================\nAdditional Client\n====================================\n");
    printf("Connecting to additional server...\n");

    int sd; //addsrv socket descriptor
    struct sockaddr_in server;
    char _2fa_opt[MAX_BUFFER_LEN];

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
        perror("[add_client:] Error creating socket.\n");
        return errno;
    }

    /* filling the sockaddr struct for connection */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    /* connection to addsrv */
    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[add_client:] Error occured while trying to connect.\n");
        return errno;
    }

    /* reading the "select 2FA auth method" message from addsrv */
    bzero(_2fa_opt, MAX_BUFFER_LEN);
    if (read(sd, _2fa_opt, MAX_BUFFER_LEN) < 0)
    {
        perror("[add_client:] Error reading 2FA options from server.\n");
        return errno;
    }
    printf("%s\n", _2fa_opt);

    /* reading 2FA auth method from stdin */
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
        case 1: // If method '1' is selected :
        {
            if (write(sd, "1", 1) <= 0)
            {
                perror("[add_client:] Error writing 2FA option to server.\n");
                return errno;
            }
        }
        break;

        case 2: // If method '2' is selected :
        {
            if (write(sd, "2", 1) <= 0)
            {
                perror("[add_client:] Error writing 2FA option to server.\n");
                return errno;
            }
        }
        break;

        default: // If selected method is invalid -> repeat :
        {
            optc = fgetc(stdin);
            repeat = true;
        }
        break;
        }

    } while (repeat);

    /* reading server response [ "access granted"/ "access not permitted" / "enter 2fa code manually" ] */
    char authaccess[MAX_BUFFER_LEN];
    if(read(sd, &authaccess, MAX_BUFFER_LEN) <= 0)
    {
        perror("[add_client:] Error reading auth response from server.\n");
        return errno;
    }
    authaccess[strlen(authaccess)] = '\0';
    printf("%s\n", authaccess);

    /* if manual auth response : */
    if(strncmp(authaccess, "Enter 2FA code", 14) == 0)
    {
        char _2fa_try[_2FA_CODE_LEN];
        fflush(stdin); fflush(stdout);
        scanf("%s", _2fa_try); //read the 2FA code input

        /* writing 2FA input back to server */
        if(write(sd, &_2fa_try, _2FA_CODE_LEN) <= 0)
        {
            perror("[add_client:] Error writing 2FA code.\n");
            return errno;
        }
        
        /* reading if the code was valid or not */
        char manualauthaccess[MAX_BUFFER_LEN];
        if(read(sd, &manualauthaccess, MAX_BUFFER_LEN) <= 0)
        {
            perror("[add_client:] Error writing 2FA code.\n");
            return errno;
        }
        manualauthaccess[strlen(manualauthaccess)] = '\0';
        printf("%s", manualauthaccess);

        /* if code invalid : */
        if(strncmp(manualauthaccess, "Invalid code!", 13) == 0)
        {
            /* read 2nd try of 2FA code */
            manualauthaccess[0] = '\0';
            fflush(stdin); fflush(stdout);
            scanf("%s", _2fa_try);

            /* writing it to server */
            if(write(sd, &_2fa_try, _2FA_CODE_LEN) <= 0)
            {
                perror("[add_client:] Error writing 2FA code.\n");
                return errno;
            }
        
            /* reading again the response */
            if(read(sd, &manualauthaccess, MAX_BUFFER_LEN) <= 0)
            {
                perror("[add_client:] Error writing 2FA code.\n");
                return errno;
            }
            manualauthaccess[29] = '\0';
            printf("%s", manualauthaccess);
        } // *if code is invalid for the 2nd time, client is not accepting a 3rd try
    }
    
}