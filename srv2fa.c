#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

#define MAX_APP_LEN 50
#define MAX_BUFFER_LEN 200
#define _2FA_PORT 2050 // Port for running 2FA server
#define MAX_2FA_BUFFER 10
#define NO_THREADS 2 // 2 threads (2 clients = cli2fa & addsrv)
#define _2FA_CODE_LEN 6 // Length of the 2FA code
#define TIME_FOR_2FA_GEN 15 // Time between 2FA generations
#define NO_DIGITS 10
#define NO_LETTERS 26

extern int errno; // Error code for specific function calls

static void *treat(void *); // Function executed by every thread for handling clients

typedef struct
{
    pthread_t idThread;
    int thCount;
} Thread;

Thread *threadsPool; // threads array

int _2fa_sd;
int nthreads; // no of threads to create
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER; // mutex variable shared by threads

void respond(int cl, int idThread);

int firstclient;
char firstclientmsg[MAX_BUFFER_LEN] = "";

int ok = 0;
int mok = 0;

char* random_2fa_pass() // Function that generates a random 2FA pass
{
    char numbers[] = "0123456789";
    char letter[] = "qwertyuiopasdfghjklzxcvbnm";
    char LETTER[] = "QWERTYUIOPASDFGHJKLZXCVBNM";

    srand((unsigned int)(time(NULL))); // seed rand() by time
    
    char *pass = malloc(_2FA_CODE_LEN);

    int randomizer = rand() % 3; // [0/1/2] for number, letter or LETTER

    for (int i=0; i < _2FA_CODE_LEN; i++)
    {
        switch(randomizer)
        {
            case 0:
            {
                pass[i] = numbers[rand() % NO_DIGITS];
                randomizer = rand() % 3;
            }break;

            case 1:
            {
                pass[i] = letter[rand() % NO_LETTERS];
                randomizer = rand() % 3;
            }break;

            case 2:
            {
                pass[i] = LETTER[rand() % NO_LETTERS];
                randomizer = rand() % 3;
            }break;

            default:
            {
                strcpy(pass,"+ERROR"); // error written instead of pass if rand() fails
            }break;
        }
    }
    return pass;
}

void ret_2fa_pass() // Function that writes different 2FA_codes to each 2FA file
{
    /* Writing code for Tinder app */
    char *_2fa_code_1 = random_2fa_pass();
    FILE *fptr1;
    fptr1 = fopen("_2fa_last_codes_Tinder.tfa", "a");
    if(fptr1 == NULL)
    {
        perror("[2fa_server:] Error opening 2FA storage file\n");
        return errno;
    }
    fprintf(fptr1, "%s", _2fa_code_1);
    fprintf(fptr1,"\n");
    fclose(fptr1);
    free(_2fa_code_1);

    sleep(1); //sleep 1 sec to get another seed for rand()
    
    /* Writing code for Facebook app */
    char *_2fa_code_2 = random_2fa_pass();
    FILE *fptr2;
    fptr2 = fopen("_2fa_last_codes_Facebook.tfa", "a");
    if(fptr2 == NULL)
    {
        perror("[2fa_server:] Error opening 2FA storage file\n");
        return errno;
    }
    fprintf(fptr2, "%s", _2fa_code_2);
    fprintf(fptr2,"\n");
    fclose(fptr2);
    free(_2fa_code_2);

    sleep(1); //sleep 1 sec to get another seed for rand()
    
    /* Writing code for Steam app */
    char *_2fa_code_3 = random_2fa_pass();
    FILE *fptr3;
    fptr3 = fopen("_2fa_last_codes_Steam.tfa", "a");
    if(fptr3 == NULL)
    {
        perror("[2fa_server:] Error opening 2FA storage file\n");
        return errno;
    }
    fprintf(fptr3, "%s", _2fa_code_3);
    fprintf(fptr3,"\n");
    fclose(fptr3);
    free(_2fa_code_3);

    sleep(1); //sleep 1 sec to get another seed for rand()
    
    /* Writing code for Whatsapp app */
    char *_2fa_code_4 = random_2fa_pass();
    FILE *fptr4;
    fptr4 = fopen("_2fa_last_codes_Whatsapp.tfa", "a");
    if(fptr4 == NULL)
    {
        perror("[2fa_server:] Error opening 2FA storage file\n");
        return errno;
    }
    fprintf(fptr4, "%s", _2fa_code_4);
    fprintf(fptr4,"\n");
    fclose(fptr4);
    free(_2fa_code_4);
}

void* gen_2fa_onthread(void *arg)
{
    while(1)
    {
        ret_2fa_pass(); // Write a code to each file
        sleep(TIME_FOR_2FA_GEN); // Wait for some seconds before generating another set of 2FA codes
    }
    return 0;
}


int main()
{
    printf("====================================\n2FA Server\n====================================\n");
    
    /* Creating a separate thread for continously generating 2FA codes for apps */
    pthread_t tid;
    pthread_create(&tid, NULL, &gen_2fa_onthread, NULL); 

    struct sockaddr_in server; // server structure

    void threadCreate(int);
    nthreads = NO_THREADS;
    threadsPool = calloc(sizeof(Thread), nthreads);

    /* creating socket */
    if ((_2fa_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[2fa_server:] Error creating socket.\n");
        return errno;
    }


    /* setting SO_REUSEADDR -> allows server to bind an address having TIME_WAIT state;
       and TCP_NODELAY -> lower latency on packets */
    int on = 1;
    setsockopt(_2fa_sd, SOL_SOCKET, SO_REUSEADDR|TCP_NODELAY, &on, sizeof(on));

    /* preparing the sockaddr struct */
    bzero(&server, sizeof(server));

    /* filling the sockaddr struct for connection */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(_2FA_PORT);

    /* binding socket */
    if (bind(_2fa_sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[2fa_server:] Error occured while binding to socket descriptor.\n");
        return errno;
    }

    /* listening for connections */
    if (listen(_2fa_sd, 2) == -1)
    {
        perror("[2fa_server:] Error occured at listen() call.\n");
        return errno;
    }

    printf("Creating %d threads...\n", nthreads);
    fflush(stdout);

    /* create the threads */
    int i;
    for (i = 0; i < nthreads; i++)
    {
        threadCreate(i);
    }

    /* concurrently serving the clients */
    for (;;)
    {
        printf("[2fa_server:] Awaiting at port %d...\n", _2FA_PORT);
        pause();
    }

    if (write(_2fa_sd, &firstclientmsg, strlen(firstclientmsg)) <= 0)
    {
        perror("Error writing message to 2FA client\n");
        return errno;
    }
};

void threadCreate(int i)
{
    void *treat(void *);

    pthread_create(&threadsPool[i].idThread, NULL, &treat, (void *)i);
    return; // returning from principal thread
}

void *treat(void *arg)
{
    int client;

    struct sockaddr_in from;
    bzero(&from, sizeof(from));
    //printf("[thread]- %d - started...\n", (int)arg);
    fflush(stdout);

    for (;;)
    {
        int length = sizeof(from);
        pthread_mutex_lock(&mlock);

        /* accepting client */
        if ((client = accept(_2fa_sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[2fa_thread:]Error accepting.\n");
        }
        else
        {
            if(ok==0)
            {
                firstclient = client; // store the cli2fa descriptor
                ok = 1;
            }
        }
        pthread_mutex_unlock(&mlock);
        threadsPool[(int)arg].thCount++;

        respond(client, (int)arg); // process the request

    }
}

char msg[MAX_BUFFER_LEN];
char _2fa_opt_1[MAX_BUFFER_LEN] = "Are you trying to log in into ";
char _2fa_opt_2[MAX_BUFFER_LEN] = "Your 2FA code is : ";

void respond(int cl, int idThread)
{
    /* read message (auth method) from additional server */
    if (read(cl, &msg, sizeof(msg)) <= 0)
    {
        perror("[2fa_thread:] Error reading from a client\n");
    }
    printf("[2fa_server:] Message received : %s\n", msg);
    fflush(stdout);

    if((strncmp(msg, "1|", 2) == 0) || (strncmp(msg, "2|", 2) == 0))
    {
        char appname[MAX_APP_LEN];
        strcpy(appname, msg+2); // extract appname from request

        if (msg[0] == '1') // confirmation auth method :
        {
            printf("Authenticating through confirmation...\n");
            strcat(_2fa_opt_1, appname);
            strcat(_2fa_opt_1, " app ? [Y/N]\n");
            if(write(firstclient, &_2fa_opt_1, strlen(_2fa_opt_1)) < 0)
            {
                perror("[2fa_server:] Error writing [Y/N] message to 2FA client\n");
                return errno;
            }

            /* reading [yes/no] message from client */
            char yn[sizeof(char)];
            if (read(firstclient, yn, sizeof(char)) < 0)
            {
                perror("[2fa_server:] Error reading 2FA option from client.\n");
                return errno;
            }
            yn[sizeof(char)] = '\0';
            printf("%s option sending to additional server...\n", yn);

            /* writing [yes/no] message to additional server */
            if (write(cl,&yn,sizeof(char)) < 0)
            {
                perror("[2fa_server:] Error writing 2FA option to additional server.\n");
                return errno;
            }

        }
        if (msg[0] == '2') // manual auth method :
        {
            printf("Authenticating through 2FA code manually...\n");

            /* Preparing 2FA_last_codes filename */
            char _2fa_file[MAX_BUFFER_LEN] = "_2fa_last_codes_";
            strcat(_2fa_file, appname);
            strcat(_2fa_file,".tfa");
            char fileSpec[strlen(_2fa_file)+1];
            snprintf(fileSpec, sizeof(fileSpec), "%s", _2fa_file);
            fflush(stdout);

            /* Reading last code written (This is our 2FA code) */
            FILE *fptr;
            fptr = fopen(fileSpec, "r");
            fseek(fptr, 0, SEEK_END);
            int seeklen = ftell(fptr);
            fseek(fptr, seeklen - _2FA_CODE_LEN - 1, SEEK_SET);
            int ch_2fa = 1;
            char ch_2fa_str[_2FA_CODE_LEN];
            int i = 0;
            do
            {
                ch_2fa = fgetc(fptr);
                ch_2fa_str[i++] = ch_2fa;
            } while(i!=_2FA_CODE_LEN);
            
            /* Writing the 2FA code to 2FA client */
            strncat(_2fa_opt_2, ch_2fa_str, _2FA_CODE_LEN);
            if (write(firstclient, &_2fa_opt_2, strlen(_2fa_opt_2)) <= 0)
            {
                perror("[2fa_server:] Error writing the 2FA option to client.\n");
                return errno;
            }
            
            /* Write C option in additional client -> additional client will know to handle manual auth method */
            if (write(cl,"C",sizeof(char)) < 0)
            {
                perror("[2fa_server:] Error writing 2FA option to additional server.\n");
                return errno;
            }

        }

    }

}
