#include	"hw_addrs.h"
#include <netinet/in.h>
#include <string.h>
#include "odr.h"
#include "odr_lib.h"
	struct sockaddr_in eth0_ip;	

	struct hw_odr_info if_list[MAX_IF];
	int total_if_count=0;
	int staleness=0;

void odr_init()
{


	struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
	char   *ptr;
	int    i, prflag;

	printf("\n");

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		
		printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
		if(!strcmp(hwa->if_name,"eth0"))
		{
			eth0_ip= *(struct sockaddr_in*) hwa->ip_addr;
	
		}
		if ( (sa = hwa->ip_addr) != NULL)
			printf("         IP addr = %s\n", (char *)Sock_ntop_host(sa, sizeof(*sa)));
				
		prflag = 0;
		i = 0;
		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {
			printf("         HW addr = ");
			ptr = hwa->if_haddr;
			i = IF_HADDR;
			do {
				printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);
		}

		printf("\n         interface index = %d\n\n", hwa->if_index);
		if(strcmp(hwa->if_name,"eth0")&&strcmp(hwa->if_name,"lo"))
		{
			strcpy(if_list[total_if_count].if_name,hwa->if_name);
			memcpy(if_list[total_if_count].if_haddr,hwa->if_haddr,IF_HADDR);
			if_list[total_if_count].if_index = hwa->if_index;
			if_list[total_if_count].ip_addr = *(struct sockaddr_in *)hwa->ip_addr;
			total_if_count++;
		}
	}

//	memset(&eth0,0,sizeof(struct sockaddr_in));

	printf("\n[INFO]eth0 ip recognized %s \n",(char*)inet_ntoa(eth0_ip.sin_addr));

	free_hwa_info(hwahead);
}


void process_app_req(int sockfd,int domainfd)
{
	spacket packet;
	t_odrp data_odr;
	t_odrp req_packet;
	struct in_addr dest_ip;
	time_t ts = time(NULL);
	t_route *entry=NULL;
	int i=0;
	char dest_mac[IF_HADDR]={};

	struct sockaddr_un src_addr;
	int size = sizeof(src_addr);
	bzero(&packet,sizeof(packet));
	bzero(&data_odr,sizeof(data_odr));
	bzero(&req_packet,sizeof(req_packet));
	dest_ip.s_addr = 0;

	printf("[ODR]: data reading from domainfd\n");

	recvfrom(domainfd,(char*)&packet,sizeof(packet),0,(struct sockaddr*)&src_addr,&size);
	
	if(!inet_aton(packet.ip,&dest_ip))
	{
		perror("Invalid dest: ");
		return;
	}
		
	entry= find_route_entry(dest_ip.s_addr,ts);

		data_odr.type= APP_DATA;
		data_odr.source_ip = eth0_ip.sin_addr.s_addr;
		data_odr.dest_ip = dest_ip.s_addr;
	
		data_odr.dest_port = packet.dest_port;

		if(!strcmp(src_addr.sun_path,"server.dg"))
		{
	
			printf("[ODR]: data recvd from server\n");
			data_odr.source_port = 4455;
		}
		else{
	
			printf("[ODR]: data recvd from client\n");
		 	data_odr.source_port = 9000;
		}
		data_odr.hop_count = 0;
		if(packet.force_flag)
			data_odr.flag|= FORCED_ROUTE;
		memcpy(data_odr.data,packet.msg,40);
	if(entry)
	{
		/*send pf data packet*/
		i=0;
		while(i<total_if_count)
		{
			if(if_list[i].if_index==entry->if_index)
				break;
			i++;
		}
		send_pf_packet(sockfd,i,entry->neighbour,&data_odr);		
			
	}				
	
	else{
		qhead = enqueue(&data_odr);	
		req_packet.type = RREQ;
		req_packet.source_ip = eth0_ip.sin_addr.s_addr;
		req_packet.dest_ip = dest_ip.s_addr;
		req_packet.hop_count =0;
		if(packet.force_flag)
			req_packet.flag|=FORCED_ROUTE;
		memset(dest_mac,0xff,IF_HADDR);
		i=0;
		while(i<total_if_count)
		{
			send_pf_packet(sockfd,i,dest_mac,&req_packet);		
			i++;
		}
	}
	/*route_data_*/		

}

int
main (int argc, char **argv)
{
	int maxfdp;
	
	fd_set rset;
	
	int sockfd; /*socketdescriptor*/

	int domainfd;
	struct sockaddr_un servaddr,cliaddr;

	assert(2==argc);

	assert(1==sscanf(argv[1],"%d",&staleness));

	odr_init(); /*Build the interface info table*/
	unlink(UNIX_D_PATH);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sun_family= AF_LOCAL;
	strcpy(servaddr.sun_path,UNIX_D_PATH);

	domainfd= socket(AF_LOCAL,SOCK_DGRAM,0);

	if (domainfd == -1) { 
		perror("Unable to create socket:");
		exit(EXIT_FAILURE);
	}
	bind(domainfd,(struct sockaddr*)&servaddr,sizeof(servaddr));

	
	
	
	sockfd = socket(PF_PACKET, SOCK_RAW, htons(2159));
	if (sockfd == -1) { 
		perror("Unable to create socket:");
		exit(EXIT_FAILURE);
	}
	
	
	FD_ZERO(&rset);

	while(1)
	{

		FD_ZERO(&rset);
		FD_SET(sockfd,&rset);
		FD_SET(domainfd,&rset);
		if(sockfd>domainfd)
			maxfdp=sockfd+1;
		else
			maxfdp=domainfd+1;

		
		
		select(maxfdp,&rset,NULL,NULL,NULL);
		if(FD_ISSET(sockfd,&rset))
		{
			recv_process_pf_packet(sockfd,domainfd);
		}
		if(FD_ISSET(domainfd,&rset))
		{
			process_app_req(sockfd,domainfd);
		}
	}
	return 0;
}
