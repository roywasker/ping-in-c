#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define PORT 9999
#define SERVER_IP_ADDRESS "0.0.0.0"


int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0); // create socket
    if (sock == -1)
    {
        printf("Unable to create a socket : %d", errno);
        exit(1);
    }

    //"sockaddr_in" used for IPv4 communication
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress)); // file struct with 0
    serverAddress.sin_family = AF_INET; // work with ipv4
    serverAddress.sin_port = htons(PORT); // insert to struct the port 
    int rval = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &serverAddress.sin_addr);//convert the address to binary
    if (rval <= 0)
    {
        printf("inet_pton failed");
        exit(1);
    }

    if (connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) // Make a connection to the receiver with socket
    {
        printf("connect failed with error code : %d", errno);
    }

    printf("connected to server\n\n");



    char *args[2];
    // compiled watchdog.c by makefile
    args[0] = "./watchdog";
    args[1] = NULL;
    int status;
    int pid = fork();
    if (pid == 0)
    {
        printf("in child \n");
        execvp(args[0], args);
    }
    wait(&status); // waiting for child to finish before exiting
    printf("child exit status is: %d", status);




    
    return 0;
}
