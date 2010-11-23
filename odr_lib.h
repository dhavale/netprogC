#ifndef ODR_LIB_H
#define ODR_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include "hw_addrs.h"

#define UNIX_D_PATH "unix2159.dg"
struct route_entry{
unsigned long dest_ip; 	/*Entry for which destination*/
int if_index;		/*which local interface to use*/
char neighbour[8];	/*neighbour's MAC from which destination is reachable*/
int hop_count;		/*Destination reachable in how many hops*/
time_t ts;		/*time stamp when this entry was made*/
struct route_entry *next;
};


typedef struct route_entry t_route;

struct odr_packet{
int type;
#define RREQ 0
#define RREP 1
#define APP_DATA 2

unsigned long source_ip;
unsigned long dest_ip;
int source_port;
int dest_port;
int hop_count;
int flag;
#define FORCED_ROUTE 0x01
#define	REP_ALREADY_SENT 0x02

char data[40]; /*valid only if DREQ or DREP else ignored*/
};

typedef struct odr_packet t_odrp;

struct seen_list_node{
	unsigned long source_ip;
	int	broadcast_id;
	int 	hop_count;
	struct seen_list_node *next;
};

int is_dup_req(unsigned long source_ip,int broadcast_id,int hop_count);


struct queue_node {
t_odrp packet;
struct queue_node *next;
};

typedef struct queue_node qnode;
typedef struct seen_list_node snode;

extern snode *shead;

extern qnode *qhead;

extern t_route *head;

t_route * find_route_entry(unsigned long dest_ip,time_t ts);

int add_route_entry(unsigned long dest_ip,int if_index, char* neighbour,int hop_count,time_t ts,int force);

int send_pf_packet(int sockfd,int index_in_if_list, char *dest_mac, t_odrp *odr_packet);

char *get_name(unsigned long);

int recv_process_pf_packet(int,int );

int is_dup_req(unsigned long source_ip,int broadcast_id,int hop_count);
qnode* enqueue(t_odrp *packet);
qnode * dequeue( unsigned long dest_ip);

#endif
