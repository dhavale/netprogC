#ifndef _ODR_H
#define _ODR_H

#include <sys/un.h>

#define IP_LEN 16
#define MSG_LEN 40
#define TTL 200000
#define SERVER_PATH "server.dg"


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

typedef struct app_details_node{

int port;
char sun_path[40];
time_t ts;
struct app_details_node *next;
}app_node;

extern	app_node * app_table_head;
app_node * lookup_sun_path(const char *sun_path);

app_node * lookup_port(int port);

app_node * add_app_node_details(int port, const char * sun_path);


#endif
