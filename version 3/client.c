#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    char *fname;
    int sockfd = 0;
    // checking number of arguments are 7
    if (argc != 7)
    {
        printf("Usage: <server-IP> <server-port> <file-name> <loop num> <sleep time> <timeout>\n");
        exit(1);
    }
    // intialising the structures
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // getting the port number
    int portno = atoi(argv[2]);
    server = gethostbyname(argv[1]);

    if (server == NULL)
    {
        printf("No such host available\n");
        exit(0);
    }

    // setting values to 0 for the server address
    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_port = htons(portno);
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);

    // creating the buffers
    char fbuff[10000];
    fname = argv[3];
    int fd = open(fname, O_RDONLY);
    int fbr = read(fd, &fbuff, 10000);
    int timeout = atoi(argv[6]);
    int count = atoi(argv[4]);
    int ic = count;
    int sleep_time = atoi(argv[5]);
    struct timeval times;
    times.tv_sec = timeout;
    times.tv_usec = 0;
    int time_sum = 0;
    int succ = 0;
    int timeout_err = 0;
    int error = 0;

    // total time taken for loop
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    while (count--)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd < 0)
        {
            printf("Error in creating a socket\n");
            exit(1);
        }

        // Set the receive timeout
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&times, sizeof(times)) < 0)
        {

            perror("setsockopt");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            if (errno = EWOULDBLOCK || errno == EAGAIN)
            {
                perror("Timeout occured at connection.");
                timeout_err++;
            }
            else
            {
                perror("Connection error in client");
            }
            error++;
            continue;
        }

        // start time
        struct timeval start_time;
        gettimeofday(&start_time, NULL);

        int fbw = write(sockfd, fbuff, fbr);

        if (fbw < 0)
        {
            if (errno = EWOULDBLOCK || errno == EAGAIN)
            {
                perror("Timeout occured at writing.");
                timeout_err++;
            }
            else
            {
                perror("Error in writing");
            }
            error++;
            continue;
        }
        else
        {
            char res[10000];
            int resbytes = read(sockfd, &res, 10000);
            if (resbytes < 0)
            {
                if (errno = EWOULDBLOCK || errno == EAGAIN)
                {
                    perror("Timeout occured at reading.");
                    timeout_err++;
                }
                else
                {
                    perror("Error in reading");
                }
                error++;
                continue;
            }
            succ++;
            // end time
            struct timeval end_time;
            gettimeofday(&end_time, NULL);
            int t_diff = (end_time.tv_sec * 1000 + end_time.tv_usec / 1000) - (start_time.tv_sec * 1000 + start_time.tv_usec / 1000);
            time_sum += t_diff;
        }
        sleep(sleep_time);
        close(sockfd);
    }

    // creating the variable of time timeval
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    int t_diff = (end_time.tv_sec * 1000 + end_time.tv_usec / 1000) - (start_time.tv_sec * 1000 + start_time.tv_usec / 1000);

    // finding the average time
    float average = (float)time_sum / ic;
    printf("average:%f \nloop_iterations:%d \ntotal_time:%d \nthroughput:%f\ntimeouts:%d\n", average, ic, t_diff, (float)(succ * 1000) / t_diff, timeout_err);

    close(fd);
}
