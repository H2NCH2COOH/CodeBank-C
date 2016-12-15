#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*
 * icmp-tool
 * Send custom icmp packet
 * Usage: icmp-tool -t <type> [-c <code>] [-r <roth>] [-d] [-w <wait>] <src addr> <dst addr>
 *      -t type: type number
 *      -c code: code number
 *      -r roth: 32-bit rest of the header as integer
 *           -d: send with extra data from stdin
 *      -w wait: wait time for response, default to 5 seconds
 *     src addr: source ip address
 *     dst addr: destination ip address
 */

static char buff[1024];

struct ip_header
{
    uint8_t     hl:4,       /* 4 bit header length */
                ver:4;      /* 4 bit version */
    uint8_t     tos;        /* type of service */
    uint16_t    totl;       /* total length of datagram */
    uint16_t    id;         /* identifier */
    uint16_t    notused;    /* this is were flags and fragment offset would go */
    uint8_t     ttl;        /* time to live */
    uint8_t     prot;       /* protocol */
    uint16_t    csum;       /* our checksum */
    uint32_t    saddr;      /* source address */
    uint32_t    daddr;      /* destination address */
};

struct icmp_header
{
    uint8_t     type;
    uint8_t     code;
    uint16_t    csum;
    uint32_t    roth;
};

static void usage()
{
    fprintf(stderr,"Usage: icmp-tool -t <type> [-c <code>] [-r <rfth>] [-d] [-w <wait>] <src addr> <dst addr>\n");
    exit(1);
}

uint16_t calcsum(uint16_t* buffer,int length)
{
    uint32_t sum;   

    // initialize sum to zero and loop until length (in words) is 0 
    for(sum=0;length>1;length-=2) // sizeof() returns number of bytes, we're interested in number of words 
    {
        sum+=*buffer++; // add 1 word of buffer to sum and proceed to the next 
    }
    // we may have an extra byte 
    if(length==1)
    {
        sum+=(char)*buffer;
    }

    sum=(sum>>16)+(sum&0xFFFF);  // add high 16 to low 16 
    sum+=(sum>>16);
    return ~sum;
}

int main(int argc, char* argv[])
{
    int opt;
    int value=1;
    
    int type,type_flag=0,code,code_flag=0,roth_flag=0,data_flag=0,wait=5;
    long roth;
    char* src_addr;
    char* dst_addr;
    
    size_t data_len=0;
    
    struct icmp_header icmp_header;
    struct ip_header* ip_h;
    struct icmp_header* icmp_h=&icmp_header;
    char* packet;
    
    struct sockaddr_in connection;
    int sockfd;
    
    uint32_t time;
    
    struct timeval tv;
    
    if(getuid()!=0)
    {
        fprintf(stderr,"You must be root to run this program\n");
        exit(1);
    }
    
    while((opt=getopt(argc,argv,"dt:c:r:w:"))!=-1)
    {
        switch(opt)
        {
            case 'd':
                data_flag=1;
                break;
            case 't':
                type=atoi(optarg);
                type_flag=1;
                break;
            case 'c':
                code=atoi(optarg);
                code_flag=1;
                break;
            case 'r':
                roth=atol(optarg);
                roth_flag=1;
                break;
            case 'w':
                wait=atoi(optarg);
                break;
            default:
                usage();
        }
    }
    
    if(optind>(argc-2))
    {
        usage();
    }
    
    src_addr=argv[optind];
    dst_addr=argv[optind+1];
    
    if(type_flag==0)
    {
        usage();
    }
    
    if(data_flag)
    {
        data_len=fread(buff,1,sizeof(buff),stdin);
    }
    
    //Build icmp header
    icmp_h->type=type;
    switch(type)
    {
        case 5: //Redirect
            if(!code_flag)
            {
                fprintf(stderr,"Type 5--Redirect requires code\n");
                exit(1);
            }
            icmp_h->code=code;
            if(!roth_flag)
            {
                fprintf(stderr,"Type 5--Redirect requires ip as <roth>\n");
                exit(1);
            }
            icmp_h->roth=roth;
            break;
        case 8: //Echo
            icmp_h->code=0;
            if(roth_flag)
            {
                icmp_h->roth=roth;
            }
            else
            {
                icmp_h->roth=0;
            }
            break;
        case 11: //Time exceeded
            if(!code_flag)
            {
                fprintf(stderr,"Type 11--Time exceeded requires code\n");
                exit(1);
            }
            icmp_h->code=code;
            icmp_h->roth=0;
            break;
        case 13: //Timestamp
            icmp_h->code=0;
            icmp_h->roth=0;
            if(!data_flag)
            {
                //No custom data, generate right data
                data_flag=1;
                data_len=3*4;
                //FIXME: Findout how to generate right time data
                memset(buff,0,data_len);
            }
            break;
        //TODO: Add more types
        default:
            icmp_h->code=(code_flag)? code:0;
            icmp_h->roth=(roth_flag)? roth:0;
    }
    
    packet=malloc(sizeof(struct ip_header)+sizeof(struct icmp_header)+data_len);
    
    ip_h=(struct ip_header*)packet;
    memcpy(packet+sizeof(struct ip_header),icmp_h,sizeof(icmp_header));
    icmp_h=(struct icmp_header*)(packet+sizeof(struct ip_header));
    
    if(data_flag)
    {
        memcpy(packet+sizeof(struct ip_header)+sizeof(struct icmp_header),buff,data_len);
    }
    
    icmp_h->csum=0;
    icmp_h->csum=calcsum((uint16_t*)icmp_h,sizeof(struct icmp_header)+data_len);
    
    //Build ip header
    ip_h->ver       =4;
    ip_h->hl        =5;
    ip_h->tos       =0;
    ip_h->totl      =sizeof(struct ip_header)+sizeof(struct icmp_header)+data_len;
    ip_h->notused   =0;
    ip_h->ttl       =255;
    ip_h->prot      =1;
    ip_h->csum      =0;
    ip_h->saddr     =inet_addr(src_addr);
    ip_h->daddr     =inet_addr(dst_addr);
    //Calc check sum at last
    ip_h->csum      =calcsum((uint16_t*)ip_h,sizeof(struct ip_header));
    
    //Create socket
    sockfd=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
    if(sockfd==-1)
    {
        free(packet);
        fprintf(stderr,"Could not initialize sending socket\n");
        exit(1);
    }
    
    //if IP_HDRINCL isn't set, the kernel will try to put it's own header on our packet
    if(setsockopt(sockfd,IPPROTO_IP,IP_HDRINCL,&value,sizeof(int)))
    {
        printf("Could not use custom header.\n");
    }
    
    //connection is just a variable used to define who we're sending to for sendto()
    connection.sin_family       =AF_INET;
    connection.sin_addr.s_addr  =ip_h->daddr;
    
    //Send
    sendto(sockfd,packet,ip_h->totl,0,(struct sockaddr *)&connection,sizeof(struct sockaddr));
    
    close(sockfd);
    free(packet);
    
    //Receive
    //Create socket
    sockfd=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
    if(sockfd==-1)
    {
        free(packet);
        fprintf(stderr,"Could not initialize receiving socket\n");
        exit(1);
    }
    
    //Set socket timeout
    tv.tv_sec=wait;
    tv.tv_usec=0;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&tv,sizeof(struct timeval));
    
    data_len=recvfrom(sockfd,buff,sizeof(buff),0,NULL,NULL);
    if(data_len==-1)
    {
        fprintf(stderr, "Failed to receive packet\n");
        exit(-1);
    }
    
    printf("===Received socket===\n");
    fwrite(buff,1,data_len,stdout);
    
    return 0;
}