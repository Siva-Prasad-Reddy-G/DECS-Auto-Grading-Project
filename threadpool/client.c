/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <assert.h>
#include <pthread.h>
#include <sys/queue.h>
#include <string.h>

const char SUBMISSIONS_DIR[] = "./submissions/";
const char EXECUTABLES_DIR[] = "./executables/";
const char OUTPUTS_DIR[] = "./outputs/";
const char COMPILER_ERROR_DIR[] = "./compiler_error/";
const char RUNTIME_ERROR_DIR[] = "./runtime_error/";
const char EXPECTED_OUTPUT[] = "./expected/output.txt";

const char PASS_MSG[] = "PASS\n";
const char COMPILER_ERROR_MSG[] = "COMPILER ERROR\n";
const char RUNTIME_ERROR_MSG[] = "RUNTIME ERROR\n";
const char OUTPUT_ERROR_MSG[] = "OUTPUT ERROR\n";
const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;

struct entry {
           int sockfd;
           TAILQ_ENTRY(entry) entries;             /* Tail queue */
       };
struct tailhead head;                   /* Tail queue head */

TAILQ_HEAD(tailhead, entry);


 // Condition variable to signal when queue goes from empty to non-empty
pthread_cond_t requestInQueue;
int queueEmpty = 1;

 // Mutex for ensuring access to request queue is safe
pthread_mutex_t queueLock;


void error(char *msg) {
  perror(msg);
  exit(1);
}

//Utility Function to receive a file of any size to the grading server
int recv_file(int sockfd, char* file_path)
//Arguments: socket fd, file name (can include path) into which we will store the received file
{
    char buffer[BUFFER_SIZE]; //buffer into which we read  the received file chars
    bzero(buffer, BUFFER_SIZE); //initialize buffer to all NULLs
    FILE *file = fopen(file_path, "wb");  //Get a file descriptor for writing received data into file
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }

	
	//buffer for getting file size as bytes
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    //first receive  file size bytes
    if (recv(sockfd, file_size_bytes, sizeof(file_size_bytes), 0) == -1)
    {
        perror("Error receiving file size");
        fclose(file);
        return -1;
    }
   
    int file_size;
    //copy bytes received into the file size integer variable
    memcpy(&file_size, file_size_bytes, sizeof(file_size_bytes));
    
    //some local printing for debugging
    printf("File size is: %d\n", file_size);
    
    //now start receiving file data
    size_t bytes_read = 0, total_bytes_read =0;;
    while (true)
    {
    	  //read max BUFFER_SIZE amount of file data
        bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0);

        //total number of bytes read so far
        total_bytes_read += bytes_read;

        if (bytes_read <= 0)
        {
            perror("Error receiving file data");
            fclose(file);
            return -1;
        }

		//write the buffer to the file
        fwrite(buffer, 1, bytes_read, file);

	// reset buffer
        bzero(buffer, BUFFER_SIZE);
        
       //break out of the reading loop if read file_size number of bytes
        if (total_bytes_read >= file_size)
            break;
    }
    fclose(file);
    return 0;
}


char* compile_command(int id, char* programFile, char* execFile) {

  char *s;
  char s1[20];
  
  s = malloc (200*sizeof(char));
  memset(s, 0, sizeof(s));
  memset(s1, 0, sizeof(s1));
  strcpy(s, "g++ -o ");
  strcat(s, execFile);
  strcat(s, "  ");
  strcat(s, programFile);
  strcat(s, " 2> compiler_err");
 	sprintf(s1, "%d", id);	
 	strcat(s, s1);	
  strcat(s, ".txt");
  printf("%s\n",s);
  return s;
}
    
char* run_command(int id, char* execFileName) {

  char *s;
  char s1[20];
  
  s = malloc (200*sizeof(char));
  memset(s, 0, sizeof(s));
  memset(s1, 0, sizeof(s1));
 	sprintf(s1, "%d", id);	  

  strcpy(s, "./");
  strcat(s, execFileName);
  strcat(s, " > out");
 	strcat(s, s1);	
  strcat(s, ".txt");
  strcat(s, " 2> runtime_err");
 	strcat(s, s1);	
 	strcat(s, ".txt");	
  printf("%s\n",s);
  return s;
}


char *makeProgramFileName(int id) {

  char *s;
  char s1[20];	
  
  s = malloc (200*sizeof(char));
  memset(s, 0, sizeof(s));
  memset(s1, 0, sizeof(s1));

  sprintf(s1, "%d", id);	  
  strcpy (s, "file");
  strcat (s, s1);
  strcat (s, ".cpp");
  return s;
}  
  
char *makeExecFileName(int id) {

  char *s;
  char s1[20];	
  
  s = malloc (200*sizeof(char));
  memset(s, 0, sizeof(s));
  memset(s1, 0, sizeof(s1));
  sprintf(s1, "%d", id);	  
  strcpy (s, "prog");
  strcat (s, s1);
  return s;
} 

void *grader(void *num) {

  int graderNum = (int) num;
  
  struct entry *reqNode;

	printf ("Grader number %d started and waiting for requests\n", graderNum);
	
  int n;
  reqNode = malloc(sizeof(struct entry));      
   // Check if queue has request, if not wait until there's one in it
  pthread_mutex_lock(&queueLock);		
  while ((queueEmpty = TAILQ_EMPTY(&head)))
    pthread_cond_wait(&requestInQueue, &queueLock);
  reqNode = TAILQ_FIRST(&head);
  int newsockfd = reqNode->sockfd;
  TAILQ_REMOVE(&head, reqNode, entries);       /* Deletion */
  pthread_mutex_unlock(&queueLock);
  free(reqNode);  	
  int MAX_PATH_LENGTH = 250;
  	char source_file[MAX_PATH_LENGTH];
    char executable[MAX_PATH_LENGTH];
    char output_file[MAX_PATH_LENGTH];
    char compiler_error_file[MAX_PATH_LENGTH];
    char runtime_error_file[MAX_PATH_LENGTH];

    snprintf(source_file, MAX_PATH_LENGTH, "%s%d.cpp", SUBMISSIONS_DIR, graderNum);
    snprintf(executable, MAX_PATH_LENGTH, "%s%d.o", EXECUTABLES_DIR, graderNum);
    snprintf(output_file, MAX_PATH_LENGTH, "%s%d.txt", OUTPUTS_DIR, graderNum);
    snprintf(compiler_error_file, MAX_PATH_LENGTH, "%s%d.err", COMPILER_ERROR_DIR, graderNum);
    snprintf(runtime_error_file, MAX_PATH_LENGTH, "%s%d.err", RUNTIME_ERROR_DIR, graderNum);

        
 //Call the recv_file utility function, argument is the filename that the received file will be stored as
		char *programFile, *execFile;
	  programFile = makeProgramFileName((int) pthread_self());
    execFile = makeExecFileName((int) pthread_self());
    
    printf("%s  %s\n", programFile, execFile);
	  
		if (recv_file(newsockfd, programFile) != 0)
    {
         close(newsockfd);
         return 0;
    }
    

    /* send reply to client 
    First argument is the full socket, second is the string, third is   the
  number of characters to write. Fourth are some flags set to 0 */

// Some progress response
    n = send(newsockfd, "I got your code file for grading\n", 33, 0);
    
  //Compile the code file, send back 'COMPILER ERROR' message to client if compile failed
  //Write compiler output to a local file compiler_err.txt
  //NO NEED TO SEND COMPILER OUTPUT BACK TO CLIENT. 	
  char result_msg[MAX_PATH_LENGTH] ;
	char result_details[MAX_PATH_LENGTH] ;

	  char *comp_command, *r_command;
	  
	  comp_command = compile_command((int) pthread_self(), programFile,execFile);
 	  r_command = run_command((int) pthread_self(), execFile);
						
    	if (system(comp_command) !=0 ) {

        snprintf(result_msg, MAX_PATH_LENGTH, COMPILER_ERROR_MSG);
        snprintf(result_details, MAX_PATH_LENGTH, compiler_error_file);

 		}
    
     //here if no compiler error
  //Run the executable, send back 'RUNTIME ERROR' message to client  if runtime error
  //Write runtime error message to a local file runtime_err.txt, program output to a file out.txt
  //NO NEED TO SEND RUNTIME ERROR MESSAGE BACK TO CLIENT. 
  
 		else if (system(r_command) !=0 ) { 			
 				


        snprintf(result_msg, MAX_PATH_LENGTH, RUNTIME_ERROR_MSG);
        snprintf(result_details, MAX_PATH_LENGTH, runtime_error_file);
 		 }  //here if no runtime error
  //Write a message "PROGRAM RAN" to client 
  //NO NEED TO SEND PROGRAM OUTPUT BACK TO CLIENT. 
 		 else { 		 


        snprintf(result_msg, MAX_PATH_LENGTH, OUTPUT_ERROR_MSG);
        snprintf(result_details, MAX_PATH_LENGTH, output_file);	

		}

   if (result_details[0] != '\0') 
        {
            char buffer[BUFFER_SIZE];
            bzero(buffer, BUFFER_SIZE);

            FILE *file = fopen(result_details, "rb");
            while (!feof(file))
            {
                size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
                if (send(newsockfd, buffer, bytes_read, 0) == -1)
                {
                    perror("Error sending result details");
                    fclose(file);
                }
                bzero(buffer, BUFFER_SIZE);
            }
            fclose(file);
        }
        
    close(newsockfd);
    if (n < 0)
      error("ERROR writing to socket");
   free (programFile);
   free (execFile);
   free (comp_command);
   free (r_command);
}

int main(int argc, char *argv[]) {
  int sockfd, //the listen socket descriptor (half-socket)
   newsockfd, //the full socket after the client connection is made
   portno; //port number at which server listens

  socklen_t clilen; //a type of an integer for holding length of the socket address
  char buffer[256]; //buffer for reading and writing the messages
  struct sockaddr_in serv_addr, cli_addr; //structure for holding IP addresses
  int n;

   if (argc < 3) {
        fprintf(stderr, "Usage: %s <port> <thread_pool_size>\n", argv[0]);
        exit(1);
    }

  /* create socket */

  sockfd = socket(AF_INET, SOCK_STREAM, 0); 
  //AF_INET means Address Family of INTERNET. SOCK_STREAM creates TCP socket (as opposed to UDP socket)
 // This is just a holder right now, note no port number either. It needs a 'bind' call


  if (sockfd < 0)
    error("ERROR opening socket");

 
  bzero((char *)&serv_addr, sizeof(serv_addr)); // initialize serv_address bytes to all zeros
  
  serv_addr.sin_family = AF_INET; // Address Family of INTERNET
  serv_addr.sin_addr.s_addr = INADDR_ANY;  //Any IP address. 

//Port number is the first argument of the server command
  portno = atoi(argv[1]);
  serv_addr.sin_port = htons(portno);  // Need to convert number from host order to network order

  /* bind the socket created earlier to this port number on this machine 
 First argument is the socket descriptor, second is the address structure (including port number).
 Third argument is size of the second argument */
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");

  /* listen for incoming connection requests */

  listen(sockfd, 1); // 1 means 1 connection requests can be in queue. 
  //now server is listening for connections


  clilen = sizeof(cli_addr);  //length of struct sockaddr_in

 // Listen socket creation is done, now create worker thread pool
 
 int nthreads = atoi(argv[2]);
 pthread_t thrIDs[nthreads];	
 int thrNum = 0;
 for (int i = 0; i < nthreads; i++) {
		thrNum ++;
 	  int rc = pthread_create(&thrIDs[i], NULL, grader, (void *) thrNum);
			assert(rc == 0);	
  }  

  //set up the request queue
  
  TAILQ_INIT(&head);  /* Initialize the queue */


  pthread_mutex_init(&queueLock, NULL);


  pthread_cond_init(&requestInQueue, NULL);
  
  /* accept a new request, now the socket is complete.
  Create a newsockfd for this socket.
  First argument is the 'listen' socket, second is the argument 
  in which the client address will be held, third is length of second
  */
  int reqID = 0;
  
  while (1){
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
      error("ERROR on accept");
    struct entry *sockfdNode;
    sockfdNode = malloc(sizeof(struct entry));   /* Insert at the tail */
    sockfdNode->sockfd = newsockfd;
    pthread_mutex_lock(&queueLock);
    queueEmpty = TAILQ_EMPTY(&head);    
    TAILQ_INSERT_TAIL(&head, sockfdNode, entries);
    struct entry *np;
    TAILQ_FOREACH(np, &head, entries)
               printf("%d ", np->sockfd);
    printf ("\n");
    if (queueEmpty) pthread_cond_signal(&requestInQueue);
    pthread_mutex_unlock(&queueLock);
    
    	  
  }
    
  return 0;
}

