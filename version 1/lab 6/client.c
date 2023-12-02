#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <error.h>
#include <stdlib.h>
#include <stdbool.h>

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
    memcpy(file_size_bytes, &file_size, sizeof(file_size));
    
    //send file size to server, return -1 if error
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

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        perror("Usage: ./client  <serverIP> <port>  <sourceCodeFileTobeGraded>\n");
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

    int tries = 0;
    while (true)
    {
        if (  connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))   == 0)
            break;
        sleep(1);
        tries += 1;
        if (tries == MAX_TRIES)
        {
            printf ("Server not responding\n");
            return -1;
        }
    }
    

    if ( send_file(sockfd,file_path)  != 0)
    {
        printf ("Error sending source file\n");
        close(sockfd);
        return -1;
    }

 		printf ("Code sent for grading, waiting for response\n");
    size_t bytes_read;

    char buffer[BUFFER_SIZE];
    while (true)
    {
        bzero(buffer,BUFFER_SIZE);
        bytes_read= recv(sockfd, buffer, BUFFER_SIZE,0); 

        if (bytes_read <= 0)
            break;
        puts(buffer);
        puts("\n");
    }
    close(sockfd);

    return 0;
}
