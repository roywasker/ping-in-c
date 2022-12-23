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
#include <sys/time.h>

#define SERVER_PORT 3000
struct timeval start, end;
char ip[17]={'\0'};  // 3dig.3did.3dig.3did + \n

void checktimer(int socket);
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

	int ClientSocket = accept(listenSocket, (struct sockaddr *)&ClientAddress, &clientAddressLen); // create socket to sender
	if (ClientSocket == -1)
	{
		printf("listen failed with error code : %d", errno);
		return -1;
	}

	printf("A new client connection accepted\n\n");
	checktimer(ClientSocket);
    while (1)
    {
        int BytesLeft = 5; // intialize how much byte left to received
		char buffer[1024];
		int BytesReceived = 0;// countig how much byte received from sender
		while (BytesReceived < 5)
		{
			int MessRecv = recv(ClientSocket, buffer, BytesLeft, 0); // receive the message 
			BytesReceived += MessRecv; // add the number of byte that arrive from sender
			BytesLeft -= MessRecv; // subtraction the number of byte that left to receive
		}
		gettimeofday(&start,0); // stop measure time
		if ((strlen)==0)
		{
			int i; //"connected to 8.8.8.8 succesfully"
			for ( i = 0; i < 16; i++)
			{
				if (buffer[12+i]==" ")
				{
					break;
				}
				ip[i]=buffer[12+i];
			}
			ip[i]='\0';
		}
    }
    return 0;
}
void checktimer(int socket){
	while (1)
	{
		gettimeofday(&end, 0); // stop measure time
		double timer = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
		if (timer > 10)
		{
			char message[] = "Time out\n";
			int messageLen = strlen(message) + 1;
			int bytesSent = send(socket, message, messageLen, 0);
			break;
		}
		int bytesSent = send(socket, "time is good\n", 14, 0);
	}
	printf("server <%s> cannot be reached.\n", ip);
	close(socket);
	exit(0);
}