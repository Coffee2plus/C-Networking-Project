/* Heavenly Medina and Joshua Grabenstein
   CSC 345-01
   Project 4 ----> main_server.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h> //used for srand
#include <stdbool.h>

#define PORT_NUM 1004

int howManyRooms = 0;

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

typedef struct _USR {
	int clisockfd;		// socket file descriptor
	struct _USR* next;	// for linked list queue
	char* username;		// keeps track of username that user specifies upon joining server
	int roomNum;		// keeps track of users room number
} USR;

USR *head = NULL;
USR *tail = NULL;

/*ROOM *rhead = NULL;
ROOM *rtail = NULL;*/

void clientList(){
	USR* temp = head;
	
	while(temp != NULL){
		printf("username: %s \n", temp->username);//Print statement will change once we start adding ip addresses and usernames
		temp = temp->next;
	}
}

void clientRemove(int clisockfd){ //NEW METHOD: Used to remove client from the client list

	USR* temp = head;
	USR* past = temp;
	
	if(temp->clisockfd == clisockfd){ //Checks to see if head client is the same client id inputted to method.
		head = head->next;
	}else{//If not, go through until it is found.
		while(temp->clisockfd != clisockfd){
			past = temp;
			temp = temp->next;
			}
		past->next = temp->next;
		if(temp == tail){		//Also, check to see if head is equal to the tail.
			tail = past;
		}
	}
	clientList();
}



void add_tail(int newclisockfd, char* usrNme, int chatRoomNum)
{
	if (head == NULL) {
		head = (USR*) malloc(sizeof(USR));
		head->clisockfd = newclisockfd;
		head->username = usrNme;
        head->roomNum = chatRoomNum;
		head->next = NULL;
		tail = head;
	} else {
		tail->next = (USR*) malloc(sizeof(USR));
		tail->next->clisockfd = newclisockfd;
		tail->next->username = usrNme;
        tail->next->roomNum = chatRoomNum;
		tail->next->next = NULL;
		tail = tail->next;
	}
	clientList();
}


void updateClient(int clisockfd)
{
	int n;
	USR* temp;
	int* buffer = (int*) malloc(256);
	buffer[0] = howManyRooms;
	
	for(int i = 1; i <= howManyRooms; i++){
		temp = head;		
		n = 0;
		while(temp != NULL){
			if(i == temp->roomNum){
				n++;
			}
		temp = temp->next;
		}
		buffer[i] = n-1; 
		}
		int nmsg = 256*sizeof(int);
		int nsen = send(clisockfd, buffer, nmsg, 0);
}

bool checkValidRoom(int roomNum){ //NEW METHOD: Used to check if user entered room number is valid
if(roomNum<= howManyRooms && roomNum > 0){
	return true;
    }
    return false;

}

int updateServer(int sockfd)
{
	int n;
	int chatrooms;
	int userNum;
	int* buffer = (int*) malloc(128);
	n = recv(sockfd, buffer, 128*sizeof(int), 0);
	chatrooms = buffer[0];
	printf("%d", chatrooms);
	if(checkValidRoom(chatrooms) == true){
		return chatrooms;
	}else{
		howManyRooms++;
		//printf("Successfully created new room! # of rooms: %d", howManyRooms);
		return howManyRooms;
	}
	return chatrooms;
}
void broadcast(int fromfd, char* message)
{
	int key = 0;
	// figure out sender address
	struct sockaddr_in cliaddr;
	socklen_t clen = sizeof(cliaddr);
	if (getpeername(fromfd, (struct sockaddr*)&cliaddr, &clen) < 0) error("ERROR Unknown sender!");

	// traverse through all connected clients
	USR* cur = head;
	USR* temp = head;
	USR* sender = head;	
	int rmNum;
	while(temp != NULL){
		if(temp->clisockfd == fromfd){
			sender = temp;
			rmNum = temp->roomNum;	
		}
		temp = temp->next;
	}
	
	while (cur != NULL) {
		// check if cur is not the one who sent the message
		if ((cur->clisockfd != fromfd) && (cur->roomNum == rmNum)) {
			char* buffer = (char*) malloc(512);

			// prepare message
			sprintf(buffer, "[%s]:%s", sender->username, message);
			int nmsg = strlen(buffer);

			// send!
			int nsen = send(cur->clisockfd, buffer, nmsg, 0);
			if (nsen != nmsg) error("ERROR send() failed");
		}

		cur = cur->next;
	}
}

typedef struct _ThreadArgs {
	int clisockfd;
} ThreadArgs;

void* thread_main(void* args)
{
	// make sure thread resources are deallocated upon return
	pthread_detach(pthread_self());

	// get socket descriptor from argument
	int clisockfd = ((ThreadArgs*) args)->clisockfd;
	free(args);

	//-------------------------------
	// Now, we receive/send messages
	char* buffer = (char*) malloc(255);
	int nsen, nrcv;

	nrcv = recv(clisockfd, buffer, 255, 0);
	if (nrcv < 0) error("ERROR recv() failed");

	while (nrcv > 0) {
		// we send the message to everyone except the sender
		broadcast(clisockfd, buffer);
		buffer = (char*) malloc(255);
		nrcv = recv(clisockfd, buffer, 255, 0);
		if (nrcv < 0) error("ERROR recv() failed");
	}
	clientRemove(clisockfd);
	close(clisockfd);
	//-------------------------------

	return NULL;
}

int main(int argc, char *argv[])
{
	int openRooms = 0;
	int n;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");

	struct sockaddr_in serv_addr;
	socklen_t slen = sizeof(serv_addr);
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;	
	//serv_addr.sin_addr.s_addr = inet_addr("192.168.1.171");	
	serv_addr.sin_port = htons(PORT_NUM);

	int status = bind(sockfd, 
			(struct sockaddr*) &serv_addr, slen);
	if (status < 0) error("ERROR on binding");

	listen(sockfd, 5); // maximum number of connections = 5
	

	while(1) {
		
		struct sockaddr_in cli_addr;
		socklen_t clen = sizeof(cli_addr);
		int newsockfd = accept(sockfd, 
			(struct sockaddr *) &cli_addr, &clen);
		if (newsockfd < 0) error("ERROR on accept");
		
		updateClient(newsockfd);
		int chatRoomNum = updateServer(newsockfd);
		

		char* buffer = (char*) malloc(255);//Code for adding username!
		n = recv(newsockfd, buffer, 255,0);
		for(int i = 0; i < 256; i++){
		if(buffer[i] == '\n'){
			buffer[i] = '\0';	
			//clientRemove()??
			break;			
			}
		}
		
		printf("%s has connected to: %s, with room number %d!\n",buffer,inet_ntoa(cli_addr.sin_addr), chatRoomNum);		
		
		//printf("Connected: %s\n ", inet_ntoa(cli_addr.sin_addr));
			
		add_tail(newsockfd, buffer, chatRoomNum); // add this new client to the client list

		// prepare ThreadArgs structure to pass client socket
		ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
		if (args == NULL) error("ERROR creating thread argument");
		
		args->clisockfd = newsockfd;

		pthread_t tid;
		if (pthread_create(&tid, NULL, thread_main, (void*) args) != 0) error("ERROR creating a new thread");
	}

	return 0; 
}

