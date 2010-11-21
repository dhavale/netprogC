#include	"hw_addrs.h"
#include <netinet/in.h>
#include <string.h>

	struct sockaddr_in eth0_ip;	

	struct hw_odr_info if_list[MAX_IF];
	int total_if_count=0;
	int staleness=1000;

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

int
main (int argc, char **argv)
{

	odr_init(); /*Build the interface info table*/


	int maxfdp;
	
	fd_set rset;
	
	int sockfd; /*socketdescriptor*/
	
	
	sockfd = socket(PF_PACKET, SOCK_RAW, htons(2159));
	if (sockfd == -1) { 
		perror("Unable to create socket:");
		exit(EXIT_FAILURE);
	}
	
	
	FD_ZERO(&rset);

	while(1)
	{
		FD_SET(sockfd,&rset);
		maxfdp=sockfd+1;
		
		select(maxfdp,&rset,NULL,NULL,NULL);
		if(FD_ISSET(sockfd,&rset))
		{
			recv_process_pf_packet(sockfd);
		}
	}
	return 0;
}
