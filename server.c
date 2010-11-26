#include <stdio.h>
#include <stdlib.h>
#include "time_lib.h"

#ifdef ODR_DEBUG
#define dprintf(fmt, args...) printf(fmt, ##args)
#else
#define dprintf(fmt, args...)
#endif
struct sockaddr_un cliaddr, servaddr;

int sig_term_handler()
{

}

int main()
{

	char client_ip[16]={};
	char client_name[12]={};
	char server_name[12]={};
	struct in_addr ipv4addr;
/*
	create temporary unix domain socket sun_path.
*/
	int sockfd;
	char msg[MSG_LEN]={};
	time_t ts;
	int source_port;
       sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if(sockfd<0)
	{
		perror("unable to create socket:");
		exit(EXIT_FAILURE);
	}

	unlink("server.dg");

	gethostname(server_name,sizeof(server_name));

       bzero(&servaddr, sizeof(servaddr)); /* bind an address for us */
       servaddr.sun_family = AF_LOCAL;
       strcpy(servaddr.sun_path, "server.dg");

       if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)
	{
		perror("Bind failed:");
		return -1;	
	}

	struct hostent *hp;
	dprintf("listening on sockfd %d\n",sockfd);
	while(1)
	{	

	//	gethostbyname();
			memset(msg,0,sizeof(msg));
			memset(client_ip,0,sizeof(client_ip));
			memset(&ipv4addr,0,sizeof(ipv4addr));
			msg_recv(sockfd,msg,client_ip,&source_port);	

			dprintf("\nRequest from %s port %d\n",client_ip,source_port);

			inet_pton(AF_INET, client_ip, &ipv4addr);
			hp= gethostbyaddr(&ipv4addr,sizeof(ipv4addr),AF_INET);
			if(hp==NULL)
			{
				printf("gethostbyaddr failed..\n");
				continue;
			}
			else {

				printf("server at node %s responding to request from %s\n",server_name,hp->h_name);
			}
			ts=time(NULL);
			snprintf(msg,sizeof(msg),"%.24s\r\n",(char *)ctime(&ts));
			msg_send(sockfd,client_ip,source_port,msg,0);
	
			/*start time*/

			/*if timed out, send again*/

			/*if still time out, give up accept next server name*/	
	}

}
