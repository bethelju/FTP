/*
Name:Justin Bethel
Program Name: FTPSERVER
Program Description: The C implementation of the ftp server
Course Number: CS372
Last Modified: 12/2/2019
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <dirent.h>

#include <arpa/inet.h>

#define MAXDATASIZE 100 

//Function that takes in a pointer to a socket address struct and returns a pointer to the 
//struct with the IP address block set
void *get_in_addr(struct sockaddr *sa){
	if(sa->sa_family = AF_INET){
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//Function that takes in a pointer to an array of char pointers
//and populates them with the names of the file in the cwd
//returns the number of files found
//Used https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/ as inspiration
int getFileNames(char ** fileNames){
	struct dirent *de;

	DIR *dr = opendir(".");
	
	int i = 0;
	while((de = readdir(dr)) != NULL)
		fileNames[i++] = de->d_name;
	closedir(dr);
	return i;
}

//Function that takes in an address and port and returns a pointer
//to a population addrinfo struct
struct addrinfo * getAddrInfo(char* port, char* address){
	int sockfd, rv;
	struct addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof hints);
	//Set specifications for a TCP stream socket
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
		
	//Populate servinfo struct using the port and address
	if((rv = getaddrinfo(address, port, &hints, &servinfo)) != 0){
		printf("Error getting address\n");
		exit(1);
	}

	return servinfo;
}

//Function that takes a pointer to a populated servInfo struct and 
//sets up the socket. Returns the socket descriptor
int setupSocket(struct addrinfo * servinfo){
	int sockfd;
	//Set up socket
	if((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, 
		servinfo->ai_protocol))==-1){
			perror("socket");
			return -1;
	}
	//return the socket descriptor
	return sockfd;
}

//Bind socket using socket descriptor and populated addrinfo struct
//Return -1 if unsuccessful but 1 if successful
int bindSocket(int sockfd, struct addrinfo * servinfo){
	int yes = 1;
	//Set socket options (used Beej's guide) so that we can reuse ports easily
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1){
		perror("sockopt");
		return -1;
	}
	//Bind the socket descriptor with the set address
	if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
		close(sockfd);
		perror("Binding error, you most likely chose a port number under 1025");
		return -1;
	}
	//if servInfo is not populated, there's been a binding error
	if(servinfo == NULL){
		printf("Binding error!");
		return -1;
	}
	return 1;
}

//Main Function
int main(int argc, char *argv[]){
	//Initialize main variables for use later
	int sockfd, new_fd;
	struct addrinfo *servinfo;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];
	char* port = argv[1];

	//Validate the ports and length of ports
	if(argc != 2 || strlen(argv[1]) < 3 || strlen(argv[1]) > 4){
		printf("There should be just a single numerical port number\n");
		printf("It should also be between 100 - 9999\n");
		exit(1);
	}

	//populate servinfo struct with port and our IP address
	servinfo = getAddrInfo(port, NULL);
	//Setup socket
	sockfd = setupSocket(servinfo);
	
	//Bind the socket, return if unsuccessful
	if(bindSocket(sockfd, servinfo)  == -1){
		return;
	}
	
	//Listen on the socket, exiting if unsuccessful
	if(listen(sockfd, 10) == -1){
		perror("listen");
		return;
	}

	//Free the address struct
	freeaddrinfo(servinfo);
		
	printf("Server is waiting for connections\n");

	//Repeat until SIGINT is sent by user
	while(1){
		//Initialize loop variables
		int numbytes;
		char buf[MAXDATASIZE];
		char dataPort[7];
		memset(dataPort, '\0', sizeof dataPort);	

		//accept connection from client	
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		//If acceptance not successful, output message and repeat loop
		if(new_fd == -1){
			perror("accept");
			continue;
		}

		//extract the client address
		//https://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c
		inet_ntop(their_addr.ss_family, 
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		char userName[MAXDATASIZE];
		char password[MAXDATASIZE];
		memset(userName, '\0', sizeof(userName));
		memset(password, '\0', sizeof(userName));
		recv(new_fd, userName, MAXDATASIZE-1, 0);
		sleep(1);
		recv(new_fd, password, MAXDATASIZE-1, 0);
		if(strcmp(userName, "bethelju") || strcmp(password, "passw")){
			send(new_fd, "No match", 8, 0);	
			continue;
			printf("NO!");
			fflush(stdout);
		}
		send(new_fd, "Match", 5, 0);
		
		//Take in command from client and set newline char to null
		if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1){
			perror("recv");
			exit(1);
		}
		buf[numbytes] = '\0';
		
		//See if the command is valid, send back error message if not
		if(strncmp(buf, "-l", 2) && strncmp(buf, "-g", 2)){
			send(new_fd, "There is no command like that", 29, 0);
		}
		//If the command is valid
		else{
			send(new_fd, "Command recognized", 18, 0);
			//Addded to make sure the client in sync
			sleep(1);

			//Take in command fields and set them accordingly
			char *fileName;
			char* token = strtok(buf, " ");
			if(!strcmp(token, "-g")){
				token = strtok(NULL, " ");
				fileName = token;
			}
			token = strtok(NULL, " ");
			strcpy(dataPort, token);

			//Create socket to connect to client using similar method as above
			servinfo = getAddrInfo(dataPort, s);
			int newsockfd = setupSocket(servinfo);
			if(connect(newsockfd, servinfo->ai_addr, servinfo->ai_addrlen)
				== -1){
				close(newsockfd);
				perror("data port connect");
				exit(1);
			}
			//If command is list
			if(!strncmp(buf, "-l", 2)){
				//Send all file names
				char * fileNames[30];
				int numberFiles = getFileNames(fileNames);
				int i = 0;
				for(i = 0; i < numberFiles; i++){
					send(newsockfd, fileNames[i], strlen(fileNames[i]), 0);
					//Sleep added to make sure the send commands don't mix at destination
					sleep(1);
				}
				//Send end of input command
				send(newsockfd, "endOfInput", 10, 0);
			}
			//If command is to send file
			else{
				//Open the file in the cwd
				FILE *fptr = NULL;
				fptr = fopen(fileName, "r");
				//If no such file, return message to the client
				if(fptr == NULL){
					send(new_fd, "NO FILE WITH THAT NAME", 22, 0);
				}
				//If there is a corresponding file, send success message
				else{
					send(new_fd, "FILE CORRECT", 12, 0);
					//Using riptutorial.com/c/example/8274/get-lines-from
					//a-file-using-getline as model
					char readBuf[1024];
					memset(readBuf, '\0', sizeof(readBuf));
					//While the end of the file hasn't been reached, keep reading
					while(!feof(fptr)){
						fread(readBuf, 1, sizeof(readBuf)-1, fptr);	
						
						send(newsockfd, readBuf, sizeof(readBuf)-1, 0);
						//Set character array to null char
						memset(readBuf, '\0', sizeof(readBuf));
					}
					//Send end of file command, close file and data socket
					sleep(2);
					send(newsockfd, "endOfInput", 10, 0);
					fclose(fptr);
					close(newsockfd);
				}
			}
		//Close the control socket
		close(new_fd);
		}
	}
}
		
