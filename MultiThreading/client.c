#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <error.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
const int BUFFER_SIZE = 1024; 
const int MAX_FILE_SIZE_BYTES = 4;
const int MAX_TRIES = 5;


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
    //copy the bytes of the file size integer into the char buffer
    memcpy(file_size_bytes, &file_size, sizeof(file_size));
    
    
    if (send(sockfd, file_size_bytes,MAX_FILE_SIZE_BYTES,0)  == -1)
    {
        perror("Error sending file size");
        fclose(file);
        return -1;
    }
    printf("Send file size %d",file_size);

    while (!feof(file)) 
    {
    
    		//read buffer from file
        size_t bytes_read = fread(buffer, 1, BUFFER_SIZE -1, file);
        
     		//send to server
        if (send(sockfd, buffer, bytes_read+1, 0) == -1)
        {
            perror("Error sending file data");
            fclose(file);
            return -1;
        }
        bzero(buffer,BUFFER_SIZE);
        
        

    }
    
    fclose(file);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        perror("Usage: ./submit  <serverIP> <port>  <sourceCodeFileTobeGraded> <loop> <sleep>\n");
        return -1;
    }

    char server_ip[40], ip_port[40], file_path[256];
    int server_port;

   
    strcpy(server_ip, argv[1]);
    server_port = atoi(argv[2]);

    strcpy (file_path,argv[3]);
    

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

    struct timeval start,end,startloop,endloop;
    int loopnum=atoi(argv[4]);
    int sleept=atoi(argv[5]);
    int avg=0,sucessfull=0;
    int duplicate=loopnum;
    

    gettimeofday(&startloop,NULL);
    while(loopnum)
    {
        if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==-1)
        {
            puts("Error connected");
        }
        
        
    gettimeofday(&start,NULL);
        if ( send_file(sockfd,file_path)  != 0)
        {
            printf ("Error sending source file\n");
            gettimeofday(&end, NULL);
            int time_taken=(end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
            avg+=time_taken;
            break;
        }

        size_t bytes_read;
        //buffer for reading server response
        char buffer[BUFFER_SIZE];
        recv(sockfd, buffer, BUFFER_SIZE,0); 
        bzero(buffer,BUFFER_SIZE);
            //read server response into buffer
            /* ***FIB Here */
        bytes_read= recv(sockfd, buffer, BUFFER_SIZE,0); 

        if (bytes_read <= 0)
            break;
        gettimeofday(&end,NULL);
        sucessfull++;
        int time_taken=(end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
        avg+=time_taken;

        loopnum--;
        sleep(sleept);
        close(sockfd);
    }
    
    gettimeofday(&endloop, NULL);
    int time_taken=(endloop.tv_sec * 1000000 + endloop.tv_usec) - (startloop.tv_sec * 1000000 + startloop.tv_usec);
  
    float d=(float)(avg)/duplicate;


    printf("Average response time %f\n",d);

    printf("Time taken %d\n",time_taken);

    printf("No.of sucessfull requests %d\n",sucessfull);

    return 0;
}
