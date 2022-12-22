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

#define PACKETSIZE	64
struct packet
{
	struct icmphdr hdr;
	char msg[PACKETSIZE-sizeof(struct icmphdr)];
};

int pid=-1; // procces id
struct protoent *proto=NULL; // pointer to protoent struct

void ping(struct sockaddr_in *addr);
void listener(void);
void display(void *buf, int bytes);
unsigned short checksum(void *b, int len);


int main(int count, char *argv[])   
{	struct hostent *hname;
	struct sockaddr_in addr;
    if ( count != 2 )
    {
        printf("usage: %s <addr> \n", argv[0]);
        exit(0);
    }
	if ( count > 1 )
	{
		pid = getpid();
		//printf("id main process = %d\n", pid);
		proto = getprotobyname("ICMP");
		hname = gethostbyname(argv[1]);
		bzero(&addr, sizeof(addr));
		addr.sin_family = hname->h_addrtype;
		addr.sin_port = 0;
		addr.sin_addr.s_addr = *(long*)hname->h_addr;
		if ( fork() == 0 )   /* child process */
		{
			//printf("child process with pid  = %d\n", (int) getpid());
			listener();
		}
		else   /* parent process */
		{
			//printf("parent process with pid  = %d\n", (int) getpid());
			ping(&addr);
		}
		wait(0);
	}
	else
		printf("usage: myping <hostname>\n");
	return 0;
}

unsigned short checksum(void *b, int len)
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
{	int i;
	struct iphdr *ip = buf;
	struct icmphdr *icmp = buf+ip->ihl*4;
    
	printf("\n");
	char sourceIPAddrReadable[32] = { '\0' };
	inet_ntop(AF_INET, &ip->saddr, sourceIPAddrReadable, sizeof(sourceIPAddrReadable));
	char destinationIPAddrReadable[32] = { '\0' }; /// mybey not nesseray
	inet_ntop(AF_INET, &ip->daddr, destinationIPAddrReadable, sizeof(destinationIPAddrReadable)); /// mybey not nesseray
    //inet_ntoa(ip->saddr)
    //inet_ntoa(ip->daddr)
    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=",ip->ihl*4,sourceIPAddrReadable,icmp->un.echo.sequence,ip->ttl);
}

void listener(void)
{	int sd;
	struct sockaddr_in addr;
	unsigned char buf[1024];

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
	if ( sd < 0 )
	{
		perror("socket");
		exit(0);
	}
	for (;;)
	{	
		int bytes, len=sizeof(addr);

		bzero(buf, sizeof(buf));
		bytes = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &len);
		if ( bytes > 0 )
			display(buf, bytes);
		else
			perror("recvfrom");
	}
	exit(0);
}

void ping(struct sockaddr_in *addr)
{	const int val=255;
	int i, j, sd, cnt=1; // cnt count seq number
	struct packet pckt;
	struct sockaddr_in r_addr;

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
	if ( sd < 0 )
	{
		perror("socket");
		return;
	}
	if ( setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
		perror("Set TTL option");
	if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
		perror("Request nonblocking I/O");
	while (1) // send pings 1 per second
	{	int len=sizeof(r_addr);
		
		if ( recvfrom(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &len) > 0 ){
            
        }
		bzero(&pckt, sizeof(pckt));
		pckt.hdr.type = ICMP_ECHO;
		pckt.hdr.un.echo.id = pid;
		for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
			pckt.msg[i] = i+'0';
		pckt.msg[i] = 0;
		pckt.hdr.un.echo.sequence = cnt++;
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
		if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0 )
			perror("sendto");
		sleep(1);
	}
}