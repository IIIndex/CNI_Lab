
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>

static int pid = -1;
static int sd = -1;
static struct sockaddr serv_addr;
static socklen_t serv_alen;
static char ticket_buf[128];
static have_ticket = 0;

#define MSGLEN 128
#define SERVER_PORTNUM 2020
#define	HOSTLEN 512
#define oops(p) {perror(p); exit(1) ; }

char *do_transaction(char *msg);

//Debug函数
void narrate(char *msg1, char *msg2){
	fprintf(stderr,"CLIENT [%d]: %s %s\n", pid, msg1, msg2);
}

//Debug函数
void syserr(char *msg1){
	char	buf[MSGLEN];
	sprintf(buf,"CLIENT [%d]: %s", pid, msg1);
	perror(buf);
}

//得到pid，socket和许可证服务器地址
void setup(){
	char hostname[BUFSIZ];
	pid = getpid();
	sd  = make_dgram_client_socket();
	if(sd == -1) 
		oops("Cannot create socket");
	gethostname(hostname, HOSTLEN);
	make_internet_address(hostname, SERVER_PORTNUM, &serv_addr);
	serv_alen = sizeof(serv_addr);
}
 
void shut_down(){
	close(sd);
}

//从许可证服务器得到许可证，返回0成功，-1失败
int get_ticket(){
	char *response;
	char buf[MSGLEN];
 
	if(have_ticket)
		return(0);
 
	sprintf(buf, "HELO %d", pid);

    //未连接到服务器
	if((response = do_transaction(buf)) == NULL)
		return(-1);

    //正确地得到了许可证
	if (strncmp(response, "TICK", 4) == 0){
		strcpy(ticket_buf, response + 5);
		have_ticket = 1;
		narrate("got ticket", ticket_buf);
		return 0;
	}

    //得到服务器的回应，获取失败
	if (strncmp(response,"FAIL",4) == 0)
		narrate("Could not get ticket",response);
	else
		narrate("Unknown message:", response);

	return -1;
}
  
//释放资源（对许可证的占用），返回0成功，-1失败
int release_ticket(){
	char buf[MSGLEN];
	char *response;

    //无资源无需释放
	if(!have_ticket)
		return 0;

	sprintf(buf, "GBYE %s", ticket_buf);
    //未得到服务器回应
	if ((response = do_transaction(buf)) == NULL)
		return -1;
 
	//得到服务器的回应，释放成功
	if (strncmp(response, "THNX", 4) == 0){
		narrate("released ticket OK", "");
		return 0;
	}
    //得到服务器的回应，释放失败
	if (strncmp(response, "FAIL", 4) == 0)
		narrate("release failed", response+5);
	else
		narrate("Unknown message:", response);
	return(-1);
}
 
//向服务器传递信息
char *do_transaction(char *msg){
	static char buf[MSGLEN];
	struct sockaddr retaddr;
	socklen_t addrlen;
	int ret;
	
	ret = sendto(sd, msg, strlen(msg), 0, &serv_addr, serv_alen);
	if (ret == -1){
		syserr("sendto");
		return NULL;
	}

	ret = recvfrom(sd, buf, MSGLEN, 0, &retaddr, &addrlen);
	if(ret == -1){
		syserr("recvfrom");
		return NULL;
	}

	return buf;
}

//向服务器询问许可证是否合法
int validate_ticket(){
	char *response;
	char buf[MSGLEN];

	if(!have_ticket)
		return 0;

	sprintf(buf, "VALD %s", ticket_buf);

	if ((response = do_transaction(buf)) == NULL)
		return -1;

	narrate("Validated ticket: ", response);

	if(strncmp(response, "GOOD", 4) == 0)
		return 0;

	if(strncmp(response,"FAIL",4) == 0){
		have_ticket = 0;
		return -1;
	}
    
	narrate("Unknown message:", response);
	return -1;
}