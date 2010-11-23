#include "time_lib.h"


int msg_recv(int sockfd,char* msg_recvd,char* ip,int* src_port)
{
	rpacket packet;
	int ret;
	
	memset(&packet,0,sizeof(rpacket));
	printf("in %s waiting for data sockfd %d\n",__FUNCTION__,sockfd);
	ret=recvfrom(sockfd,(char *)&packet, sizeof(rpacket),0,NULL,NULL);
	printf("data read from sockfd in %s\n",__FUNCTION__);	
	if(ret!=sizeof(rpacket))
	{
		printf("data read error from UDS in %s\n",__FUNCTION__);
		return -1;
	}
	memcpy(msg_recvd,packet.msg,MSG_LEN);
	memcpy(ip,packet.ip,IP_LEN);
	*src_port = packet.src_port;

	return 0;	

}

int msg_send(int sockfd, char* ip,int dest_port,char* msg,int flag)
{
	spacket packet;

	struct sockaddr_un servaddr;
	memset(&packet,0,sizeof(packet));
	memset(&servaddr,0,sizeof(servaddr));

	servaddr.sun_family =AF_LOCAL;


	strcpy(servaddr.sun_path,UNIX_D_PATH);
	

	memcpy(packet.ip,ip,IP_LEN);
	packet.dest_port = dest_port;
	strcpy(packet.msg,msg);
	packet.force_flag = flag;

	return sendto(sockfd,(char *)&packet,sizeof(packet),0,(struct sockaddr*)&servaddr,sizeof(servaddr));	

}



