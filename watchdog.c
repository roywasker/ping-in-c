#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <signal.h>

#define SERVER_PORT 3000
char ip[17]={'\0'};  // 3dig.3did.3dig.3did + \n
char buffer[1024]={0};
int ClientSocket;

void listener(int ClientSocket);
void signal_handler(int signum);

int main()
{
    int listenSocket = -1; // create listening socket
	if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Could not create listening socket : %d", errno);
		exit(1);
	}
	printf("Sokcet created\n");

	int yes = 1; // check if the ip in not in use
	if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
	{
		perror("setsockopt");
		exit(1);
	}

	//"sockaddr_in" used for IPv4 communication
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(SERVER_PORT);

	// Bind the socket to the port with any IP at this port
	if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
	{
		printf("Bind failed with error code : %d", errno);
		close(listenSocket);
		exit(1);
	}

	printf("Bind success\n");

	// Make the socket listening; actually mother of all client sockets.
	if (listen(listenSocket, 1) == -1) // 1 is a Maximum size of queue connection requests
	{
		printf("listen failed with error code : %d", errno);
		close(listenSocket);
		exit(1);
	}

	printf("Waiting for incoming TCP-connections\n");

	struct sockaddr_in ClientAddress;
	socklen_t clientAddressLen = sizeof(ClientAddress);
	memset(&ClientAddress, 0, sizeof(ClientAddress)); // file struct with 0

	ClientSocket = accept(listenSocket, (struct sockaddr *)&ClientAddress, &clientAddressLen); // create socket to sender
	if (ClientSocket == -1)
	{
		printf("listen failed with error code : %d", errno);
		return -1;
	}

	printf("A new client connection accepted\n\n");
	signal(SIGALRM, signal_handler);
	listener(ClientSocket);
    return 0;
}
void listener(int ClientSocket){
	while (1)
    {
        int BytesLeft = 5; // intialize how much byte left to received
		memset(&buffer, 0, sizeof(buffer));
		alarm(10);
		int BytesReceived = 0;// countig how much byte received from sender
		while (BytesReceived < 5)
		{
			int MessRecv = recv(ClientSocket, buffer, sizeof(buffer), 0); // receive the message 
			BytesReceived += MessRecv; // add the number of byte that arrive from sender
			BytesLeft -= MessRecv; // subtraction the number of byte that left to receive
		}
		printf("from watchdog %s\n",buffer);
		strncpy(ip,buffer+13,strlen(buffer)-12-13);
    }
}
void signal_handler(int signum)
{
    char message[] = "Time out";
	int messageLen = strlen(message) + 1;
	int bytesSent = send(ClientSocket, message, messageLen, 0);
    printf("\nserver <%s> cannot be reached.\n", ip);
	printf("close soket\n");
	close(ClientSocket);
    exit(0);
}