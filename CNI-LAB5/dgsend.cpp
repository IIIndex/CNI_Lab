#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<string.h>
 
#define oops(m,x)  { perror(m);exit(x);}
 
int make_dgram_client_socket();
int make_internet_address(char *,int, struct sockaddr_in *);
 
int main(int ac, char *av[]){
	int	sock;
	char *msg;
	struct sockaddr_in saddr;
 
	if (ac != 4){
		fprintf(stderr,"usage: dgsend host port 'message'\n");
		exit(1);
	}
	msg = av[3];

	//请求分配socket
	if((sock = make_dgram_client_socket()) == -1)
		oops("cannot make socket", 2);


	if (make_internet_address(av[1], atoi(av[2]), &saddr) == -1)
		oops("make addr",4);

	//发送失败
	//sendto函数参数表
	//socket id，发送信息指针，发送字符数，设置的发送属性（0表示普通）
	//指向远端socket地址的指针，地址长度
	if (sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
		oops("sendto failed", 3);
	return 0;
}
