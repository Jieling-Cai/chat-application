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
#include <time.h>

#define BUFSIZE 1048576
#define MAX_DATA_SIZE 400
#define Wait_Time 60

struct __attribute__((__packed__)) Msg{
unsigned short type;
char src[20];
char dest[20];
unsigned int len;
unsigned int Msg_ID;
};

typedef struct __attribute__((__packed__)) ClientList{
  char clientID[20];
  int socket;
  struct ClientList *next;
}ClientList;

typedef struct __attribute__((__packed__)) partialMsg{
  char msg[sizeof(struct Msg)+MAX_DATA_SIZE];
  unsigned int len;
  time_t timestamp;
  int socket;
  struct partialMsg *next;
}par_msg;

// head node
ClientList *client_start, *client_end;
par_msg *par_msg_start, *par_msg_end;

void construct_Msg(unsigned short type, char src[20], char dst[20], unsigned int len_net, unsigned int MsgID, char *data, char*msg){
      memcpy(msg,&type,2);
      memcpy(msg+2,src,20);
      memcpy(msg+22,dst,20);
      memcpy(msg+42,&len_net,4);
      memcpy(msg+46,&MsgID,4);
      if(data!=NULL){memcpy(msg+50,data,ntohl(len_net));}
}

int get_client_list_msg(char*client_msg, char src[20]){
    	ClientList *ptr = client_start->next;
      char* LIST = (char*)malloc(MAX_DATA_SIZE);
      bzero(LIST,MAX_DATA_SIZE);
      unsigned int len_total=strlen(LIST);
      while(ptr != NULL) 
      { 
        if(len_total==0){
          memcpy(LIST, ptr->clientID, strlen(ptr->clientID));
          *(LIST+strlen(ptr->clientID))='\0';
        }
        else{
          memcpy(LIST+len_total, ptr->clientID, strlen(ptr->clientID));
          *(LIST+len_total+strlen(ptr->clientID))='\0'; 
        }
        len_total+=strlen(ptr->clientID);
        len_total++;
        ptr = ptr->next;
      }
      printf("\ncurrent client list: ");
      for(int i=0;i<=len_total;i++)
      { 
        if(*(LIST+i)=='\0'){printf(" ");}
        else{printf("%c",*(LIST+i));}
      }
      
      printf("\ncurrent client list length: %d\n",len_total);
      
      construct_Msg(htons(4), "Server", src, htonl(len_total), htonl(0), LIST, client_msg);

      free(LIST);
      return len_total;
}

int get_client_list_len(){
    	ClientList *ptr = client_start->next;
      unsigned int len_total=0;
      while(ptr != NULL) 
      { 
        len_total+=strlen(ptr->clientID);
        len_total++;
        ptr = ptr->next;
      }
      return len_total;
}

void delet_client(int socket){
	ClientList *t = client_start;
  ClientList *in = (ClientList*)malloc(sizeof(ClientList));
  bzero(in,sizeof(ClientList));
	while (t->next!= NULL) {
    if(t->next->socket==socket){in = t;t=t->next;break;}
		t = t->next;
	}

	if (in->next!= NULL) {
		in->next = t->next;
		free(t);
    
    if (client_start->next==NULL) { 
      client_end=client_start;
    }
    else{
      ClientList *temp = client_start;
      while(temp->next!=NULL){
        temp=temp->next;
      }
      client_end=temp;
    }
	}
	else {
		printf("Found the socket %d does not exist when trying to delete the corresponding client\n", socket);
	}
}

void delet_partial_msg(int socket){
	par_msg *t = par_msg_start;
  par_msg *in = (par_msg*)malloc(sizeof(par_msg));
  bzero(in,sizeof(par_msg));
	while (t->next!= NULL) {
    if(t->next->socket==socket){in = t;t=t->next;break;}
		t = t->next;
	}
	if (in->next!= NULL) {
		in->next = t->next;
    free(t);

    if (par_msg_start->next==NULL) { 
    par_msg_end=par_msg_start;
    }
    else{
    par_msg *temp = par_msg_start;
    while(temp->next!=NULL){
      temp=temp->next;
    }
    par_msg_end=temp;
    }
	}
	else {
		printf("Found the socket %d does not exist when trying to delete corresponding partial/intact message\n", socket);
	}
}

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int max(int a, int b)
{
return a>b?a:b;
}

ClientList* find_target_client_pointer(char clientID[20]){
  ClientList* l = client_start->next;
  while (l!=NULL)
  {
    if(!strcmp(l->clientID,clientID)){break;}
    l=l->next;
  }
  return l;
}

void find_corres_clientID_using_socket(int socket, char tar_clientID[20])
{
  ClientList* m = client_start->next;
  while (m!=NULL)
  {
    if(m->socket==socket){memcpy(tar_clientID,m->clientID,20);}
    m=m->next;
  }
}

int decide_valid_msg(unsigned int len, char msg[sizeof(struct Msg)+MAX_DATA_SIZE]){
  unsigned short type;
  if(len>=2){
    memcpy(&type,msg,2);
    type = ntohs(type);
    if(type!=1&&type!=2&&type!=3&&type!=4&&type!=5&&type!=6&&type!=7&&type!=8)
    {return 0;}
  }
  return 1;
}

int main(int argc, char **argv)
{
  int listenfd; /* listening socket */
  int clientSocket; /* connection socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char *buf; /* message buffer */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  client_start=(ClientList*)malloc(sizeof(ClientList));
  client_end=client_start;
  par_msg_start=(par_msg*)malloc(sizeof(par_msg));
  par_msg_end=par_msg_start;

  /* check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* socket: create a socket */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) 
    error("ERROR opening server socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /* build the server's internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET; /* we are using the Internet */
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  serveraddr.sin_port = htons((unsigned short)portno); /* port to listen on */

  /* bind: associate the listening socket with a port */
  if (bind(listenfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* listen: make it a listening socket ready to accept connection requests */
  if (listen(listenfd, 1000) < 0) /* allow 1000 requests to queue up */ 
    error("ERROR on listen");

  clientlen = sizeof(clientaddr);
  fd_set master_set;
  fd_set temp_set;

  FD_ZERO(&master_set);
  FD_ZERO(&temp_set);

  FD_SET(listenfd,&master_set);
  int fdmax = listenfd;

  struct timeval *time_val = malloc(sizeof(struct timeval));
  time_val->tv_sec=Wait_Time;
  time_val->tv_usec=0;

  while(1){
    temp_set = master_set;
    select(fdmax+1, &temp_set, NULL, NULL, time_val);
    
    if(FD_ISSET(listenfd, &temp_set)){ //new client is coming, then accept();

      clientSocket = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);
      if (clientSocket < 0) 
          error("ERROR on accept");
      printf("\n******************************************************************************\n");
      printf("\nA new client socket %d created\n",clientSocket);

      FD_SET(clientSocket,&master_set);
      fdmax = max(clientSocket, fdmax);
    }
    else{ //it is an existing client socket, then read();
      for(int socket=0; socket<=fdmax; ++socket){
        par_msg *k = par_msg_start->next;
        while (k!=NULL)
        {
          if(k->socket==socket){
            if(time(NULL)-k->timestamp>=Wait_Time){
              printf("\n******************************************************************************\n");
              printf("\nTimeout after waiting %d seconds for partial message!\n", time(NULL)-k->timestamp);
              printf("\nThe client on socket %d will be deleted\n",socket);
              delet_client(socket);
              delet_partial_msg(socket);
              printf("socket %d was deleted\n",socket);
              FD_CLR(socket,&master_set);
              close(socket);
              break;
            }
          }
          k=k->next;
        } 
        // take the socket that has sth to read
        if(FD_ISSET(socket,&temp_set)){
            buf = (char*)malloc(BUFSIZE);
            bzero(buf,BUFSIZE);
            
            n = read(socket, buf, BUFSIZE);
            
            /* error handling: client socket closed/receive too long message/ERROR reading from socket */
            // ERROR reading from socket
            if (n < 0){
              printf("\n******************************************************************************\n");
              printf("\nERROR reading from socket\n");
              printf("\nThe client on socket %d will be deleted\n",socket);
              delet_client(socket);
              delet_partial_msg(socket);
              printf("socket %d was deleted\n",socket);
              FD_CLR(socket,&master_set);
              close(socket);
            } 
            // client socket closed
            else if (n == 0){ 
              printf("\n******************************************************************************\n");
              printf("\nThe client on socket %d will be deleted because client has closed its connection\n",socket);
              delet_client(socket);
              delet_partial_msg(socket);
              printf("socket %d was deleted\n",socket);
              FD_CLR(socket,&master_set);
              close(socket);
            } 
            // received too long message
            else if (n > (sizeof(struct Msg)+MAX_DATA_SIZE)){
              printf("\n******************************************************************************\n");
              printf("\nReceived %d bytes, too long invalid message!\n", n);
              printf("\nThe client on socket %d will be deleted\n",socket);
              delet_client(socket);
              delet_partial_msg(socket);
              printf("socket %d was deleted\n",socket);
              FD_CLR(socket,&master_set);
              close(socket);
            }  
            // Received bytes length is reasonable  
            else{
                par_msg *par_temp = par_msg_start->next;
                /* find if there is an existing socket that corresponds, if not then create */
                while (par_temp!=NULL)
                { 
                  if(par_temp->socket==socket){ // find existing socket that corresponds
                    memcpy(par_temp->msg+par_temp->len,buf,n);
                    par_temp->len+=n;
                    break;
                  } 
                  par_temp=par_temp->next;
                }
                
                if(par_temp==NULL){ // do not find existing socket that corresponds
                  /* Create partial message node of this socket */         
                  par_msg *p_msg = malloc(sizeof(par_msg));
                  bzero(p_msg,sizeof(par_msg));
                  memcpy(p_msg->msg,buf,n);
                  p_msg->len=n;  
                  p_msg->socket=socket;
                  /* Adding partial message node to the end of the list */  
                  par_msg_end->next = p_msg;
                  par_msg_end = p_msg;
                  par_temp = par_msg_end;
                  par_msg_end->next = NULL;
                }

                par_temp->timestamp=time(NULL);
                // invalid type message received 
                if(!decide_valid_msg(par_temp->len, par_temp->msg))
                {   
                    printf("\n******************************************************************************\n");
                    printf("\nReceived invalid message type\n");
                    printf("\nThe client on socket %d will be deleted\n",socket);
                    delet_partial_msg(socket);
                    delet_client(socket);
                    printf("\nThe client on socket %d was deleted\n",socket);
                    FD_CLR(socket,&master_set);
                    close(socket);
                }
                // segment using structure until receiving at least intact header
                else if (par_temp->len>=sizeof(struct Msg)){
                  struct Msg *sub=malloc(sizeof(struct Msg));
                  bzero(sub,sizeof(struct Msg));
                  memcpy(sub, par_temp->msg, sizeof(struct Msg));
                  // received bytes number is larger than header specified, so wrong
                  if(par_temp->len-sizeof(struct Msg)>ntohl(sub->len)){
                    printf("\n******************************************************************************\n");
                    printf("\nReceived data size is larger than header specified\n");
                    printf("\nThe client %s on socket %d will be deleted\n",sub->src,socket);
                    delet_partial_msg(socket);
                    delet_client(socket);
                    printf("\nThe client %s on socket %d was deleted\n",sub->src,socket);
                    FD_CLR(socket,&master_set);
                    close(socket);
                  } 
                  // received bytes number is <= header specified, possibly correct
                  else{
                      /* server received correct message that is not chat message */
                      if(ntohl(sub->len)==0 && par_temp->len==sizeof(struct Msg)){ 
                  
                        /* receive HELLO message */
                        if (ntohs(sub->type)==1)
                        {
                          printf("\nReceived %d bytes HELLO message from socket %d, want to register clientID: %s\n", par_temp->len, socket, sub->src);

                          if(find_target_client_pointer(sub->src)!=NULL){
                            /*construct ERROR(CLIENT_ALREADY_PRESENT) message*/
                            char *CLIENT_PRESENT=(char*)malloc(sizeof(struct Msg));
                            bzero(CLIENT_PRESENT,sizeof(struct Msg));
                            construct_Msg(htons(7), "Server", sub->src, htonl(0), htonl(0), NULL, CLIENT_PRESENT);
                            write(socket, CLIENT_PRESENT, sizeof(struct Msg));

                            printf("\n******************************************************************************\n");
                            printf("\nReceived clientID already exists\n");
                            printf("\nThe client on socket %d will be deleted\n", socket);
                            delet_partial_msg(socket);
                            delet_client(socket);
                            printf("\nThe client on socket %d was deleted\n", socket);
                            free(CLIENT_PRESENT);
                            FD_CLR(socket,&master_set);
                            close(socket);
                          } 
                          else if (get_client_list_len()+strlen(sub->src)+1>MAX_DATA_SIZE){
                            printf("\n******************************************************************************\n");
                            printf("\nclient list is already full, cannot register clientID for now\n");
                            printf("\nThe client on socket %d will be deleted\n", socket);
                            delet_partial_msg(socket);
                            delet_client(socket);
                            printf("\nThe client on socket %d was deleted\n", socket);
                            FD_CLR(socket,&master_set);
                            close(socket);
                          }
                          else{
                            ClientList *object = malloc(sizeof(ClientList));
                            bzero(object,sizeof(ClientList));
                            memcpy(object->clientID,sub->src,20);
                            object->socket=socket;

                            /* Adding node to the end of the list*/  
                            client_end->next = object;
                            client_end = object;
                            client_end->next = NULL;

                            char *HELLO_ACK=(char*)malloc(sizeof(struct Msg));
                            bzero(HELLO_ACK,sizeof(struct Msg));
                            /*construct HELLO_ACK Msg*/
                            construct_Msg(htons(2), "Server", sub->src, htonl(0), htonl(0), NULL, HELLO_ACK);
                    
                            /*construct CLIENT_LIST Msg*/
                            char *client_msg=(char*)malloc(sizeof(struct Msg)+MAX_DATA_SIZE);
                            bzero(client_msg,sizeof(struct Msg)+MAX_DATA_SIZE);
                            int len_total=get_client_list_msg(client_msg,sub->src);
                            free(sub);
                            
                            /*concate two Msgs*/
                            char *msg_all=(char*)malloc(sizeof(struct Msg)*2+MAX_DATA_SIZE);
                            bzero(msg_all,sizeof(struct Msg)*2+MAX_DATA_SIZE);
                            memcpy(msg_all,HELLO_ACK,sizeof(struct Msg));
                            memcpy(msg_all+sizeof(struct Msg),client_msg,sizeof(struct Msg)+len_total);

                            free(HELLO_ACK);
                            free(client_msg);

                            /* send back all msgs*/
                            write(socket, msg_all, sizeof(struct Msg)*2+len_total);
                            delet_partial_msg(socket);
                            free(msg_all);
                          }
                        }

                        /* server received LIST_REQUEST message */
                        if (ntohs(sub->type)==3)
                        {        
                          char t_clientID[20];
                          bzero(t_clientID,20);
                          find_corres_clientID_using_socket(socket, t_clientID);
                          if (!strcmp(t_clientID,sub->src)) // Coming Msg clientID and socket match
                          {
                            printf("\nReceived %d bytes LIST_REQUEST message from socket %d, clientID: %s\n", par_temp->len, socket, sub->src);
                            /*construct CLIENT_LIST Msg*/
                            char *client_msg=(char*)malloc(sizeof(struct Msg)+MAX_DATA_SIZE);
                            bzero(client_msg,sizeof(struct Msg)+MAX_DATA_SIZE);
                            int len_total=get_client_list_msg(client_msg,sub->src);
                            free(sub);
                            write(socket, client_msg, sizeof(struct Msg)+len_total);
                            delet_partial_msg(socket);
                            free(client_msg);
                          }
                          else{
                            printf("\n******************************************************************************\n");
                            printf("\nClient wrongly sent LIST_REQUEST message using other client's ID or this ID does not exist\n");
                            printf("\nThe client on socket %d will be deleted\n", socket);
                            delet_partial_msg(socket);
                            delet_client(socket);
                            printf("\nThe client on socket %d was deleted\n", socket);
                            FD_CLR(socket,&master_set);
                            close(socket);
                          }
                        }

                        /* server received EXIT message */
                        if (ntohs(sub->type)==6)
                        {
                          char t_clientID[20];
                          bzero(t_clientID,20);
                          find_corres_clientID_using_socket(socket, t_clientID);
                          printf("\n******************************************************************************\n");
                          if (!strcmp(t_clientID,sub->src)) // Coming Msg clientID and socket match
                          {
                            printf("\nReceived %d bytes EXIT message from socket %d, clientID: %s\n", par_temp->len, socket, sub->src);
                            printf("\nThe client %s on socket %d requested to exit\n",sub->src,socket);
                            delet_partial_msg(socket);
                            delet_client(socket);
                            printf("\nThe client %s on socket %d was deleted\n",sub->src,socket);
                            FD_CLR(socket,&master_set);
                            close(socket);
                          }
                          else{
                            printf("\n******************************************************************************\n");
                            printf("\nClient wrongly sent LIST_REQUEST message using other client's ID or this ID does not exist\n");
                            printf("\nThe client on socket %d will be deleted\n", socket);
                            delet_partial_msg(socket);
                            delet_client(socket);
                            printf("\nThe client on socket %d was deleted\n", socket);
                            FD_CLR(socket,&master_set);
                            close(socket);
                          }
                        }
                        
                      } // is not chat msg
                      /* server received message that is chat message and bytes length is correct*/ 
                      else if (ntohs(sub->type)==5 && par_temp->len-sizeof(struct Msg)==ntohl(sub->len)){
                        printf("\n******************************************************************************\n");
                        printf("\nReceived %d bytes chat message from client ID: %s\n", par_temp->len, sub->src);
                        
                        char t_clientID[20];
                        bzero(t_clientID,20);
                        find_corres_clientID_using_socket(socket, t_clientID);
                        if (!strcmp(t_clientID,sub->src)) // Coming Msg clientID and socket match
                        {
                            printf("\nRecipient ID is: %s, start to forward\n", sub->dest);
                            // find corresponding recipient socket
                            int rec_socket;
                            ClientList *ptr = client_start->next;
                            while(ptr!=NULL){  
                              if(!strcmp(ptr->clientID,sub->dest)){rec_socket=ptr->socket;break;}
                              ptr=ptr->next;
                            }
                            int n0;
                            // forward only when recipient is present
                            if(ptr!=NULL){
                              if(sub->dest==NULL){
                                printf("\n******************************************************************************\n");
                                printf("\nRecipient ID is NULL\n");
                                printf("\nThe client %s on socket %d will be deleted\n",sub->src,socket);
                                delet_partial_msg(socket);
                                delet_client(socket);
                                printf("\nThe client %s on socket %d was deleted\n",sub->src,socket);
                                FD_CLR(socket,&master_set);
                                close(socket);
                              }
                              else if(!strcmp(sub->dest,sub->src)){
                                printf("\n******************************************************************************\n");
                                printf("\nclient cannot try to chat with herself\n");
                                printf("\nThe client %s on socket %d will be deleted\n",sub->src,socket);
                                delet_partial_msg(socket);
                                delet_client(socket);
                                printf("\nThe client %s on socket %d was deleted\n",sub->src,socket);
                                FD_CLR(socket,&master_set);
                                close(socket);
                              }
                              else{                   
                                n0=write(rec_socket, par_temp->msg, par_temp->len);
                                printf("\nForwarded %d bytes to the recipient on the socket %d\n", n0, rec_socket);
                                delet_partial_msg(socket);
                              }
                            }   
                            // recipient ID is not present, then discard the message and send an ERROR(CANNOT_DELIVER) message back to client 
                            else {
                              /* construct ERROR(CANNOT_DELIVER) message & send back to client */
                              char *CANNOT_DELIVER=(char*)malloc(sizeof(struct Msg));
                              bzero(CANNOT_DELIVER,sizeof(struct Msg));
                              construct_Msg(htons(8), "Server", sub->src, htonl(0), sub->Msg_ID, NULL, CANNOT_DELIVER);
                              write(socket, CANNOT_DELIVER, sizeof(struct Msg));

                              printf("\n******************************************************************************\n");
                              printf("\nThe recipient client doesn’t exist\n");
                              printf("\nThe client on socket %d will be deleted\n", socket);
                              delet_partial_msg(socket);
                              delet_client(socket);
                              printf("\nThe client on socket %d was deleted\n", socket);
                              free(CANNOT_DELIVER);
                              FD_CLR(socket,&master_set);
                              close(socket);
                            }
                        }
                        else{
                          printf("\n******************************************************************************\n");
                          printf("\nClient wrongly sent CHAT message using other client's ID or this ID does not exist\n");
                          printf("\nThe client on socket %d will be deleted\n", socket);
                          delet_partial_msg(socket);
                          delet_client(socket);
                          printf("\nThe client on socket %d was deleted\n", socket);
                          FD_CLR(socket,&master_set);
                          close(socket);
                        }
                    } // server received message that is chat message and bytes length is correct (else if)
                  } // received bytes number is <= header specified, possibly correct
                } // segment using structure until receiving at least intact header
              } // Received bytes length is reasonable  
           free(buf);
        } // take the socket that has sth to read
      } // for loop for each socket
    } //it is an existing client socket, then read();
  } // endless while loop
} // end int main()