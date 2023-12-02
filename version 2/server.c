/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/in.h>
#include<pthread.h>
#include<fcntl.h>

const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;

void error(char *msg) {
  perror(msg);
  exit(1);
}

int recv_file(int sockfd, char* file_path)
{
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    char buffer[BUFFER_SIZE]; 
    bzero(buffer, BUFFER_SIZE); 
    FILE *file = fopen(file_path, "wb");  
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }
	
     recv(sockfd,file_size_bytes,MAX_FILE_SIZE_BYTES,0) ;
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
void *thread_function(void *fd)
{
    int newsockfd=  * (int *)fd;
    char cfname[30];
    char errfname[30];
    char opfname[30];
    char exefname[30];
    char compile_cmd[100];
    char run_cmd[100];
    char diff_cmd[100];
    int n;

    sprintf(cfname, "./grader/src%d.c", newsockfd);
    sprintf(errfname, "./grader/err%d.txt", newsockfd);
    sprintf(opfname, "./grader/op%d.txt", newsockfd);
    sprintf(exefname, "./grader/exe%d", newsockfd);
    int cfd=creat(cfname,00700);
    

    if (recv_file(newsockfd, cfname) != 0)
    {
         close(newsockfd);
         pthread_exit(NULL);
         
    }

    sprintf(compile_cmd, "gcc -o %s %s 2> %s", exefname, cfname, errfname);
    sprintf(run_cmd, "./%s 1> %s 2> %s", exefname, opfname, errfname);
    sprintf(diff_cmd, "diff %s expectedoutput.txt", opfname);
    
    FILE *fp;
    char buffer[BUFFER_SIZE];
    
    if ( system(compile_cmd) !=0 ) 
    {
  
        n = send(newsockfd,"COMPILER ERROR\n",strlen("COMPILER ERROR\n"),0);    
        fp=fopen(errfname,"r");
        bzero(buffer,1024);
        fread(buffer,1,BUFFER_SIZE-1,fp);
        send(newsockfd,buffer,1024,0);
        fclose(fp);
    } 
    else if ( system(run_cmd)  !=0 ) 
    {           
        n =  send(newsockfd,"RUNTIME ERROR\n",strlen("RUNTIME ERROR\n"),0);    
        if (n < 0)
          error("ERROR writing to socket"); 
        fp=fopen(errfname,"r");
        bzero(buffer,1024);
        fread(buffer,1,BUFFER_SIZE-1,fp);
        send(newsockfd,buffer,1024,0);
        fclose(fp);
    }  
    else if(system(diff_cmd)!=0)
    {
        n=send(newsockfd,"Wrong output\n",strlen("Wrong output\n"),0);
    }
    else 
    {
        n = send(newsockfd,"PASSED\n",strlen("PASSED\n"),0); 
    }
    close(newsockfd);
    pthread_exit(NULL);

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


  listen(sockfd, 512); 
  
  while (1)
  {
    newsockfd= accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

	if (newsockfd < 0)
    {
        perror("ERROR on accept"); 
        continue;
    }

    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_function, &newsockfd) != 0)
    {
        perror("Failed to create Thread");
    }
    
    sleep(1);
    close(newsockfd);
    
  }
  
  return 0;
}


