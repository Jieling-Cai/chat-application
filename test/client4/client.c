#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <arpa/inet.h>

#define BUFSIZE 4096
#define h_addr h_addr_list[0] /* for backward compatibility */
#define MAX_DATA_SIZE 400
#define username "c4"

struct __attribute__((__packed__)) Msg{
unsigned short type;
char src[20];
char dest[20];
unsigned int len;
unsigned int Msg_ID;
};

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

void construct_Msg(unsigned short type, char src[20], char dst[20], unsigned int len_net, unsigned int MsgID, char *data, char*msg){
      memcpy(msg,&type,2);
      memcpy(msg+2,src,20);
      memcpy(msg+22,dst,20);
      memcpy(msg+42,&len_net,4);
      memcpy(msg+46,&MsgID,4);
      if(data!=NULL){memcpy(msg+50,data,ntohl(len_net));}
}

void print_hello_buffer(char *buf){
    unsigned short type;
    char src[20];
    char dest[20];
    unsigned int len1;
    unsigned int len2;
    unsigned int msg_id;
    char data[MAX_DATA_SIZE];
    memcpy(&type,buf,2);
    memcpy(src,buf+2,20);
    memcpy(dest,buf+22,20);
    memcpy(&len1,buf+42,4);
    memcpy(&msg_id,buf+46,4);

    printf("\nmsg1 type: %d\n",ntohs(type));
    printf("msg1 src: %s\n",src);
    printf("msg1 dest: %s\n",dest);
    printf("msg1 len: %d\n",ntohl(len1));
    printf("msg1 id: %d\n",ntohl(msg_id));

    memcpy(&type,buf+50,2);
    memcpy(src,buf+52,20);
    memcpy(dest,buf+72,20);
    memcpy(&len2,buf+92,4);
    memcpy(&msg_id,buf+96,4);
    memcpy(data,buf+100,MAX_DATA_SIZE);

    printf("\nmsg2 type: %d\n",ntohs(type));
    printf("msg2 src: %s\n",src);
    printf("msg2 dest: %s\n",dest);
    printf("msg2 len: %d\n",ntohl(len2));
    printf("msg2 id: %d\n",ntohl(msg_id));
    
    printf("msg2 data: ");
    int i=0;
    while (i<=ntohl(len1)+ntohl(len2))
    {
      printf("%c",data[i]);
      i++;
    }
    printf("\n\n");
}

void print_regular_buffer(char *buf, int bytes){
    unsigned short type;
    char src[20];
    char dest[20];
    unsigned int len;
    unsigned int msg_id;
    char data[MAX_DATA_SIZE];
    memcpy(&type,buf,2);
    memcpy(src,buf+2,20);
    memcpy(dest,buf+22,20);
    memcpy(&len,buf+42,4);
    memcpy(&msg_id,buf+46,4);

    printf("\nmsg type: %d\n",ntohs(type));
    printf("msg src: %s\n",src);
    printf("msg dest: %s\n",dest);
    printf("msg len: %d\n",ntohl(len));
    printf("msg id: %d\n",ntohl(msg_id));

    if(bytes>50){
      memcpy(data,buf+50,MAX_DATA_SIZE);
      printf("msg data: ");
      int i=0;
      while (i<=ntohl(len))
      {
        printf("%c",data[i]);
        i++;
      }
      printf("\n\n");
    }
}


int main(int argc, char **argv) {
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0) 
      error("ERROR connecting");

    /* send the HELLO message line to the server */
    char *msg1 = (char*)malloc(50);
    bzero(msg1,50);
    construct_Msg(htons(1), username, "Server", htonl(0), htonl(0), NULL,  msg1);
    n = write(sockfd, msg1,50);
    if (n < 0) 
      error("ERROR writing to socket");

    /* print the server's reply to HELLO message*/
    char *buf = (char*)malloc(BUFSIZE);
    bzero(buf, BUFSIZE);
    n = read(sockfd, buf, BUFSIZE);
    printf("\nReceived %d bytes respond wrt HELLO message from server\n", n);
    if (n < 0) 
      error("ERROR reading from socket");
    print_hello_buffer(buf);

    // /* send the LIST_REQUEST message line to the server */
    // char *msg2 = (char*)malloc(50);
    // bzero(msg2,50);
    // construct_Msg(htons(3), username, "Server", htonl(0), htonl(0), NULL,  msg2);
    // n = write(sockfd, msg2,50);
    // printf("writing %d bytes to server\n",n);
    // if (n < 0) 
    //   error("ERROR writing to socket");
    
    // /* print the server's reply to LIST_REQUEST message*/
    // char *buf2 = (char*)malloc(BUFSIZE);
    // bzero(buf2, BUFSIZE);
    // n = read(sockfd, buf2, BUFSIZE);
    // printf("\nReceived %d bytes respond wrt LIST_REQUEST message from server\n", n);
    // if (n < 0) 
    //   error("ERROR reading from socket");
    // print_regular_buffer(buf2,n);

    /* read the CHAT message line from the server */
    char *msg2 = (char*)malloc(BUFSIZE);
    bzero(msg2,BUFSIZE);
    n = read(sockfd, msg2, BUFSIZE);
    printf("read %d bytes from server\n",n);
    if (n < 0) 
      error("ERROR writing to socket");

    struct Msg *sub=malloc(sizeof(struct Msg));
    char data[400];
    bzero(sub,sizeof(struct Msg));
    bzero(data,400);
    memcpy(sub, msg2, sizeof(struct Msg));
    memcpy(data, msg2+50, ntohl(sub->len));

    printf("type: %d, src: %s, dst: %s, len: %d, msg ID: %d, data: %s\n", ntohs(sub->type), sub->src, sub->dest, ntohl(sub->len), ntohl(sub->Msg_ID), data);

    sleep(6);

    /* send the EXIT message line to the server */
    char *msg3 = (char*)malloc(sizeof(50));
    bzero(msg1,50);
    construct_Msg(htons(6), username, "Server", htonl(0), htonl(2147483647), NULL,  msg3);

    n = write(sockfd, msg3, 27);
    if (n < 0) 
      error("ERROR writing to socket");
      
    sleep(2);
    
    n = write(sockfd, msg3+27, 23);
    if (n < 0) 
      error("ERROR writing to socket");

    sleep(10);

    return 0;
}