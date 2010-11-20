#include "odr_lib.h"

t_route *head= NULL;


t_route * find_route_entry(unsigned long dest_ip,time_t ts)
{
	t_route *node;

	if(head==NULL)
		return NULL;
	node= head;
	while(node)
	{
		if(node->dest_ip==dest_ip)
			break;
		else node=node->next;
	}		
	if(node==NULL)
		return NULL;

	if((ts-node->ts)>=staleness)
		return NULL;
	else 	return node;

}


int process_RREQ(int sockfd,char* src_mac,int from_ifindex,t_odrp* odr_packet)
{
	/*
		A. Broadcast on all interfaces except from_ifindex.
		B. If path to destination is known, send RREP but broadcast RREQ with
		   REP_ALREADY_SENT. 
		C. Add reverse path into the routing table.
		D. Handle forced discovery if necessary.
	*/


}
int process_RREP(int sockfd,char *src_mac,int from_ifindex,t_odrp* odr_packet)
{

}

int add_route_entry(unsigned long dest_ip, int if_index, char* neighbour,int hop_count, time_t ts,int force)
{
	t_route *node = head;

	while(node)
	{
		if(node->dest_ip==dest_ip)
			break;
		else node=node->next;
	}
	
	if(node==NULL){
		node = (t_route *)malloc(sizeof(t_route));
		memset(node,0,sizeof(t_route));
		node->next=head;
		head = node;
	}

	if(force||((ts-node->ts)>=staleness)||(hop_count<node->hop_count)){
		node->dest_ip=	dest_ip;
		node->if_index=	if_index;
		memcpy(node->neighbour,neighbour,IF_HADDR);
		node->hop_count=hop_count;
		node->ts =	ts;
	}
	else{

		/*do nothing, table is upto date*/
	}
			
		

}

int send_pf_packet(int sockfd,int if_index, char *dest_mac, t_odrp *odr_packet)
{
	/*target address*/
	struct sockaddr_ll socket_dest_address;
	char* src_mac=NULL;
	int i=0;
	/*buffer for ethernet frame*/
	void* buffer = (void*)malloc(ETH_HDRLEN+sizeof(t_odrp));
	 
	/*pointer to ethenet header*/
	unsigned char* etherhead = buffer;
		
	/*userdata in ethernet frame*/
	unsigned char* data = buffer + 14;
		
	/*another pointer to ethernet header*/
	struct ethhdr *eh = (struct ethhdr *)etherhead;
	 
	int send_result = 0;


	while(i<total_if_count)
	{
		if(if_list[i].if_index==if_index){
			src_mac= if_list[i].if_haddr;	
			break;
		}
		i++;
		
	}

	if(!src_mac){
		free(buffer);
		return -1;
	}
		
	memset(&socket_dest_address,0,sizeof(socket_dest_address));	
	/*prepare sockaddr_ll*/
		
	/*RAW communication*/
	socket_dest_address.sll_family   = PF_PACKET;	
	/*we don't use a protocoll above ethernet layer
	  ->just use anything here*/
	socket_dest_address.sll_protocol = htons(ODR_K2159);	
	
	/*index of the network device
	see full code later how to retrieve it*/
	socket_dest_address.sll_ifindex  = if_index;
	
	/*address length*/
	socket_dest_address.sll_halen    = ETH_ALEN;		
	/*MAC - begin*/
	memcpy(socket_dest_address.sll_addr,dest_mac,ETH_ALEN);
	/*MAC - end*/
	
	/*set the frame header*/
	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = htons(ODR_K2159);
	/*fill the frame with some data*/
	/*for (j = 0; j < 1500; j++) {
		data[j] = (unsigned char)((int) (255.0*rand()/(RAND_MAX+1.0)));
	}
	*/

	memcpy(data,odr_packet,sizeof(t_odrp));

	/*send the packet*/
	send_result = sendto(sockfd, buffer, ETH_HDRLEN+sizeof(t_odrp), 0, 
		      (struct sockaddr*)&socket_dest_address, sizeof(socket_dest_address));
	if (send_result == -1) { 
		perror("Send failed..:");
		exit(EXIT_FAILURE);
	}
	return 0;
}

int recv_process_pf_packet(int sockfd)
{
	void* recv_buffer = (void*)malloc(ETH_HDRLEN+sizeof(t_odrp)); /*Buffer for ethernet frame*/
	struct sockaddr_ll from;
	int size = sizeof(struct sockaddr_ll);
	t_odrp *odr_packet;
	memset(&from,0,sizeof(from));
	char *src_mac;

	int length = 0; /*length of the received frame*/ 

	length = recvfrom(sockfd, recv_buffer,ETH_HDRLEN+sizeof(t_odrp) , 0, &from,&size);

	if (length == -1) {
		perror("Recieve failed:");
		return -1;
	}

	odr_packet= (t_odrp*)recv_buffer+ETH_HDRLEN;
	src_mac = (char *) recv_buffer;
	
	switch(odr_packet->type)
	{
		case RREQ:
			process_RREQ(sockfd,src_mac,from->sll_ifindex,odr_packet);
				
		break;
		case RREP:
			process_RREP(sockfd,src_mac,from->sll_ifindex,odr_packet);
		break;
		case DREQ:
		break;
		case DREP:
		break;
	}	


		
	printf("data recieved: %s",(char*)recv_buffer+14);

	return 0;

}
