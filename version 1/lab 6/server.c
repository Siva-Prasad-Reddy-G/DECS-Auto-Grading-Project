/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/in.h>

const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;

void error(char *msg) {
  perror(msg);
  exit(1);
}

int recv_file(int sockfd, char* file_path)
{
    char buffer[BUFFER_SIZE]; 
    bzero(buffer, BUFFER_SIZE); 
    FILE *file = fopen(file_path, "wb");  
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }

	
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    
    
    if (  recv(sockfd,file_size_bytes,MAX_FILE_SIZE_BYTES,0)  == -1)
    {
        perror("Error receiving file size");
        fclose(file);
        return -1;
    }
   
    int file_size;
    memcpy(&file_size, file_size_bytes, sizeof(file_size_bytes));
    
    size_t bytes_read = 0, total_bytes_read =0;
    while (true)
    {
    	bytes_read=recv(sockfd,buffer,BUFFER_SIZE,0);

        total_bytes_read += bytes_read;

        if (bytes_read <= 0)
        {
            perror("Error receiving file data");
            fclose(file);
            return -1;
        }
        fwrite(buffer, 1, bytes_read, file);

        bzero(buffer, BUFFER_SIZE);
        
        if (total_bytes_read >= file_size)
            break;
    }
    fclose(file);
    return 0;
}




int main(int argc, char *argv[]) 
{
  int sockfd, newsockfd;
  socklen_t clilen; 
  char buffer[256]; 
  struct sockaddr_in serv_addr, cli_addr; 
  int n;

  if (argc < 2) {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0); 

  if (sockfd < 0)
    error("ERROR opening socket");

 
  bzero((char *)&serv_addr, sizeof(serv_addr)); 
  
  serv_addr.sin_family = AF_INET; 
  serv_addr.sin_addr.s_addr = INADDR_ANY; 

  serv_addr.sin_port = htons(atoi(argv[1]));  
  clilen = sizeof(cli_addr);

  if ( ( bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    error("ERROR on binding");


  listen(sockfd, 1); 

  while (1)
  {

    newsockfd= accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
 
    if (newsockfd < 0)
      error("ERROR on accept");

 
	if (recv_file(newsockfd, "file.c") != 0)
    {
         close(newsockfd);
         continue;
    }
    puts("Received file\n");
    FILE *fp;
    char buffer[BUFFER_SIZE];
    
    if ( system("gcc file.c -o file 2>compiler_err.txt") !=0 ) 
    {
  
	    n = send(newsockfd,"COMPILER ERROR\n",strlen("COMPILER ERROR\n"),0);    
	    if (n < 0)
		  error("ERROR writing to socket");	
        fp=fopen("compiler_err.txt","r");
        while(!feof(fp))
        {
            int bytes_read=fread(buffer,1,BUFFER_SIZE-1,fp);
            if(send(newsockfd,buffer,bytes_read+1,0)==-1)
            {
                perror("Error sending the error details");
                fclose(fp);
                break;
            }
            bzero(buffer,BUFFER_SIZE);
        }
        fclose(fp);
 		close(newsockfd);	
 	} 
 	else if ( system("./file 1>out.txt 2>runtime_err.txt")  !=0 ) 
    { 			
 		n =  send(newsockfd,"RUNTIME ERROR\n",strlen("RUNTIME ERROR\n"),0);    
	    if (n < 0)
		  error("ERROR writing to socket");	
        fp=fopen("runtime_err.txt","r");
        while(!feof(fp))
        {
            int bytes_read=fread(buffer,1,BUFFER_SIZE-1,fp);
            if(send(newsockfd,buffer,bytes_read+1,0)==-1)
            {
                perror("Error sending the error details");
                fclose(fp);
                break;
            }
            bzero(buffer,BUFFER_SIZE);
        }
        fclose(fp);
 		close(newsockfd);	
 	}  
    else if(system("diff out.txt expectedoutput.txt > outputerror.txt")!=0)
    {
        n=send(newsockfd,"Wrong output\n",strlen("Wrong output\n"),0);
        if(n<0)
            error("Error writing to socket");
        fp=fopen("outputerror.txt","r");
        while(!feof(fp))
        {
            int bytes_read=fread(buffer,1,BUFFER_SIZE-1,fp);
            if(send(newsockfd,buffer,bytes_read+1,0)==-1)
            {
                perror("Error sending the error details");
                fclose(fp);
                break;
            }
            bzero(buffer,BUFFER_SIZE);
        }
        fclose(fp);
        close(newsockfd);
    }
 	else 
 	{
 		n = send(newsockfd,"PASSED\n",strlen("PASSED\n"),0);
	    if (n < 0)
		  error("ERROR writing to socket");	
 		 close(newsockfd);	
	}
    
    puts("Completed grading the file\n");
    
    close(newsockfd);
  }

  return 0;
}


