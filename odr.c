#include	"hw_addrs.h"
#include <netinet/in.h>
#include <string.h>

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
		if(strcmp(hwa->if_name,"eth0")||strcmp(hwa->if_name,"lo"))
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



	void* recv_buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
	int s; /*socketdescriptor*/
	
	/*target address*/
	struct sockaddr_ll socket_dest_address;
	
	/*buffer for ethernet frame*/
	void* buffer = (void*)malloc(ETH_FRAME_LEN);
	 
	/*pointer to ethenet header*/
	unsigned char* etherhead = buffer;
		
	/*userdata in ethernet frame*/
	unsigned char* data = buffer + 14;
		
	/*another pointer to ethernet header*/
	struct ethhdr *eh = (struct ethhdr *)etherhead;
	 
	int send_result = 0;
	
	/*our MAC address*/
	unsigned char src_mac[6] = {0x00,0x0c,0x29,0x58,0x83,0x82};
	
	/*other host MAC address*/
	unsigned char dest_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
	s = socket(PF_PACKET, SOCK_RAW, htons(2159));
	if (s == -1) { 
		perror("Unable to create socket:");
		exit(EXIT_FAILURE);
	}
	
	
	/*prepare sockaddr_ll*/
	
	/*RAW communication*/
	socket_dest_address.sll_family   = PF_PACKET;	
	/*we don't use a protocoll above ethernet layer
	  ->just use anything here*/
	socket_dest_address.sll_protocol = htons(ODR_K2159);	
	
	/*index of the network device
	see full code later how to retrieve it*/
	socket_dest_address.sll_ifindex  = 2;
	
	/*address length*/
	socket_dest_address.sll_halen    = ETH_ALEN;		
	/*MAC - begin*/
	socket_dest_address.sll_addr[0]  = 0xff;		
	socket_dest_address.sll_addr[1]  = 0xff;		
	socket_dest_address.sll_addr[2]  = 0xff;
	socket_dest_address.sll_addr[3]  = 0xff;
	socket_dest_address.sll_addr[4]  = 0xff;
	socket_dest_address.sll_addr[5]  = 0xff;
	/*MAC - end*/
	socket_dest_address.sll_addr[6]  = 0x00;/*not used*/
	socket_dest_address.sll_addr[7]  = 0x00;/*not used*/
	
	
	/*set the frame header*/
	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = htons(ODR_K2159);
	/*fill the frame with some data*/
	/*for (j = 0; j < 1500; j++) {
		data[j] = (unsigned char)((int) (255.0*rand()/(RAND_MAX+1.0)));
	}
	*/

	strcpy(data,"Hi from the machine");

	/*send the packet*/
	send_result = sendto(s, buffer, ETH_FRAME_LEN, 0, 
		      (struct sockaddr*)&socket_dest_address, sizeof(socket_dest_address));
	if (send_result == -1) { 
		perror("Send failed..:");
		exit(EXIT_FAILURE);
	}



	int length = 0; /*length of the received frame*/ 

	length = recvfrom(s, recv_buffer, ETH_FRAME_LEN, 0, NULL, NULL);
	if (length == -1) {
		perror("Recieve failed:");
		exit(EXIT_FAILURE);
	}
	
	printf("data recieved: %s",(char *)recv_buffer+14);

	return 0;
}
