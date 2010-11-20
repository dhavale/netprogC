#include "odr_lib.h"

t_route *head= NULL;
qnode *qhead=NULL;



qnode* enqueue(t_odrp *packet)
{
	qnode *node = (qnode*)malloc(sizeof(qnode));
	
	memcpy(&node->packet,packet,sizeof(t_odrp));
	
	node->next= qhead;
	
	return node;
}


char * get_name(unsigned long ip)
{
	struct hostent * host;
	host= gethostbyaddr(&ip,4,AF_INET);
	
	printf("\nfor ip %s hname is %s\n",(char *)inet_ntoa(ip),host->h_name);
	return host->h_name;	
}

qnode * dequeue( unsigned long dest_ip)
{
	qnode *node=qhead;
	qnode *prev= NULL;
	while(node)
	{
		if(node->packet.dest_ip==dest_ip)
			break;
		prev=node;
		node=node->next;

	}

	if(node==NULL)
		return NULL;

	if(prev==NULL)
	{
		qhead=qhead->next;
	}
	else{
		prev->next=node->next;
	}
	
	return node;
}

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
		E. If REP_ALREADY_SENT is set, just update entry.

	*/
	char dest_mac[6];
	int i=0;
	time_t ts = time(NULL);
	t_route *entry ;
	t_odrp reply;
	
	memset(&reply,0,sizeof(reply));

	/*passive entry for back path*/
	add_route_entry(odr_packet->source_ip,from_ifindex,src_mac,odr_packet->hop_count+1,ts,
				odr_packet->flag&FORCED_ROUTE);

	if((odr_packet->dest_ip==eth0_ip.sin_addr.s_addr)&&(!(odr_packet->flag&REP_ALREADY_SENT)))
	{
		reply.type= RREP;
		reply.source_ip = eth0_ip.sin_addr.s_addr;
		reply.dest_ip  = odr_packet->source_ip;
		reply.hop_count = 0;
		memcpy(dest_mac,src_mac,IF_HADDR);
		i=0;
		while(i<total_if_count)
		{
			if(from_ifindex==if_list[i].if_index)
				break;
			i++;	
		}	
		assert(i<total_if_count);
	
		send_pf_packet(sockfd,i,dest_mac,&reply);
	
	}

	else if((odr_packet->flag)||((entry=find_route_entry(odr_packet->dest_ip,ts))==NULL))
	{
		/*broadcast without looking for routing entry*/
		memset(dest_mac,0xff,IF_HADDR);
		odr_packet->hop_count++;
		i=0;	
		while(i<total_if_count)
		{
			if(if_list[i].if_index==from_ifindex)
			{
				i++;
				continue;
			}
				
		send_pf_packet(sockfd,i,dest_mac,odr_packet);
	
		}
		
	}
	else{
			
		reply.type= RREP;
		reply.source_ip = entry->dest_ip;
		reply.dest_ip  = odr_packet->source_ip;
		reply.hop_count = entry->hop_count;
		memcpy(dest_mac,entry->neighbour,IF_HADDR);
		i=0;
		while(i<total_if_count)
		{
			if(from_ifindex==if_list[i].if_index)
				break;
			i++;	
		}	
		assert(i<total_if_count);
	
		send_pf_packet(sockfd,i,dest_mac,&reply);
		
	/*	entry->ts=ts;
		printf("\npath reconfirmed %s to %s in %d hops",get_name(reply.source_ip),
							get_name(reply.dest_ip),reply.hop_count);
	*/
		/*broadcast with REP_ALREADY_SENT flag*/
		memset(dest_mac,0xff,IF_HADDR);
		odr_packet->hop_count++;
		odr_packet->flag|= REP_ALREADY_SENT;
		i=0;
		while(i<total_if_count)
		{
			if(if_list[i].if_index==from_ifindex)
			{
				i++;
				continue;
			}
				
		send_pf_packet(sockfd,i,dest_mac,odr_packet);
	
		}
	

	}
		
	return 0;

}
int process_RREP(int sockfd,char *src_mac,int from_ifindex,t_odrp* odr_packet)
{
	/*
		Add routing table entry.
		Add RREP to the queue if cannot be forwarded.
		Check the queue for pending RREP and Data.
	 
	*/
	qnode *node;
	time_t ts = time(NULL);
	int i=0;
	t_route *entry;
	t_odrp request;
	char dest_mac[6];

	memset(&request,0,sizeof(request));

	add_route_entry(odr_packet->source_ip,from_ifindex,src_mac,odr_packet->hop_count+1,ts,0);


	while(node=dequeue(odr_packet->source_ip))
	{
		
		i=0;
		while(i<total_if_count)
		{
			if(from_ifindex==if_list[i].if_index)
				break;
			i++;
		}	
		assert(i<total_if_count);
		
		node->packet.hop_count++;
		
		send_pf_packet(sockfd,i,src_mac,&node->packet);
		free(node);
	}	
	
	
	if(odr_packet->dest_ip==eth0_ip.sin_addr.s_addr)
	{
		/*Do nothing, we only had initiated*/
		printf("\npath from %s to %s in %d hops\n",get_name(eth0_ip.sin_addr.s_addr),
					get_name(odr_packet->source_ip),odr_packet->hop_count);
		return;
	}
	entry= find_route_entry(odr_packet->dest_ip,ts);

	if(entry==NULL)
	{
		qhead= enqueue(odr_packet);
		request.type=RREQ;
		request.source_ip = eth0_ip.sin_addr.s_addr;
		request.dest_ip = odr_packet->dest_ip;
		request.hop_count = 0;
		
		memset(dest_mac,0xff,IF_HADDR);

		i=0;
		while(i<total_if_count)
		{
			if(if_list[i].if_index==from_ifindex)
			{
				i++;
				continue;
			}
			send_pf_packet(sockfd,i,dest_mac,&request);
		}
	}
	else{

		memcpy(dest_mac,entry->neighbour,IF_HADDR);
		odr_packet->hop_count++;
		
		i=0;
		while(i<total_if_count)
		{
			if(if_list[i].if_index==entry->if_index)
				break;
			i++;
		}	
		send_pf_packet(sockfd,i,dest_mac,odr_packet);
	}	
	return 0;
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

	if(force||((ts-node->ts)>=staleness)||(hop_count<=node->hop_count)){
		node->dest_ip=	dest_ip;
		node->if_index=	if_index;
		memcpy(node->neighbour,neighbour,IF_HADDR);
		node->hop_count=hop_count;
		node->ts =	ts;

		printf("\npath from %s to %s in %d hops\n",get_name(eth0_ip.sin_addr.s_addr),
							get_name(node->dest_ip),node->hop_count);
	}
	else{

		/*do nothing, table is upto date*/
	}
			
	return 0;

}

int send_pf_packet(int sockfd,int index_in_if_list, char *dest_mac, t_odrp *odr_packet)
{
	/*target address*/
	struct sockaddr_ll socket_dest_address;
	char* src_mac=NULL;
	int i=0;
	int if_index;
	/*buffer for ethernet frame*/
	void* buffer = (void*)malloc(ETH_HDRLEN+sizeof(t_odrp));
	 
	/*pointer to ethenet header*/
	unsigned char* etherhead = buffer;
		
	/*userdata in ethernet frame*/
	unsigned char* data = buffer + 14;
		
	/*another pointer to ethernet header*/
	struct ethhdr *eh = (struct ethhdr *)etherhead;
	 
	int send_result = 0;


	assert(index_in_if_list<total_if_count);

	src_mac= if_list[i].if_haddr;	
	if_index= if_list[i].if_index;		

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

	length = recvfrom(sockfd, recv_buffer,ETH_HDRLEN+sizeof(t_odrp) , 0, (struct sockaddr*)&from,&size);

	if (length == -1) {
		perror("Recieve failed:");
		return -1;
	}

	odr_packet= (t_odrp*)recv_buffer+ETH_HDRLEN;
	src_mac = (char *) recv_buffer;
	
	switch(odr_packet->type)
	{
		case RREQ:
			process_RREQ(sockfd,src_mac,from.sll_ifindex,odr_packet);
				
		break;
		case RREP:
			process_RREP(sockfd,src_mac,from.sll_ifindex,odr_packet);
		break;
		case DREQ:
		break;
		case DREP:
		break;
	}	


		
	printf("data recieved: %s",(char*)recv_buffer+14);

	return 0;

}
