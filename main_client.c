/* Heavenly Medina and Joshua Grabenstein
   CSC 345-01
   Project 4 ----> main_client.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

//used for colors
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define MAG "\x1B[35m"
#define BLUE "\x1B[34m"
#define CYN "\x1B[36m"
#define RESET "\x1B[0m"

#define PORT_NUM 1004

int howManyRooms = 0;



void error(const char *msg)
{
	perror(msg);
	exit(0);
}

typedef struct _ThreadArgs {
	int clisockfd;
} ThreadArgs;

void setRandCol(){
	

	int ran = rand() % 5 + 1;
	if(ran == 1){
		printf(RED);
	}else if(ran == 2){
		printf(GREEN);
	}else if(ran == 3){
		printf(MAG);
	}else if(ran == 4){
		printf(BLUE);
	}else{
		printf(CYN);
	}
}

bool checkValidRoom(int roomNum){ //NEW METHOD: Used to check if user entered room number is valid

if(roomNum<= howManyRooms && roomNum > 0){
	return true;
    }
    return false;

}

int randRoom(int chatRoomNum){
	
	//srand((unsigned) time (NULL));
	int ran; 
	if(howManyRooms == 0){
		ran = 0;
	}else{
		ran = rand() % howManyRooms + 1;
	}
	if(checkValidRoom(ran) == false){//If there are no rooms, ran will be zero, making the check return false, so we create a new room.
		howManyRooms++;
            	//add_room_tail(howManyRooms, 1);
            	chatRoomNum = howManyRooms;
	}else{	
                chatRoomNum = ran;
		//updateUserNum(chatRoomNum);
		}
	return chatRoomNum;
}

bool printBuff(int sockfd){
	int* buffer = (int*) malloc(256);
	int n;
	//printf("Check 3\n");
	n = recv(sockfd, buffer, 256*sizeof(int), 0);//The initial receive will give us our loop parameter
	//printf("Check 4\n");
	int param = buffer[0];
	if(param<=0){
		//printf("param");
		//howManyRooms++;
           	//add_room_tail(howManyRooms, 1);
		return false;
	}else{
	printf("Server says following options are available:\n");
	for(int i = 1; i <= param; i++){
		//printf("%d",i);
		printf("Room %d: %d people(s)\n", i,buffer[i]+1);
	}
	//printf("%s\n", buffer);
	
	printf("Choose the room number or type [new] to create a new room:_\n");
	howManyRooms = param;
	return true;
	}
	
	return false;
}

void passnum(int numtopass, int clisockfd){//Passes number through file.
	/*FILE *pass = fopen("pass.txt", "w");
	fprintf(pass, "%d", numtopass);
	fclose(pass);*/
	int* pass = (int*) malloc(128);
	pass[0] = numtopass;
	int nmsg = 128;
	int nsen = send(clisockfd, pass, nmsg*sizeof(int), 0);
	
}

void newRoom(int sockfd){
	howManyRooms++;
        //add_room_tail(howManyRooms, 1);
	passnum(howManyRooms,sockfd);
}

void chooseRoom(int passNum, int sockfd){
	int n;
	int chatRoomNum = passNum;
	if(checkValidRoom(chatRoomNum) == true){	//join existing room --> need to make sure room exists and that it's open
        passnum(chatRoomNum,sockfd);
	/*char* buff = (char*) malloc(255);
	sprintf(buff, "%d\n",chatRoomNum);
	n = send(sockfd, buff, 255, 0);*/
	}else{
		printf("This input is invalid. Generating random room...");
		chatRoomNum = randRoom(chatRoomNum);
		passnum(chatRoomNum,sockfd);
		/*char* buff = (char*) malloc(255);
		sprintf(buff, "%d\n",chatRoomNum);
		n = send(sockfd, buff, 255, 0);*/
	}
}

void* thread_main_recv(void* args)
{
	pthread_detach(pthread_self());

	int sockfd = ((ThreadArgs*) args)->clisockfd;
	free(args);

	// keep receiving and displaying message from server
	char* buffer = (char*) malloc(512);
	int n;

	n = recv(sockfd, buffer, 512, 0);
	while (n > 0) {
		if (n < 0) error("ERROR recv() failed");
		
		setRandCol();

		printf("\n%s\n", buffer);
		buffer = (char*) malloc(255);
		n = recv(sockfd, buffer, 256, 0);
		
		printf(RESET);
	}

	return NULL;
}

void* thread_main_send(void* args)
{
	pthread_detach(pthread_self());

	int sockfd = ((ThreadArgs*) args)->clisockfd;
	free(args);

	// keep sending messages to the server
	char* buffer = (char*) malloc(255);
	int n;

	while (1) {
		// You will need a bit of control on your terminal
		// console or GUI to have a nice input window.
		printf("\n----------------------------------------------\n");
		printf("Please enter the message: ");
		buffer = (char*) malloc(255);
		fgets(buffer, 255, stdin);

		if (strlen(buffer) == 1) buffer[0] = '\0';

		n = send(sockfd, buffer, strlen(buffer), 0);
		if (n < 0) error("ERROR writing to socket");

		if (n == 0) break; // we stop transmission when user type empty string
	//need to remove/free USR from linked list            
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	int n;
	if (argc < 2) error("Please speicify hostname");
	srand((unsigned) time (NULL));

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");
	//
	//printf("%d\n",atoi(argv[2]));
	//
	struct sockaddr_in serv_addr;
	socklen_t slen = sizeof(serv_addr);
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT_NUM);

	printf("Try connecting to %s...\n", inet_ntoa(serv_addr.sin_addr));
	
	int status = connect(sockfd, (struct sockaddr *) &serv_addr, slen);
	if (status < 0) error("ERROR connecting");
	int chatRoomNum = 0;
        char* news = "new";
	//int tempusrnum = rtail->next->numUsers;
	//printf("Early connection established\n");
	/*printf("Before");
	int* buffer = (int*) malloc(256);
	int n;
	n = recv(sockfd, buffer, 256*sizeof(int), 0);
	howManyRooms = buffer[0];
	newRoom(sockfd);
	printf("After\n");*/
	
	if(argc == 3){
	//printf("Check 1\n");
	int* buffer = (int*) malloc(256);
	int n;
	n = recv(sockfd, buffer, 256*sizeof(int), 0);
	howManyRooms = buffer[0];
	if(strcmp(argv[2], news) == 0){		//create a new room
            	//receiverm(sockfd);
		newRoom(sockfd);
		}else if(argv[2] != NULL) {
			int passnum = atoi(argv[2]);
			//printf("From command line: %d\n", passnum);
			chooseRoom(passnum,sockfd);
			}
		}else if(argc != 3){
			//printf("Check 2\n");
			if(printBuff(sockfd) == false){
				newRoom(sockfd);
				}else{
					char* passVal = (char*)malloc(128);
					fgets(passVal,128,stdin);
					passVal[strcspn(passVal, "\n")] = '\0';
					if(strcmp(passVal,"new") == 0){
						newRoom(sockfd);
					}else{
						int passNum;
						passNum = atoi(&passVal[0]);
						chooseRoom(passNum,sockfd);
						}
				}
		}else{						//join a random room if available 
		chatRoomNum = randRoom(chatRoomNum);
		passnum(chatRoomNum,sockfd);
		 }
	

	printf("Connected to %s with new room number %d\n", inet_ntoa(serv_addr.sin_addr), howManyRooms);

	printf("Enter a username: \n");
	char* usrnmBuff = (char*) malloc(255);
	fgets(usrnmBuff, 255, stdin);
	n = send(sockfd, usrnmBuff, strlen(usrnmBuff), 0);

	pthread_t tid1;
	pthread_t tid2;

	ThreadArgs* args;
	
	args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
	args->clisockfd = sockfd;
	pthread_create(&tid1, NULL, thread_main_send, (void*) args);

	args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
	args->clisockfd = sockfd;
	pthread_create(&tid2, NULL, thread_main_recv, (void*) args);

	// parent will wait for sender to finish (= user stop sending message and disconnect from server)
	pthread_join(tid1, NULL);

	close(sockfd);

	return 0;
}

