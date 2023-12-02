#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/time.h>
#include <pthread.h>


typedef struct {
    int *data;
    int front, rear;
    int size, max_size;
} CircularQueue;

void initializeQueue(CircularQueue *queue, int max_size) {
    queue->front = queue->rear = -1;
    queue->size = 0;
    queue->max_size = max_size;
    queue->data = (int *)malloc(max_size * sizeof(int));
}

int isEmpty(CircularQueue *queue) {
    return (queue->size == 0);
}

int isFull(CircularQueue *queue) {
    return (queue->size == queue->max_size);
}

int getFront(CircularQueue *queue) {
    if (isEmpty(queue)) {
        printf("Queue is empty\n");
        return -1;
    }
    return queue->data[queue->front];
}

void push(CircularQueue *queue, int value) {
    if (isFull(queue)) {
        printf("Queue is full\n");
        return;
    }

    if (isEmpty(queue)) {
        queue->front = queue->rear = 0;
    } else {
        queue->rear = (queue->rear + 1) % queue->max_size;
    }

    queue->data[queue->rear] = value;
    queue->size++;
}

int pop(CircularQueue *queue) {
    if (isEmpty(queue)) {
        printf("Queue is empty\n");
        return -1;
    }

    int value = queue->data[queue->front];

    if (queue->front == queue->rear) {
        // If only one element is present, reset the queue
        queue->front = queue->rear = -1;
    } else {
        queue->front = (queue->front + 1) % queue->max_size;
    }

    queue->size--;

    return value;
}

CircularQueue threadPool;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int compareContent(const char *file_output) {
    const char *desired_output = "1 2 3 4 5 6 7 8 9 10 ";
    return strcmp(file_output, desired_output);
}

pthread_mutex_t lock;
pthread_cond_t sp_avail, it_avail;


void* handleClient() {

  while(1){
    int client_socket;
    pthread_mutex_lock(&lock);
    while(isEmpty(&threadPool)){
        pthread_cond_wait(&it_avail,&lock);
    }
    client_socket = pop(&threadPool);
    pthread_cond_signal(&sp_avail);
    pthread_mutex_unlock(&lock);
    char buffer[5000];
    int n;
    bzero(buffer, 5000);
    char filetaking[20] ;
    snprintf(filetaking,20,"received_code%d.c",client_socket);
    int fp = open(filetaking, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fp < 0) {
        error("Error opening file");
    }
    n = read(client_socket, buffer, sizeof(buffer));
    write(fp, buffer, n);
    printf("The file has been received successfully\n");
    close(fp);
    char compile_com[100];
    
    snprintf(compile_com,100,"gcc received_code%d.c -o received_code%d 2> compiler_error%d.txt",client_socket,client_socket,client_socket);
    
    int compile_result = system(compile_com);
    if (compile_result != 0) {
        printf("Compilation failed...\n\n");
        int filec;
        char buffc[5000];
        bzero(buffc, 5000);
        char comp_err[100];
        snprintf(comp_err,100,"compiler_error%d.txt",client_socket);
        filec = open(comp_err, O_RDONLY);
        if (filec < 0) {
            error("Error opening compiler error file");
        }

        while ((n = read(filec, buffc, sizeof(buffc))) > 0) {
            write(client_socket, buffc, n);
        }

        close(filec);
    } else {
        char rec_code[200];
        snprintf(rec_code,200,"./received_code%d 1> output%d.txt 2> runtime_error%d.txt",client_socket,client_socket,client_socket);
        char run_err[100];
        snprintf(run_err,100,"runtime_error%d.txt",client_socket);
        char outp[100];
        snprintf(outp,100,"output%d.txt",client_socket);
        int second_run = system(rec_code);
        if (second_run != 0) {
            printf("Runtime error...\n\n");
            int filer;
            char buffr[5000];
            bzero(buffr, 5000);
            filer = open(run_err, O_RDONLY);
            if (filer < 0) {
                error("Error opening compiler error file");
            }

            while ((n = read(filer, buffr, sizeof(buffr))) > 0) {
                write(client_socket, buffr, n);
            }

            close(filer);
        } 
        else {
            char echo_code[200];
            snprintf(echo_code,200,"echo >> output%d.txt",client_socket);
            system(echo_code);
            char diff_res[200];
            snprintf(diff_res,200,"diff expected_output.txt output%d.txt 1> diff_out%d.txt",client_socket,client_socket);
           
            int diffResult = system(diff_res);
        char oth_res[200];
            snprintf(oth_res,200,"diff_out%d.txt",client_socket);
            int file;
            file = open(oth_res, O_RDONLY);
            if (file < 0) {
                error("Error opening output file");
            }
            char file_output[5000];
            size_t bytes_read = read(file, file_output, sizeof(file_output) - 1);
            file_output[bytes_read] = 'F';
            file_output[bytes_read+1] = 'A';
            file_output[bytes_read+2] = 'I';
            file_output[bytes_read+3] = 'L';
            file_output[bytes_read+4] = '\n';
            file_output[bytes_read+5] = '\0';

            close(file);

            if (bytes_read == 0) {
                printf("File contents match the desired output.\n\n");
                write(client_socket, "PASS\n\n", sizeof("PASS\n\n"));
            } else {
                printf("File contents do not match the desired output.\n\n");
                write(client_socket, file_output, sizeof(file_output));
            }
        }
    }
    close(client_socket);
  }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./server port num_of_threads\n");
        exit(1);
    }
    int max_size = atoi(argv[2]);
    initializeQueue(&threadPool, max_size);
    pthread_mutex_init(&lock,NULL);
    pthread_cond_init(&sp_avail,NULL);
    pthread_cond_init(&it_avail,NULL);
    int sockfd, newsockfd, portno, n;
    char buffer[5000];
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket\n");
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));

    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("Binding Failed\n");
    }

    listen(sockfd, 1000);
    clilen = sizeof(cli_addr);

   for(int d=0;d<max_size;d++){
        pthread_t thread;
    if (pthread_create(&thread, NULL, handleClient, NULL) < 0) {
            perror("Could not create thread");
            return 1;
        }
   }

    while (1) {
      
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (newsockfd < 0) {
            error("Error on accept");
        }
        pthread_mutex_lock(&lock);
     if (isFull(&threadPool)) {
            pthread_cond_wait(&sp_avail,&lock);
        }
        push(&threadPool,newsockfd);
        pthread_cond_signal(&it_avail);
        pthread_mutex_unlock(&lock);
    }

    close(sockfd);
    return 0;
}

