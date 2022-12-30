#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>

#define PORT 3000
#define SERVER_IP_ADDRESS "0.0.0.0"
#define PACKETSIZE	64 
struct packet   //create new struct named packet
{
	struct icmphdr hdr;   //struct have field of icmphdr struct
	char msg[PACKETSIZE-sizeof(struct icmphdr)]; //create string to the message
};

int pid=-1; // process id
struct protoent *proto=NULL; // pointer to protoent struct
double timer=0; 
struct timeval start;
int firstmessping=0; //get value 1 when first message of ping arrive
char message[45]={0};
int sock;

unsigned short checksum(void *b, int len);
void display(void *buf, int bytes);
void listener(int sock);
void ping(struct sockaddr_in *addr);
int checktimeout();
void signal_handler(int signum);

int main(int count, char *argv[])
{
    struct hostent *hname;
	struct sockaddr_in addr;
	char sourceIP[1] = { '\0' }; 
	char *args[2];
	args[0] = "./watchdog";
    args[1] = NULL;
    if ( count != 2 ) // check if we receive to where check connection
    {
        printf("usage: %s <addr> \n", argv[0]);
        exit(0);
    }
	if(inet_pton(AF_INET, argv[1], sourceIP)<=0){ // check if the ip is correct
			printf("%s is wrong ip\n",argv[1]);
			exit(1);
	}
	pid = fork();// get process id
	if (pid==0)
	{
		execvp(args[0], args);
	}
	sleep(1);
    sock = socket(AF_INET, SOCK_STREAM, 0); // create socket
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
        printf("connect failed with error code : %d\n", errno);
    }

    printf("connected to server\n\n");

	signal(SIGALRM, signal_handler);
	if ( count == 2 )
	{
		proto = getprotobyname("ICMP");
		hname = gethostbyname(argv[1]); // insert the host ip to check connection
		bzero(&addr, sizeof(addr));
		addr.sin_family = hname->h_addrtype;
		addr.sin_port = 0;
		addr.sin_addr.s_addr = *(long*)hname->h_addr;
		ping(&addr); //send ping
		wait(0);
	}
	else
		printf("usage: ping <hostname>\n");
		
    return 0;
}
unsigned short checksum(void *b, int len) //checksum to detection error
{	unsigned short *buf = b;
	unsigned int sum=0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

void display(void *buf, int bytes)
{
	struct iphdr *ip = buf; 
	struct icmphdr *icmp = buf+ip->ihl*4;  //pointer to iphdr struct that point to first bit after ip hader
    int iphader=20 ,icmphader=8;
	char sourceIPAddrReadable[32] = { '\0' }; 
	inet_ntop(AF_INET, &ip->saddr, sourceIPAddrReadable, sizeof(sourceIPAddrReadable)); // convert the ip form bit to string
    if (firstmessping==0) // check if it first ping message enter if yes
    {
        printf("PING %s(%s) %d bytes of data\n",sourceIPAddrReadable,sourceIPAddrReadable,bytes-iphader-icmphader); //print first message of ping 
        firstmessping++;
    }
    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.03f ms\n",bytes-iphader,sourceIPAddrReadable,icmp->un.echo.sequence,ip->ttl,timer); //print ping massage
	memset(&message, 0, sizeof(message));
	strcat(message,"connected to "); 
    strcat(message, sourceIPAddrReadable); // add the ip to message
    strcat(message," succesfully");
}

void listener(int sock)
{
	struct sockaddr_in addr;
	unsigned char buf[1024]; // buffer of 1024 char to read bytes

	int sd = socket(AF_INET, SOCK_RAW, proto->p_proto); // create a socket
	if (sd < 0)											// check if socket create successfully
	{
		perror("socket");
		exit(0);
	}
	int len = sizeof(addr);
	bzero(buf, sizeof(buf)); // reset the buffer
	int bytes = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &len); // receive bytes from socket
	alarm(9);
	struct timeval end;
	gettimeofday(&end, 0); // start measure time
	timer=(end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0;
	if (bytes > 0)
	{ // if we get 1 or more bytes send to  display that print it
		display(buf, bytes);
		int messageLen = strlen(message) + 1;
		int bytesSent = send(sock, message, messageLen, 0);
	}
	else
		perror("recvfrom");
}

void ping(struct sockaddr_in *addr)
{	const int val=255;
	int i,cnt=1; // cnt count seq number
	struct packet pckt;
	struct sockaddr_in r_addr;

	int sd = socket(AF_INET, SOCK_RAW, proto->p_proto); //create a socket 
	if ( sd < 0 ) // check if it first ping message enter if yes
	{
		perror("socket");
		return;
	}
	if (setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0) // cheack if the socket is free
		perror("Set TTL option");
	if (fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
		perror("Request nonblocking I/O");
	while (1) // send pings infinity
	{	
		int len=sizeof(r_addr);
		if (recvfrom(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &len) < 0 ){}// receive bytes from socket 
		bzero(&pckt, sizeof(pckt)); // reset the buffer
		pckt.hdr.type = ICMP_ECHO; // build the hader in 6 next line
		pckt.hdr.un.echo.id = pid;
		for (i = 0; i < sizeof(pckt.msg)-1; i++ )
			pckt.msg[i] = i+'0';
		pckt.msg[i] = 0;
		pckt.hdr.un.echo.sequence = cnt++;
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
		gettimeofday(&start,0); // start measure time
		if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0 ){ // send the packet 
			perror("sendto");
        }
		listener(sock);
		sleep(1); // wait 1 second to send next ping
	}
}
int checktimeout(){
	char buffer[1024];
	int MessRecv = recv(sock, buffer, sizeof(buffer), 0); // receive the message
	if (strcmp(buffer, "Time out") == 0)
	{
		printf("\nTime out\nclose socket\n");
		close(sock);
		return 0;
	}
	return 1;
}
void signal_handler(int signum)
{
    int result = checktimeout();
	if (result!=0)
	{
    	return;
	}
	exit(0);
}
