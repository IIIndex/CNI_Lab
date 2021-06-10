#include<stdio.h>
#include<stdlib.h>
void do_regular_work();
 
int main(int ac, char *av[]){
	setup();
	if (get_ticket() != 0 )
		exit(0);	
 
	do_regular_work();
 
	release_ticket();
	shut_down();
 
}

void do_regular_work(){
	printf("SuperSleep version 1.0 Running - Licensed Software\n");
	sleep(15);

	if (validate_ticket() != 0){
		printf("Server errors. Please Try later.\n");
		return;
	}
    
	sleep(15);
}
