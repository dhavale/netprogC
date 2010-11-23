#ifndef _ODR_H
#define _ODR_H

#include <sys/un.h>

#define IP_LEN 16
#define MSG_LEN 40
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



#endif
