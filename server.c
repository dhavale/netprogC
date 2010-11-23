#include <stdio.h>
#include <stdlib.h>
#include "time_lib.h"

struct sockaddr_un cliaddr, servaddr;

int sig_term_handler()
{

}

int main()
{

	char client_ip[16]={};
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

       bzero(&servaddr, sizeof(servaddr)); /* bind an address for us */
       servaddr.sun_family = AF_LOCAL;
       strcpy(servaddr.sun_path, "server.dg");

       bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));


	printf("listening on sockfd %d\n",sockfd);
	while(1)
	{	

	//	gethostbyname();
			memset(msg,0,sizeof(msg));
			memset(client_ip,0,sizeof(client_ip));

			msg_recv(sockfd,msg,client_ip,&source_port);	

			printf("\nRequest from %s port %d\n",client_ip,source_port);		
			ts=time(NULL);
			snprintf(msg,sizeof(msg),"%.24s\r\n",(char *)ctime(&ts));
			msg_send(sockfd,client_ip,source_port,msg,0);
	
			/*start time*/

			/*if timed out, send again*/

			/*if still time out, give up accept next server name*/	
	}

}
