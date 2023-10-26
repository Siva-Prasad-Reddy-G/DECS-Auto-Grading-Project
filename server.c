#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<arpa/inet.h>
#include <netdb.h>
#include<sys/socket.h>
#include<pthread.h>

const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;

void error(char *msg) {
  perror(msg);
  exit(1);
}

void *thread_function(void *sockfd)
{
  puts("Thread created");
  int newsockfd = *(int *)sockfd;
  int n;
  
    char buffer[BUFFER_SIZE]; //buffer into which we read  the received file chars
    bzero(buffer, BUFFER_SIZE); //initialize buffer to all NULLs
    FILE *file = fopen("file.cpp", "wb");  //Get a file descriptor for writing received data into file
    if (!file)
    {
        perror("Error opening file");
    }

  //buffer for getting file size as bytes
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    
    
    //first receive  file size bytes
    if (  recv(newsockfd,file_size_bytes,MAX_FILE_SIZE_BYTES,0)  == -1)
    {
        perror("Error receiving file size");
        fclose(file);
    }
   
    int file_size;
    //copy bytes received into the file size integer variable
    memcpy(&file_size, file_size_bytes, sizeof(file_size_bytes));
    
    //some local printing for debugging, print file size 
    
    
    //now start receiving file data
    size_t bytes_read = 0, total_bytes_read =0;
    while (1)
    {
      bytes_read=recv(newsockfd,buffer,BUFFER_SIZE,0);
        total_bytes_read += bytes_read;

        if (bytes_read <= 0)
        {
            fclose(file);
            break;
        }

   
        fwrite(buffer, 1, bytes_read, file);

  
        bzero(buffer, BUFFER_SIZE);
        
        if (total_bytes_read >= file_size)
            break;
    }
    send(newsockfd, "I got your code file for grading\n", 50, 0);
    
        FILE *fp;
        if ( system("g++ file.cpp -o file 2>compiler_err.txt") !=0 )
        {
            fp=fopen("compiler_err.txt","r");
            while(!feof(fp))
            {
                bytes_read+=fread(buffer, 1, 255, fp);
            }
            n = send(newsockfd,buffer,bytes_read,0);       
            if (n < 0)
              error("ERROR writing to socket"); 
            close(newsockfd); 
        } //here if no compiler error 
  
        else if ( system("./file 1>out.txt 2>runtime_err.txt")  !=0 )
        {
          fp=fopen("runtime_err.txt","r");
          while(!feof(fp))
          {
            bytes_read+=fread(buffer, 1, 255, fp);
          }
          n =  send(newsockfd,buffer,bytes_read,0);     
          if (n < 0)
            error("ERROR writing to socket"); 
          close(newsockfd); 
        }  //here if no runtime error
        else if(system("diff out.txt expectedoutput.txt >out_err.txt")!=0)
        {
          puts("output not matched");
          fp=fopen("out_err.txt","r");
          while(!feof(fp))
          {
            bytes_read+=fread(buffer, 1, 255, fp);
          }
          n = send(newsockfd,buffer,bytes_read,0);
          if (n < 0)
            error("ERROR writing to socket"); 
          close(newsockfd); 
        }
        else
        {
          n = send(newsockfd,"PASSED\n",strlen("PASSED"),0);
        }
  close(newsockfd);
  puts("Thread exited");
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) 
{
  int sockfd,  newsockfd, portno; 

  socklen_t clilen; //a type of an integer for holding length of the socket address
  char buffer[500]; //buffer for reading and writing the messages
  struct sockaddr_in serv_addr, cli_addr; //structure for holding IP addresses
  int n;

  if (argc < 2) {
    puts("ERROR, no port provided\n");
    exit(1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0); 

  if (sockfd < 0){
    puts("ERROR opening socket");
    return 1;
  }

  bzero((char *)&serv_addr, sizeof(serv_addr)); // initialize serv_address bytes to all zeros
  
  serv_addr.sin_family = AF_INET; // Address Family of INTERNET
  serv_addr.sin_addr.s_addr = INADDR_ANY;  //Any IP address. 
  serv_addr.sin_port = htons(atoi(argv[1]));  // Need to convert number from host order to network order

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    puts("ERROR binding socket");
    return 1;
  }

  listen(sockfd, 15); 
  while(1)
  {
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

    if (newsockfd < 0)
      error("ERROR on accept");
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_function, &newsockfd) != 0)
      printf("Failed to create Thread\n");
  }

  return(1);

}
