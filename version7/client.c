#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <error.h>
#include <stdlib.h>
#include <stdbool.h>
#include<sys/time.h>
#include<errno.h>

const int BUFFER_SIZE = 1024; 
const int MAX_FILE_SIZE_BYTES = 4;
const int MAX_TRIES = 5;

double GetTime() {
    struct timeval t;
    int rc = gettimeofday(&t, NULL);
    return (double) t.tv_sec + (double) t.tv_usec/1e6;
}

int send_file(int sockfd, char* file_path)
{
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE); 
    FILE *file = fopen(file_path, "rb"); 
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }
		
    fseek(file, 0L, SEEK_END); 
    int file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    memcpy(file_size_bytes, &file_size, sizeof(file_size));

    if (send(sockfd, file_size_bytes,MAX_FILE_SIZE_BYTES,0)  == -1)
    {
        perror("Error sending file size");
        fclose(file);
        return -1;
    }
    while (!feof(file))  {
        size_t bytes_read = fread(buffer, 1, BUFFER_SIZE -1, file);
        
        if (send(sockfd, buffer, bytes_read+1, 0) == -1)
        {
            perror("Error sending file data");
            fclose(file);
            return -1;
        }
        bzero(buffer,BUFFER_SIZE);
        
    }
    //close file
    fclose(file);
    return 0;
}

int main(int argc, char const *argv[])
{

    int success=0;
    double sum=0;
	if(argc !=6)
	{
		perror("Usage: ./client  <serverIP> <port>  <sourceCodeFileTobeGraded> <loopnum> <sleep>\n");
        return -1;
	}

	char server_ip[40], ip_port[40], file_path[256];
    int server_port;
  
    strcpy(server_ip, argv[1]);
    server_port = atoi(argv[2]);

    strcpy (file_path,argv[3]);
    int loop=atoi(argv[4]);
    int sleep_time=atoi(argv[5]);
    
    double start_loop=GetTime();

    while(loop--)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            perror("Socket creation failed");
            return -1;
        }

        struct sockaddr_in serv_addr;
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip, &serv_addr.sin_addr.s_addr);

    
        if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))!=0)
        {
            perror("Error connecting");
            return -1;
        }

        double Tsend=GetTime();
    	

        if ( send_file(sockfd,file_path)  != 0)
    	{	
        	printf ("Error sending source file\n");
        	close(sockfd);
        	continue;
    	}	

    	size_t bytes_read;

    	char buffer[1024];

    	bzero(buffer,BUFFER_SIZE);
    	recv(sockfd,buffer,BUFFER_SIZE,0);

        bzero(buffer,BUFFER_SIZE);
        recv(sockfd,buffer,1024,0);


        double Trecv=GetTime();

        double tot_time=Trecv-Tsend;
        sum+=tot_time;
        success++;

        sleep(sleep_time);
        close(sockfd);

    }

    double end_loop=GetTime();

    double loop_time=end_loop-start_loop;
    double avg_res_time=sum/success;
    double throughput=success/loop_time;

    printf("%lf ",avg_res_time);
    printf("%f ",success/loop_time);
    printf(" %d ",success);
    printf("Loop time:%lf ",loop_time);
    



	return 0;
}