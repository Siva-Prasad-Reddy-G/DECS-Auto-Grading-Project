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
int Error=0;
int timeout_err=0;


double GetTime() {
    struct timeval t;
    int rc = gettimeofday(&t, NULL);
    return (double) t.tv_sec + (double) t.tv_usec/1e6;
}

int send_file(int sockfd,char *file_path)
{
    char buffer[BUFFER_SIZE];
    bzero(buffer,BUFFER_SIZE);
    FILE *file= fopen(file_path,"rb");
    if(!file)
    {
        perror("ERROR opening file");
        return -1;
    }

    fseek(file,0L,SEEK_END);
    int file_size=ftell(file);
    fseek(file,0L,SEEK_SET);

    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    memcpy(file_size_bytes, &file_size, sizeof(file_size));

    if(send(sockfd,file_size_bytes,MAX_FILE_SIZE_BYTES,0)==-1)
    {
        perror("ERROR sending file size");
        fclose(file);
        return -1;
    }

    while(!feof(file))
    {
        size_t bytes_read=fread(buffer,1,BUFFER_SIZE-1,file);

        if(send(sockfd,buffer,bytes_read+1,0)==-1)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                perror("Sending timeout occurred");
                timeout_err++;
                Error++;
                close(sockfd);
                continue;
            } 
            else 
            {
                perror("ERROR Sending");
                Error++;
                close(sockfd);
                continue;
            }
        }
        bzero(buffer,BUFFER_SIZE);
    }
    fclose(file);
    return 0;


}

int main(int argc, char const *argv[])
{
    int success=0;
    double sum=0;
    int n;
    if(argc!=8)
    {
        perror("Usage: ./client  <serverIP> <port>  <sourceCodeFileTobeGraded> <loopnum> <sleep> <timeout> <id>\n");
        return -1;
    }
    char server_ip[20],ip_port[30],file_path[256];
    int server_port;
    int time_out=atoi(argv[6]);
    struct timeval timeout;
    timeout.tv_sec = time_out;   
    timeout.tv_usec = 0;

    strcpy(server_ip, argv[1]);
    server_port=atoi(argv[2]);

    strcpy (file_path,argv[3]);
    int loop=atoi(argv[4]);
    int icount=loop;
    int sleep_time=atoi(argv[5]);
    int prog_id=atoi(argv[7]);

    double start_loop=GetTime();

    while(loop--)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            perror("Socket creation failed");
            return -1;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) 
        {
            perror("ERROR setting socket options");
            close(sockfd);
            continue;
        }

        struct sockaddr_in serv_addr;
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip, &serv_addr.sin_addr.s_addr);


        if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))!=0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN) 
            {
                perror("Connection timeout occurred");
                timeout_err++;
                Error++;
                close(sockfd);
                continue;
            }
            else 
            {
                perror("ERROR connecting");
                Error++;
                close(sockfd);
                continue;
            }
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
        bzero(buffer,1024);
        n=recv(sockfd,buffer,1024,0);

        double Trecv=GetTime();

        if (n < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN) 
            {
                perror("Receive timeout occurred");
                timeout_err++;
                Error++;
                close(sockfd);
                continue;
            }    
            else 
            {
                perror("ERROR Receiving");
                Error++;
                close(sockfd);
                continue;
            }
        }
        double tot_time=Trecv-Tsend;
        sum+=tot_time;
        success++;

        sleep(sleep_time);
        close(sockfd);
    }

    double end_loop=GetTime();

    double loop_time=end_loop-start_loop;
    float average = (float) sum/icount;

    printf("Successful %d of %d. Average time taken in prog %d = %f with %d loop iterations. Total time taken for loop = %lf ms. Throughput = %f Rate of timeouts = %f Rate of errors = %f\n", success, icount, prog_id, average, icount, loop_time, (float) (success)/loop_time, (float) (timeout_err*1000)/loop_time, (float) (Error*1000)/loop_time);



    return 0;
}