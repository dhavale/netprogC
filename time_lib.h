#ifndef TIME_LIB_H
#define TIME_LIB_H

#include <linux/if_ether.h>
#include <sys/un.h>
#include <netdb.h>
#include <sys/socket.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define IP_LEN 16
#define MSG_LEN 40

#define UNIX_D_PATH "unix2159.dg"
typedef struct msg_send_packet{

char ip[IP_LEN];
int dest_port;
char msg[MSG_LEN];
int force_flag;

}spacket;

typedef struct msg_recv_packet{
char ip[IP_LEN];
int src_port;
char msg[MSG_LEN];

}rpacket;

int msg_recv(int sockfd,char* msg_recvd,char* ip,int* src_port);

int msg_send(int sockfd, char* ip,int dest_port,char* msg,int flag);


#endif
