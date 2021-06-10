#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<signal.h>
#include<sys/errno.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>

#define SERVER_PORTNUM 2020
#define MSGLEN 128
#define TICKET_AVAIL 0
#define MAXUSERS 3
#define	oops(x)	{perror(x); exit(-1); }

int ticket_array[MAXUSERS];
int sd = -1;
int num_tickets_out = 0;
 
char *do_hello();
char *do_goodbye();
void narrate(char *msg1, char *msg2, struct sockaddr_in *clientp);
void free_all_tickets();

#define RECLAIM_INTERVAL 60

//检查用户是否在线
void ticket_reclaim(){
	int	i;
	char tick[BUFSIZ];
 
	for(i = 0; i < MAXUSERS; i++) {
		if((ticket_array[i] != TICKET_AVAIL) &&
		   (kill(ticket_array[i], 0) == -1) && (errno == ESRCH)) {
			/* Process is gone - free up slot */
			sprintf(tick, "%d.%d", ticket_array[i],i);
			narrate("freeing", tick, NULL);
			ticket_array[i] = TICKET_AVAIL;
			num_tickets_out--;
		}
	}
	alarm(RECLAIM_INTERVAL);
} 
 
//初始化服务器
int setup(){
	sd = make_dgram_server_socket(SERVER_PORTNUM);
	if (sd == -1)
		oops("make socket");
	free_all_tickets();
	return sd;
}

//释放所有许可证连接
void free_all_tickets(){
	for(int i=0; i < MAXUSERS; i++)
		ticket_array[i] = TICKET_AVAIL;
}

void shut_down(){
	close(sd);
}

//分发许可证（如果还有剩余）
char *do_hello(char *msg_p){

	static char replybuf[MSGLEN];
 
	if(num_tickets_out >= MAXUSERS) 
		return("FAIL no tickets available");

    int x;
	for(x = 0; x < MAXUSERS && ticket_array[x] != TICKET_AVAIL; x++);

	if(x == MAXUSERS){
		narrate("database corrupt","",NULL);
		return("FAIL database corrupt");
	}

	ticket_array[x] = atoi(msg_p + 5);
	sprintf(replybuf, "TICK %d.%d", ticket_array[x], x);
	num_tickets_out++;
	return(replybuf);
}

//回收正在使用的许可证
char *do_goodbye(char *msg_p){

	int pid, slot;

	if((sscanf((msg_p + 5), "%d.%d", &pid, &slot) != 2) || (ticket_array[slot] != pid)){
		narrate("Bogus ticket", msg_p+5, NULL);
		return("FAIL invalid ticket");
	}
 
	//释放许可证连接
	ticket_array[slot] = TICKET_AVAIL;
	num_tickets_out--;
 
	//回复消息，以便客户端验证
	return("THNX See ya!");
}

//Debug/log文件
void narrate(char *msg1, char *msg2, struct sockaddr_in *clientp){
	fprintf(stderr,"\t\tSERVER: %s %s ", msg1, msg2);
	if(clientp)
		fprintf(stderr,"(%s : %d)", inet_ntoa(clientp->sin_addr), ntohs(clientp->sin_port));
	putc('\n', stderr);
}

//合法性检查
static char *do_validate(char *msg){

	int pid, slot;
	 
	if (sscanf(msg+5,"%d.%d",&pid,&slot) == 2 && ticket_array[slot] == pid) 
		return("GOOD Valid ticket");

	narrate("Bogus ticket", msg+5, NULL);
	return("FAIL invalid ticket");
}


void handle_request(char *req,struct sockaddr *client, socklen_t addlen){
	char	*response;
	int	ret;
 
	//根据请求选择相应的相应方式
	if(strncmp(req, "HELO", 4) == 0)
		response = do_hello(req);
	else if(strncmp(req, "GBYE", 4) == 0)
		response = do_goodbye(req);
	else if(strncmp(req, "VALD", 4) == 0)
		response = do_validate(req);
	else
		response = "FAIL invalid request";
 
	narrate("SAID:", response, client);
	ret = sendto(sd, response, strlen(response),0, client, addlen);
	if (ret == -1)
		perror("SERVER sendto failed");
}