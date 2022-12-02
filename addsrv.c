#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>

#define _2FA_CODE_LEN 6 // Length of the 2FA code
#define MAX_BUFFER_LEN 200
#define MAX_APP_LEN 50
#define PORT 3103 // Used port for additional server
#define _2FA_PORT 2050 // Used port for 2fa server
#define PREDEFAPP "Steam" // Simulated app in the additional server-client pair [Available : "Steam", "Facebook", "Whatsapp", "Tinder"]
#define PREDEF_2FA_FILENAME "_2fa_last_codes_Steam.tfa"

/* No of bytes to go backwards for checking 2FA codes
(We check the last 5 codes because the user 2FA input could be delayed)*/
#define _2FA_CODE_CHECK 34

extern int errno;

int main()
{
    printf("====================================\nAdditional Server\n====================================\n");

    /* Setting the app name from macro (Could also be read by user input) */
    char appname[MAX_APP_LEN] = " ";
    strcpy(appname, PREDEFAPP);

    char _2fa_opt[MAX_BUFFER_LEN] = " ";

    struct sockaddr_in server;
    struct sockaddr_in from;

    /* Creating the connection socket */
    int sd;
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[add_server:] Error creating socket.\n");
        return errno;
    }

    /* preparing the structs */
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    /* filling the sockaddr struct for connection */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    /* attaching the socket */
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[add_server:] Error occured while binding to socket descriptor.\n");
        return errno;
    }

    /* listen for incoming connections */
    if (listen(sd, 2) == -1)
    {
        perror("[add_server:] Error occured at listen() call.\n");
        return errno;
    }

    /* creating the socket for communicating with the 2FA server */
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

    /* serving the clients (addcli and srv2fa are the clients) */
    while (1)
    {
        int client;
        int length = sizeof(from);
        printf("[add_server:] Awaiting at %d port...\n", PORT);
        fflush(stdout);

        /* accept the client */
        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[add_server:] Error accepting client connection.");
            return errno;
        }
        bzero(_2fa_opt, MAX_BUFFER_LEN);
        strcpy(_2fa_opt, "Auth into ");
        strcat(_2fa_opt, appname);
        strcat(_2fa_opt, " app.\n1. Send notification in 2FA app.\n2. Enter 2FA code manually.\n[1/2]\n");

        /* writing the "select 2FA auth method" message to additional client */
        if (write(client, _2fa_opt, MAX_BUFFER_LEN) <= 0)
        {
            perror("[add_server:] Error writing to client.\n");
            continue;
        }

        /* reading 2FA auth method response */
        char _2fa_response[MAX_BUFFER_LEN] = "";
        if (read(client, _2fa_response, 1) <= 0)
        {
            perror("[add_server:] Error reading 2FA response from client.\n");
            close(client);
            continue;
        }
        printf("Option %s chosen for 2FA auth\n", _2fa_response);

        printf("Sending option to 2FA server ... \n");
        
        /* connect to the 2FA server (srv2fa) */
        if (connect(_2fa_sd, (struct sockaddr *)&_2fa_server, sizeof(struct sockaddr)) == -1)
        {
            perror("[add_server:] Error connecting to 2FA server");
            return errno;
        }

        strcat(_2fa_response, "|");
        strcat(_2fa_response, appname);

        /* writing the response from additional client to the 2FA server ([<1/2>|<appname>] format) */
        if(write(_2fa_sd, _2fa_response, sizeof(_2fa_response)) <= 0)
        {
            perror("[add_server:] Error writing to _2fa_sd.\n");
            return errno;
        }

        /* read state of authentication [ "yes" / "no" / "manual auth required" ] */
        char yn[sizeof(char)];
        if (read(_2fa_sd, yn, sizeof(char)) < 0)
        {
            perror("[add_server:] Error reading 2FA option from 2FA server.\n");
            return errno;
        }
        yn[1] = '\0';
        printf("%s option chosen\n", yn);

        switch(yn[0]) // handling the auth cases
        {
            case 78: // no
            {
                if(write(client, "Authentication not permitted !", 30) < 0)
                {
                    perror("[add_server:] Error writing auth response to client.\n");
                    return errno;
                }
            }break;

            case 89: // yes
            {
                if(write(client, "Granted access !\n", 17) < 0)
                {
                    perror("[add_server:] Error writing auth response to client.\n");
                    return errno;
                }
            }break;

            case 67: // manual auth required
            {
                /* requesting the client for 2FA code */
                if(write(client, "Enter 2FA code : \n", 18) < 0)
                {
                    perror("[add_server:] Error writing auth response to client.\n");
                    return errno;
                }

                /* reading the inserted 2FA code */
                char _2fa_try[_2FA_CODE_LEN];
                if(read(client, &_2fa_try, sizeof(_2fa_try)) < 1)
                {
                    perror("[add_server:] Error reading 2FA code.\n");
                    return errno;
                }
                _2fa_try[_2FA_CODE_LEN] = '\0';
                printf("2FA code inserted : %s\n", _2fa_try);


                printf("Checking if code is valid...\n");
                
                /* check if 2FA server wrote the inserted code */
                FILE *fp;
                fp = fopen(PREDEF_2FA_FILENAME, "rb"); // open 2fa_codes_ file in binary mode
                fseek(fp, 0, SEEK_END); // seeking to end of file
                int seeklen = ftell(fp); // getting the pos of offset
                fseek(fp, seeklen - _2FA_CODE_CHECK - 1, SEEK_SET); // go back (to check last 5 FA codes)
                int ch_2fa = 1;
                char ch_2fa_str[_2FA_CODE_LEN];
                int i = 0;
                int j = 0;
                bool validcode = false; // flag to set if code is valid
                do
                {
                    i++;
                    ch_2fa = fgetc(fp); // reading char by char
                    ch_2fa_str[j++] = ch_2fa; // extracting an entire 2FA code from file
                    if(j == _2FA_CODE_LEN)
                    {
                        ch_2fa_str[_2FA_CODE_LEN] = '\0';
                        //printf("%d : %s\n", i, ch_2fa_str);
                        if(strncmp(ch_2fa_str, _2fa_try, _2FA_CODE_LEN) == 0)
                        {
                            validcode = true; // if the inserted code matches the code from file we set the flag to 'true'
                        }
                        j = 0;
                        fgetc(fp); // reading the EOL
                    }
                } while (i!=_2FA_CODE_CHECK); // when i = _2FA_CODE_CHECK, the offset is at EOF

                fclose(fp);

                /*if(validcode == true) printf("VALID 2FA CODE\n");
                else printf("INVALID CODE\n");*/

                if(validcode == true)
                {
                    /* writing to client that the code is valid */
                    if(write(client, "Valid code! Granted access !\n", 29) < 0)
                    {
                        perror("[add_server:] Error writing auth response to client.\n");
                        return errno;
                    }
                }
                else
                {
                    /* writing to client that the code is invalid */
                    if(write(client, "Invalid code! Not permitted. Try again !\n", 41) < 0)
                    {
                        perror("[add_server:] Error writing auth response to client.\n");
                        return errno;
                    }

                    /* let the client try for a 2nd time if the 1st code is invalid */
                    if(read(client, &_2fa_try, sizeof(_2fa_try)) < 1)
                    {
                        perror("[add_server:] Error reading 2FA code.\n");
                        return errno;
                    }
                    _2fa_try[_2FA_CODE_LEN] = '\0';
                    printf("2FA code inserted : %s\n", _2fa_try);


                    printf("Checking if code is valid...\n");
                
                    /* check again the validity */ {
                    FILE *fp;
                    fp = fopen(PREDEF_2FA_FILENAME, "rb");
                    fseek(fp, 0, SEEK_END);
                    int seeklen = ftell(fp);
                    fseek(fp, seeklen - _2FA_CODE_CHECK - 1, SEEK_SET);
                    int ch_2fa = 1;
                    char ch_2fa_str[_2FA_CODE_LEN];
                    int i = 0;
                    int j = 0;
                    bool validcode = false;
                    do
                    {
                        i++;
                        ch_2fa = fgetc(fp);
                        ch_2fa_str[j++] = ch_2fa;
                        if(j == _2FA_CODE_LEN)
                        {
                            ch_2fa_str[_2FA_CODE_LEN] = '\0';
                            //printf("%d : %s\n", i, ch_2fa_str);
                            if(strncmp(ch_2fa_str, _2fa_try, _2FA_CODE_LEN) == 0)
                            {
                                validcode = true;
                            }
                            j = 0;
                            fgetc(fp);
                        }
                    } while (i!=_2FA_CODE_CHECK);
                    fclose(fp);
                    if(validcode == true)
                    {
                        if(write(client, "Valid code! Granted access !\n", 29) < 0)
                        {
                            perror("[add_server:] Error writing auth response to client.\n");
                            return errno;
                        }
                    }
                    else
                    {
                        if(write(client, "Invalid 2FA code!\nExiting...\n", 29) < 0)
                        {
                            perror("[add_server:] Error writing auth response to client.\n");
                            return errno;
                        }
                    }

                    }
                }  

            }break; // If code is invalid for the 2nd time, connection is closing
        }

    }
}