#include "odr_lib.h"
#include "odr.h"


#ifdef ODR_DEBUG
#define dprintf(fmt, args...) printf(fmt, ##args)
#else
#define dprintf(fmt, args...)
#endif

t_route *head= NULL;
qnode *qhead=NULL;
snode *shead=NULL;

/* returns 1 if it has seen req before, else returns 0 and updates the list*/
int is_dup_req(unsigned long source_ip,int broadcast_id,int hop_count)
{
	snode *node=shead;
	snode *new_node;	
	while(node)
	{
		if((node->source_ip==source_ip)&&(node->broadcast_id==broadcast_id)&&
			(node->hop_count>hop_count))
		{
			node->hop_count = hop_count;
			return 0;
		}
		else if((node->source_ip==source_ip)&&(node->broadcast_id==broadcast_id)&&
			(node->hop_count<=hop_count))
			return 1;
		node=node->next;
	}
	
	new_node = (snode *) malloc(sizeof(snode));
	
	new_node->source_ip=source_ip;
	new_node->broadcast_id = broadcast_id;
	new_node->hop_count = hop_count;

	new_node->next= shead;

	shead= new_node;

	return 0;
}
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
	
	dprintf("\nfor ip %s hname is %s\n",(char *)inet_ntoa(ip),host->h_name);
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


int process_RREQ(int sockfd,int domainfd,char* src_mac,int from_ifindex,t_odrp* odr_packet)
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
	int i=0,update=0;
	time_t ts = time(NULL);
	t_route *entry ;
	t_odrp reply;
	
//	getchar();
	memset(&reply,0,sizeof(reply));

	/*passive entry for back path*/
	if(odr_packet->source_ip==eth0_ip.sin_addr.s_addr)
	{
		dprintf("Cyclic request, not relaying..\n");
		return;
	}

	//printf("src %s dest %s  type->RREQ\n",get_name(odr_packet->source_ip),get_name(odr_packet->dest_ip));
	if(odr_packet->source_ip!=eth0_ip.sin_addr.s_addr)
		update=add_route_entry(odr_packet->source_ip,from_ifindex,src_mac,odr_packet->hop_count+1,ts,
									odr_packet->flag&FORCED_ROUTE);

	if((odr_packet->dest_ip==eth0_ip.sin_addr.s_addr))
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
		if((odr_packet->flag&REP_ALREADY_SENT))
			dprintf("I am Last node, but not sending RREP back as REP_ALREADY_SENT flag is set..\n");
		else{
			dprintf("I am Last node sending RREP back\n");	
			reply.flag = odr_packet->flag&FORCED_ROUTE;
			send_pf_packet(sockfd,i,dest_mac,&reply);
		}
	
	}

	else if((odr_packet->flag&FORCED_ROUTE)||((entry=find_route_entry(odr_packet->dest_ip,ts))==NULL))
	{
		/*broadcast without looking for routing entry*/
		if(entry==NULL)
		{
			dprintf("\nRouting Miss for %s\n",(char*)inet_ntoa(odr_packet->dest_ip) );
		}
		else
			printf("\nForced discovery is active\n");

		memset(dest_mac,0xff,IF_HADDR);
		odr_packet->hop_count++;
		i=0;	

		dprintf("broadcasting RREQ \n");		
		while(i<total_if_count)
		{
			if(if_list[i].if_index==from_ifindex)
			{
				i++;
				continue;
			}
		send_pf_packet(sockfd,i,dest_mac,odr_packet);
			i++;
	
		}
		
	}
	else{
	
		if(!(odr_packet->flag&REP_ALREADY_SENT))
		{		
			reply.type= RREP;
			reply.source_ip = entry->dest_ip;
			reply.dest_ip  = odr_packet->source_ip;
			reply.hop_count = entry->hop_count;
			memcpy(dest_mac,src_mac,IF_HADDR);
			i=0;
			while(i<total_if_count)
			{
				if(from_ifindex==if_list[i].if_index)
					break;
				i++;	
			}	
			assert(i<total_if_count);
			
			dprintf("Routing entry hit, sending unicast RREP\n");	
			send_pf_packet(sockfd,i,dest_mac,&reply);
		}
		else {
			dprintf("Not sending RREP as REP_ALREADY_SENT\n");
		}

	/*	entry->ts=ts;
		printf("\npath reconfirmed %s to %s in %d hops",get_name(reply.source_ip),
							get_name(reply.dest_ip),reply.hop_count);
	*/
		/*broadcast with REP_ALREADY_SENT flag*/
		if(((odr_packet->flag&REP_ALREADY_SENT)&&update)||(!(odr_packet->flag&REP_ALREADY_SENT)))
		{
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
				
			dprintf("Broadcasting RREQ with REP_ALREADY_SENT\n");
			send_pf_packet(sockfd,i,dest_mac,odr_packet);
				i++;	
			}
		}
		else{
			dprintf("Broadcasting REP_ALREADY_SENT's RREQ skipped\n");
		}

	}
		
	return 0;

}
int process_RREP(int sockfd,int domainfd,char *src_mac,int from_ifindex,t_odrp* odr_packet)
{
	/*
		Add routing table entry.
		Add RREP to the queue if cannot be forwarded.
		Check the queue for pending RREP and Data.
	 
	*/
	qnode *node;
	time_t ts = time(NULL);
	int i=0;
	t_route *entry=NULL;
	t_odrp request;
	char dest_mac[6];

	memset(&request,0,sizeof(request));

	if(odr_packet->source_ip!=eth0_ip.sin_addr.s_addr)
		add_route_entry(odr_packet->source_ip,from_ifindex,src_mac,odr_packet->hop_count+1,
					ts,odr_packet->flag&FORCED_ROUTE);

	//printf("src %s dest %s  type->RREP\n",get_name(odr_packet->source_ip),get_name(odr_packet->dest_ip));

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
		dprintf("Replying to prev queued entry\n");	
		send_pf_packet(sockfd,i,src_mac,&node->packet);
		free(node);
	}	
	
	
	if(odr_packet->dest_ip==eth0_ip.sin_addr.s_addr)
	{
		/*Do nothing, we only had initiated*/
		//dprintf("\n Ultimate path from %s to %s in %d hops\n",get_name(eth0_ip.sin_addr.s_addr),
//					get_name(odr_packet->source_ip),odr_packet->hop_count+1);
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

		dprintf("Routing miss, sending RREQ\n");
		i=0;
		while(i<total_if_count)
		{
			if(if_list[i].if_index==from_ifindex)
			{
				i++;
				continue;
			}

			send_pf_packet(sockfd,i,dest_mac,&request);
			i++;
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
		dprintf("\ngot RREP source %s dest %s \n",(char *)inet_ntoa(odr_packet->source_ip),
								(char*)inet_ntoa(odr_packet->dest_ip));
		dprintf("RREP routing hit, relaying RREP for %s\n",(char*)inet_ntoa(entry->dest_ip));	
		send_pf_packet(sockfd,i,dest_mac,odr_packet);
	}	
	return 0;
}

int add_route_entry(unsigned long dest_ip, int if_index, char* neighbour,int hop_count, time_t ts,int force)
{
	t_route *node = head;
	int ret=0;
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

		if(hop_count!=node->hop_count)
			ret=1;
		else ret=0;
		node->hop_count=hop_count;
		node->ts =	ts;

		//printf("\nrouting entry:-> from %s to %s in %d hops\n",get_name(eth0_ip.sin_addr.s_addr),
		//					get_name(node->dest_ip),node->hop_count);
	}
	else{

		/*do nothing, table is upto date*/
	}
			
	return ret;

}

int send_pf_packet(int sockfd,int index_in_if_list, char *dest_mac, t_odrp *odr_packet)
{
	/*target address*/
	struct sockaddr_ll socket_dest_address;
	char* src_mac=NULL;
	int i=0,j=0;
	int if_index;
	/*buffer for ethernet frame*/
	void* buffer = (void*)malloc(ETH_HDRLEN+sizeof(t_odrp));
	 
	/*pointer to ethenet header*/
	unsigned char* etherhead = buffer;
		
	/*userdata in ethernet frame*/
	unsigned char* data = buffer + ETH_HDRLEN;
		
	/*another pointer to ethernet header*/
	struct ethhdr *eh = (struct ethhdr *)etherhead;
	 
	int send_result = 0;
	unsigned char ch;

	assert(index_in_if_list<total_if_count);

	src_mac= if_list[index_in_if_list].if_haddr;	
	if_index= if_list[index_in_if_list].if_index;		
/*
	print ODR details
*/	
	printf("ODR at node %s:",get_name(eth0_ip.sin_addr.s_addr));
	
	printf("Sending frame hdr src: ");
	for(j=0;j<6;j++)
	{
		ch=src_mac[j];
		printf("%.2x:",ch);
	}
		
	printf("  dest: ");

	for(j=0;j<6;j++)
        {
                ch=dest_mac[j];
                printf("%.2x:",ch);
        }

	printf("\n\t ODR msg   type %d   src %s  ",odr_packet->type,get_name(odr_packet->source_ip));
	printf(" dest %s\n",get_name(odr_packet->dest_ip));

	if(odr_packet->type==RREQ)
	dprintf("\nSending RREQ ");
	else if(odr_packet->type==RREP)
	dprintf("\nSending RREP ");
	else if(odr_packet->type==APP_DATA)
		dprintf("\nSending APP_DATA ");
	else dprintf("\nSending undefined ");

	dprintf("packet on PHY index:%d\n",if_index);

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

int process_APP_DATA(int sockfd,int domainfd,char *src_mac,int from_ifindex, t_odrp *odr_packet)
{
	qnode *node;
	time_t ts = time(NULL);
	int i=0,ret=0;
	t_route *entry=NULL;
	rpacket app_packet;
	t_odrp request;
	char dest_mac[6]={};
	struct sockaddr_un cliaddr;
	app_node *app_entry=NULL;

	
	memset(&cliaddr,0,sizeof(cliaddr));
	memset(&request,0,sizeof(request));
	memset(&app_packet,0,sizeof(app_packet));

	//printf("src %s dest %s  type->DATA\n",get_name(odr_packet->source_ip),get_name(odr_packet->dest_ip));
	if(odr_packet->source_ip!=eth0_ip.sin_addr.s_addr)
		add_route_entry(odr_packet->source_ip,from_ifindex,src_mac,odr_packet->hop_count+1,ts,0);

	if(odr_packet->dest_ip==eth0_ip.sin_addr.s_addr)
	{
		/*BINGO!! send data to App.*/
		
		inet_ntop(AF_INET,&odr_packet->source_ip,app_packet.ip,INET_ADDRSTRLEN);

		//printf("\n recieved data from %s to %s in %d hops\n",get_name(odr_packet->source_ip),
		//				get_name(odr_packet->dest_ip),odr_packet->hop_count+1);
		app_packet.src_port = odr_packet->source_port;

		memcpy(app_packet.msg,odr_packet->data,MSG_LEN);
		cliaddr.sun_family = AF_LOCAL;

		if(odr_packet->dest_port==4455)
		{
			dprintf("data for server at port %d \n",odr_packet->dest_port);
			strcpy(cliaddr.sun_path,SERVER_PATH);
		}
		else{
	
			dprintf("data for client at port %d \n",odr_packet->dest_port);
			app_entry = lookup_port(odr_packet->dest_port);
			if(app_entry!=NULL)
			{
				dprintf("data will be sendto %s\n",app_entry->sun_path);
			 	strcpy(cliaddr.sun_path,app_entry->sun_path);
			}
			else{
				printf("Very stale packet.. dropping\n");
				return;	
			}
		}
	
		ret=sendto(domainfd,(char *)&app_packet,sizeof(app_packet),0,(struct sockaddr*)&cliaddr,(socklen_t)sizeof(cliaddr));			
		if(ret<sizeof(app_packet))
		{
			perror("sendto failed:");
			return;
		}
			
		dprintf("data sent successfully\n");	
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

			dprintf("Routing miss for data pack, sending RREQ\n");
			send_pf_packet(sockfd,i,dest_mac,&request);
			i++;
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
		dprintf("\ngot RREP source %s dest %s \n",(char *)inet_ntoa(odr_packet->source_ip),
								(char*)inet_ntoa(odr_packet->dest_ip));
		dprintf("RREP routing hit, relaying RREP for %s\n",(char*)inet_ntoa(entry->dest_ip));	
		send_pf_packet(sockfd,i,dest_mac,odr_packet);
	}	
	return 0;

			

}

int recv_process_pf_packet(int sockfd,int domainfd)
{
	void* recv_buffer = (void*)malloc(ETH_HDRLEN+sizeof(t_odrp)); /*Buffer for ethernet frame*/
	struct sockaddr_ll from;
	int size = sizeof(struct sockaddr_ll);
	t_odrp *odr_packet;
	memset(&from,0,sizeof(from));
	char src_mac[6];
	char dest_mac[6];
	int length = 0; /*length of the received frame*/ 

	length = recvfrom(sockfd, recv_buffer,ETH_HDRLEN+sizeof(t_odrp) , 0, (struct sockaddr*)&from,&size);

	if (length == -1) {
		perror("Recieve failed:");
		return -1;
	}

	odr_packet= (t_odrp*)(recv_buffer+ETH_HDRLEN);
	memcpy(src_mac, (char *) (recv_buffer+IF_HADDR),IF_HADDR);
	memcpy(dest_mac,(char*)recv_buffer,IF_HADDR);

	
	switch(odr_packet->type)
	{
		case RREQ:
			process_RREQ(sockfd,domainfd,src_mac,from.sll_ifindex,odr_packet);
				
		break;
		case RREP:

			dprintf("RREP from %x:%x:%x:%x:%x:%x to me at %x:%x:%x:%x:%x:%x \n",
				src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5],
				dest_mac[0],dest_mac[1],dest_mac[2],dest_mac[3],dest_mac[4],dest_mac[5]);
			process_RREP(sockfd,domainfd,src_mac,from.sll_ifindex,odr_packet);
		break;
		case APP_DATA:
			process_APP_DATA(sockfd,domainfd,src_mac,from.sll_ifindex,odr_packet);
		break;
		default:
			printf("Garbage packet on ODR.. dropping\n");
		break;
	}	


		
//	printf("data recieved: %s",(char*)recv_buffer+14);

	free(recv_buffer);
	return 0;

}
