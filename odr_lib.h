#ifndef ODR_LIB_H
#define ODR_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "hw_addrs.h"

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
#define DREQ 2
#define DREP 3
unsigned long source_ip;
unsigned long dest_ip;
int source_port;
int dest_port;
int hop_count;
int flag;
char data[28]; /*valid only if DREQ or DREP else ignored*/
};

typedef struct odr_packet t_odrp;

extern t_route *head;

t_route * find_route_entry(unsigned long dest_ip,time_t ts);

int add_route_entry(unsigned long dest_ip,int if_index, char* neighbour,int hop_count,time_t ts,int force);

int send_pf_packet(int sockfd,int if_index, char *dest_mac, t_odrp *odr_packet);



#endif