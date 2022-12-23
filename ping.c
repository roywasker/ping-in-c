#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>

#define PACKETSIZE	64 
struct packet   //create new struct named packet
{
	struct icmphdr hdr;   //struct have field of icmphdr struct
	char msg[PACKETSIZE-sizeof(struct icmphdr)]; //create string to the message
};

int pid=-1; // process id
struct protoent *proto=NULL; // pointer to protoent struct
struct timeval start , end; 
double timer=0; 
int firstmessping=0; //

unsigned short checksum(void *b, int len);
void display(void *buf, int bytes);
void listener(void);
void ping(struct sockaddr_in *addr);

int main(int count, char *argv[])   
{	struct hostent *hname;
	struct sockaddr_in addr;
    if ( count != 2 ) // check if we receive to where check connection
    {
        printf("usage: %s <addr> \n", argv[0]);
        exit(0);
    }
	if ( count > 1 )
	{
		pid = getpid(); // get process id
		proto = getprotobyname("ICMP");
		hname = gethostbyname(argv[1]); // insert the host ip to check connection
		bzero(&addr, sizeof(addr));
		addr.sin_family = hname->h_addrtype;
		addr.sin_port = 0;
		addr.sin_addr.s_addr = *(long*)hname->h_addr;
		if ( fork() == 0 )  
		{
			listener(); //listen to socket
		}
		else   
		{
			ping(&addr); //send ping
		}
		wait(0);
	}
	else
		printf("usage: myping <hostname>\n");
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
    
	char sourceIPAddrReadable[32] = { '\0' }; 
	inet_ntop(AF_INET, &ip->saddr, sourceIPAddrReadable, sizeof(sourceIPAddrReadable)); // convert the ip form bit to string
    if (firstmessping==0) // check if it first ping message enter if yes
    {
        printf("PING %s(%s) %d bytes of data\n",sourceIPAddrReadable,sourceIPAddrReadable,bytes-28); //print first message of ping 
        firstmessping++;
    }
    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.03f ms\n",bytes-20,sourceIPAddrReadable,icmp->un.echo.sequence,ip->ttl,timer); //print ping massage with seq number, ttl 
}																																	  //and how much time its take to receive

void listener(void)
{	
	struct sockaddr_in addr; 
	unsigned char buf[1024]; // buffer of 1024 char to read bytes

	int sd = socket(AF_INET, SOCK_RAW, proto->p_proto); // create a socket 
	if ( sd < 0 ) // check if socket create successfully
	{
		perror("socket");
		exit(0);
	}
	while (1)
	{
		int len=sizeof(addr); 
		bzero(buf, sizeof(buf)); // reset the buffer
		gettimeofday(&start,0);
		int bytes = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &len); // receive bytes from socket
		gettimeofday(&end, 0);
		long seconds = (end.tv_sec-start.tv_sec);
    	long microseconds = end.tv_usec - start.tv_usec;
		timer=(seconds)*1000+(microseconds)*1e-4;
		if ( bytes > 0 ) // if we get 1 or more bytes send to  display that print it
			display(buf, bytes);
		else
			perror("recvfrom");
	}
	exit(0);
}

void ping(struct sockaddr_in *addr)
{	const int val=255;
	int i,j, cnt=1; // cnt count seq number
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
		if (recvfrom(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &len) < 0 ){ // receive bytes from socket 
            perror("recv from");
        }
		bzero(&pckt, sizeof(pckt)); // reset the buffer
		pckt.hdr.type = ICMP_ECHO; // build the hader in 6 next line
		pckt.hdr.un.echo.id = pid;
		for (i = 0; i < sizeof(pckt.msg)-1; i++ )
			pckt.msg[i] = i+'0';
		pckt.msg[i] = 0;
		pckt.hdr.un.echo.sequence = cnt++;
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
		if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0 ){ // send the packet 
			perror("sendto");
        }
		sleep(1); // wait 1 second to send next ping
	}
}