#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdlib.h>

#define oops(m,x)  {perror(m);exit(x);}

int  make_dgram_server_socket(int);
int  get_internet_address(char *, int,  int *, struct sockaddr_in *);
void say_who_called(struct sockaddr_in *);

int main(int ac, char *av[]){
	int	port;
	int	sock;
	char buf[BUFSIZ];
	size_t msglen;
	struct sockaddr_in saddr;
	socklen_t saddrlen;

	//未输入端口号
	if (ac == 1 || (port = atoi(av[1])) <= 0){
		fprintf(stderr,"usage: dgrecv portnumber\n");
		exit(1);
	}

	//获得端口号失败
	if((sock = make_dgram_server_socket(port)) == -1)
		oops("cannot make socket",2);

	saddrlen = sizeof(saddr);
	//获取信息并打印地址
	//recvfrom函数参数表
	//socket id，接受信息指针，接受字符数，设置的接受属性（0表示普通）
	//指向远端socket地址的指针，地址长度
	while((msglen = recvfrom(sock, buf, BUFSIZ, 0, (struct sockaddr *)&saddr, &saddrlen)) > 0) {
		buf[msglen] = '\0';
		printf("dgrecv: got a message: %s\n", buf);
		say_who_called(&saddr);
	}
	return 0;
}

void say_who_called(struct sockaddr_in *addrp){
	char host[BUFSIZ];
	int	port;
	get_internet_address(host,BUFSIZ,&port,addrp);
	printf("  from: %s:%d\n", host, port);
}