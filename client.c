#include <stdio.h>
#include <stdlib.h>

#include "time_lib.h"

#include <sys/un.h>
#include <sys/socket.h>

struct sockaddr_un cliaddr, servaddr;

int sig_term_handler()
{

}

int main()
{

	char server_name[12]={};
	char server_ip[16]={};
	char server_ret_ip[16]={};
/*
	create temporary unix domain socket sun_path.
*/
	int sockfd;
	char msg[MSG_LEN]={};
	int source_port,server_port=4455;
       sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if(sockfd<0)
	{
		perror("unable to create socket:");
		exit(EXIT_FAILURE);
	}
	
	unlink("client.dg");

       bzero(&cliaddr, sizeof(cliaddr)); /* bind an address for us */
       cliaddr.sun_family = AF_LOCAL;
       strcpy(cliaddr.sun_path, "client.dg");

       bind(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));

       bzero(&servaddr, sizeof(servaddr)); /* fill in server's address */
       servaddr.sun_family = AF_LOCAL;
       strcpy(servaddr.sun_path, UNIX_D_PATH);
	int j=0;
	struct hostent *hp;

	while(1)
	{
			
		memset(server_name,0,sizeof(server_name));
		memset(msg,0,sizeof(msg));
		memset(server_ip,0,sizeof(server_ip));

		printf("\nserver: ");
		j=scanf("%s",server_name);
		assert(j);

	//	gethostbyname();
		hp = (struct hostent*)gethostbyname((const char *)server_name);

    		if (hp == NULL) {
   		   	 printf("gethostbyname() failed\n");
		continue;
	    	} else {
      			 printf("%s = ", hp->h_name);
     	  		sprintf(server_ip,"%s",(char*) inet_ntoa( *( struct in_addr*)( hp -> h_addr_list[0])));
			printf("server ip(canonical): %s",server_ip);
		
			msg_send(sockfd,server_ip,server_port,"TIMEREQ",0);
	
			/*start time*/
			msg_recv(sockfd,msg,server_ret_ip,&source_port);	
		
			printf("\nTime recvd: %s from %s \n",msg,server_ret_ip);

			/*if timed out, send again*/

			/*if still time out, give up accept next server name*/	
		}
	}

}