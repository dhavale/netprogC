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
	char client_name[12]={};
	char server_ip[16]={};
	char server_ret_ip[16]={};

	int j=0;

	struct hostent *hp;
	
	fd_set rset;
	struct timeval tv;
	tv.tv_sec = 6;
	tv.tv_usec =0;

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
	
	gethostname(client_name,sizeof(client_name));

       bzero(&cliaddr, sizeof(cliaddr)); /* bind an address for us */
       cliaddr.sun_family = AF_LOCAL;

	strcpy(cliaddr.sun_path,"/tmp/fileXXXXXX");
       if(mkstemp(cliaddr.sun_path)<0)
	{
		perror("\nSunpath cannot be created");
		return -1;
	}
	else{

		printf("going to use sun_path %s\n",cliaddr.sun_path);
		unlink(cliaddr.sun_path);
	}
	j=0;
      if( bind(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr))<0)
	{
		perror("unable to bind:");
		return -1;
	}

       bzero(&servaddr, sizeof(servaddr)); /* fill in server's address */
       servaddr.sun_family = AF_LOCAL;
       strcpy(servaddr.sun_path, UNIX_D_PATH);
	while(1)
	{
		tv.tv_sec =6;
		tv.tv_usec =0;		
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
			FD_ZERO(&rset);
			FD_SET(sockfd,&rset);

			select(sockfd+1,&rset,NULL,NULL,&tv);
			if(FD_ISSET(sockfd,&rset))	
			{		
				msg_recv(sockfd,msg,server_ret_ip,&source_port);	
		
				printf("\nclient at node %s: received from %s %s\n",client_name,server_name,msg);
			}
			else {
				printf("client at node %s: timeout on response from %s\n",client_name,server_name);
				printf("Initiating forced rediscovery\n");
			
				msg_send(sockfd,server_ip,server_port,"TIMEREQ",1);
	
				/*start time*/
				FD_ZERO(&rset);
				FD_SET(sockfd,&rset);
				tv.tv_sec = 6;
				tv.tv_usec =0;
	
				select(sockfd+1,&rset,NULL,NULL,&tv);
				if(FD_ISSET(sockfd,&rset))	
				{		
					msg_recv(sockfd,msg,server_ret_ip,&source_port);	
		
					printf("\nclient at node %s: received from %s %s\n",client_name,server_name,msg);
				}
				else {
					printf("client at node %s Timed out again.. Try another server..\n",client_name);
				}

			}

		}
	}

}
