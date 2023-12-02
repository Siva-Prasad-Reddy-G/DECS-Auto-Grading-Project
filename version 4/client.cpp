#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <error.h>
#include <stdlib.h>
#include <stdbool.h>
#include<string>
#include<iostream>
#include "utility.h"

using namespace std;


int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        perror("Usage: ./submit  <serverIP> <port> <typeOfRequest(0/1)> <sourceCodeFileTobeGraded|requestID>\n");
        return -1;
    }
    
    int typeOfRequest=atoi(argv[3]);
    char *requestId,*file_path;
    
    char server_ip[40], ip_port[40];
    int server_port;
   
    strcpy(server_ip, argv[1]);
    server_port = atoi(argv[2]);

  
    
	//create the socket file descriptor
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Socket creation failed");
        return -1;
    }

	// setup the server side variables
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr.s_addr);

    int tries = 0;
    while (true)
    {
    	  //connect to the server using the socket fd created earlier
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0)
            break;
        sleep(1);
        tries += 1;
        if (tries == 5)
        {
            printf ("Server not responding\n");
            return -1;
        }
    }
    
    
	if(typeOfRequest==0)
	{
		file_path=argv[4];
    	send(sockfd,"0",sizeof(int),0);
    	//send the file by calling the send file utility function
    	cout<<file_path<<endl;
    	if (send_file(sockfd, file_path) != 0)
    	{
        	printf ("Error sending source file\n");
        	close(sockfd);
        	return -1;
    	};
    	printf ("Code sent for grading\n");
    	char req_id[100];
    	recv(sockfd,req_id,100,0);
    	char message[1000];
    	recv(sockfd,message,1000,0);
    	printf("%s\n",message);
    	cout<<"started with reqID"<<req_id<<endl;
    	while(1)
    	{
    		
    		send(sockfd,req_id,255,0);
	
			int size_int=sizeof(int);
			char receive_type[size_int];
    		if (recv(sockfd, receive_type, sizeof(receive_type), 0) == -1)
    		{
        		perror("Error receiving position.");
        		return -1;
    		}
    		int type=atoi(receive_type);
    		if(type==3)
    		{
    			char pos_recv[size_int];
    			if (recv(sockfd, pos_recv, sizeof(pos_recv), 0) == -1)
    			{
        			perror("Error receiving position.");
        			return -1;
    			}
    			int position=atoi(pos_recv);
    		
    			printf("Server Response: Request is being processed in  queue , at position: %d\n",position);

    		}
    		else if(type==0)
    		{
    			char pos_recv[size_int];
    			if (recv(sockfd, pos_recv, sizeof(pos_recv), 0) == -1)
    			{
        			perror("Error receiving position.");
        			return -1;
    			}
    			int position=atoi(pos_recv);
    		
    			printf("Server Response: Request is in the queue , at position: %d\n",position);
    		}
    		else if(type==1)
    		{
    			printf("Request not found!\n");	    		}
    		else
    		{
    			string reqId(requestId);
    			string fileName="./received_Response/Recv_"+reqId+".txt";
    			if(recv_file(sockfd,fileName))
    			{
    				perror("Error receiving the file.");
    				return 1;
    			}
    		
    			string cat_command="cat "+fileName;
    			system(cat_command.c_str());
    			break;
    		}	

    	}
	}
	else
	{  
		requestId=argv[4]; 
		send(sockfd,"1",sizeof(int),0);
		send(sockfd,requestId,255,0);
	
		int size_int=sizeof(int);
		char receive_type[size_int];
    	if (recv(sockfd, receive_type, sizeof(receive_type), 0) == -1)
    	{
        	perror("Error receiving position.");
        	return -1;
    	}
    	int type=atoi(receive_type);
        if(type==3)
            {
                char pos_recv[size_int];
                if (recv(sockfd, pos_recv, sizeof(pos_recv), 0) == -1)
                {
                    perror("Error receiving position.");
                    return -1;
                }
                int position=atoi(pos_recv);
            
                printf("Server Response: Request is being processed in  queue , at position: %d\n",position);

            }
    	if(type==0){
    		char pos_recv[size_int];
    		if (recv(sockfd, pos_recv, sizeof(pos_recv), 0) == -1)
    		{
        		perror("Error receiving position.");
        		return -1;
    		}
    		int position=atoi(pos_recv);
    		
    		printf("Server Response: Request is in the queue , at position: %d\n",position);
    	}
    	else if(type==1){
    		printf("Request not found!\n");	
    	}
    	else{
    		string reqId(requestId);
    		string fileName="./received_Response/Recv_"+reqId+".txt";
    		if(recv_file(sockfd,fileName)){
    			perror("Error receiving the file.");
    			return 1;
    		}
    		
    		string cat_command="cat "+fileName;
    		system(cat_command.c_str());
    	}
	
	}	

    close(sockfd);

    return 0;
}

