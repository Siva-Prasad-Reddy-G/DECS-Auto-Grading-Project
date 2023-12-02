#include <iostream>
#include <fstream>
#include <cstring>
#include <thread>
#include <filesystem>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <netinet/in.h>
#include<queue>
#include<pthread.h>
#include "FileQueue.h"
#include "utility.h"
#include <chrono>
#include<random>
#include<cassert>
#include<string>

using namespace std;
namespace fs = std::filesystem;

struct ThreadArgs {
    string request_id;
    int client_sockfd;
};

int Size;
pthread_mutex_t Lock=PTHREAD_MUTEX_INITIALIZER, fileLock=PTHREAD_MUTEX_INITIALIZER,StatusFileLock=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t space_available=PTHREAD_COND_INITIALIZER, items_available=PTHREAD_COND_INITIALIZER, data_available=PTHREAD_COND_INITIALIZER;


//Creating Queues  
FileQueue fileQueue("RequestQueue.txt"); 
FileQueue statusQueue("StatusQueue.txt");
FileQueue Queue("Processqueue.txt");

const char SUBMISSIONS_DIR[] = "./submissions/";
const char EXECUTABLES_DIR[] = "./executables/";
const char OUTPUTS_DIR[] = "./outputs/";
const char COMPILER_ERROR_DIR[] = "./compiler_error/";
const char RUNTIME_ERROR_DIR[] = "./runtime_error/";
const char DIFF_ERROR_DIR[]="./diff_error/";
const char EXPECTED_OUTPUT[] = "./expected/output.txt";
const char PASS_MSG[] = "PROGRAM RAN";
const char COMPILER_ERROR_MSG[] = "COMPILER ERROR";
const char RUNTIME_ERROR_MSG[] = "RUNTIME ERROR";
const char OUTPUT_ERROR_MSG[] = "OUTPUT ERROR";


int total_requests = 0;
int served_requests = 0;


//function to create unique ID for requestID using timestamp
string generateUUID() {
    // Get current time
    auto currentTime = std::chrono::system_clock::now();
    auto durationSinceEpoch = currentTime.time_since_epoch();
    auto secondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(durationSinceEpoch);

    // Convert time to string
    std::stringstream ss;
    ss << secondsSinceEpoch.count();

    // Optionally, add a random component for uniqueness
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    int randomNum = dis(gen);

    // Append the random component to the timestamp
    ss << randomNum;

    return ss.str();
}


struct Result {
    string result_msg;
    string result_details;
};


//function to search for the filename in StatusQueue ang get the output message and output file details
Result findResult(const string& filename) {
    ifstream file("StatusQueue.txt");
    string line;
    Result result;

    while (getline(file, line)) {
        istringstream iss(line);
        string file, result_msg, result_details;
        if (getline(iss, file, ',') &&
            getline(iss, result_msg, ',') &&
            getline(iss, result_details)) {
            if (file == filename) {
                result.result_msg = result_msg;
                result.result_details = result_details;
                return result; // Return result when filename is found
            }
        }
    }

    // If filename is not found, set an indicator in the result
    return result;
}


//thread function which takes file from fileQueue and places in Process Queue , processes it and push into StatusQueue and pop from ProcessQueue

void* thread_function(void *)
{
    while(1){
        string filename;

        pthread_mutex_lock(&Lock);
        while(Queue.size()==Size)
        {
        	pthread_cond_wait(&space_available,&Lock);
        }

        pthread_mutex_unlock(&Lock);

        pthread_mutex_lock(&Lock);
        while(fileQueue.size()==0)
        {
        	pthread_cond_wait(&data_available,&Lock);
        }
        filename=fileQueue.pop();
        Queue.push(filename);
        pthread_mutex_unlock(&Lock);

            
        string source_file =  filename;

        //getting request id from filename
        size_t lastSlashPos = source_file.find_last_of('/');
        size_t lastDotPos = source_file.find_last_of('.');
        string request_id = source_file.substr(lastSlashPos + 1, lastDotPos - lastSlashPos - 1);

        string executable = EXECUTABLES_DIR + request_id + ".o";
        string output_file = OUTPUTS_DIR + request_id + ".txt";
        string compiler_error_file = COMPILER_ERROR_DIR + request_id + ".err";
        string runtime_error_file = RUNTIME_ERROR_DIR + request_id+ ".err";
        string diff_error_file=DIFF_ERROR_DIR + request_id+ ".err";
        string compile_command = "g++ " + source_file + " -o " + executable + " > /dev/null 2> " + compiler_error_file;
        string run_command = executable + " > " + output_file + " 2> " + runtime_error_file;
        string diff_command="diff ./expected/output.txt " + output_file + " > " + diff_error_file;

        string result_msg = "";
        string result_details = "";

        if (system(compile_command.c_str()) != 0)
        {
            result_msg = COMPILER_ERROR_MSG;
            result_details = compiler_error_file;
        }
        else if (system(run_command.c_str()) != 0)
        {
            result_msg = RUNTIME_ERROR_MSG;
            result_details = runtime_error_file;
        }
        else if (system(diff_command.c_str())!=0){
            result_msg=OUTPUT_ERROR_MSG;
            result_details = diff_error_file;
        }
        else 
        {
            result_msg = PASS_MSG;
            result_details = output_file;
        }
        
        string status=filename+","+result_msg+","+result_details;

        pthread_mutex_lock(&StatusFileLock);
        statusQueue.push(status.c_str());
        pthread_mutex_unlock(&StatusFileLock);
        
        pthread_mutex_lock(&Lock);
        Queue.removeElement(filename);
        pthread_mutex_unlock(&Lock);


    
    }

}



//function to check the status of a request, check in fileQueue then ProcessQueue and then Status Queue, 
//if found in statusQueue send the output file of this request.
void* status_check(void *args){
    int client_sockfd=*(int*)args;
    char request[256];
    bzero(request,256);
    recv(client_sockfd,request,255,0);
    string request_id=request;
    string submit(SUBMISSIONS_DIR);
    string filename=submit+request_id+".cpp";
   
    int pos=fileQueue.findPosition(filename.c_str());
    if(pos!=-1){
    	string position=to_string(pos);
    	send(client_sockfd,"0",sizeof(int),0);
    	send(client_sockfd,position.c_str(),position.length()+1,0);
        close(client_sockfd);
    	pthread_exit(nullptr);
    }
    pos=Queue.findPosition(filename.c_str());
    if(pos!=-1){
    	pos+=Queue.size();
    	string position=to_string(pos);
    	send(client_sockfd,"3",sizeof(int),0);
    	send(client_sockfd,position.c_str(),position.length(),0);
        close(client_sockfd);
    	pthread_exit(nullptr);
    }
    
    string result_Msg,result_Details;
    Result foundResult = findResult(filename.c_str());
    if (foundResult.result_msg.empty() ) {
    	send(client_sockfd,"1",sizeof(int),0);
        close(client_sockfd);
        pthread_exit(nullptr);
        
    } else {
    	result_Msg=foundResult.result_msg;
        result_Details=foundResult.result_details;
    	
    }
    // File containing data
    ifstream inputFile(result_Details); 
    string file_to_send="./results/result_"+request_id+".txt";

	// New file to store combined content
    ofstream outputFile(file_to_send.c_str(), ios::app); 

    if (outputFile.is_open() && inputFile.is_open()) {
        outputFile << "\nDONE\n" << result_Msg << endl; // Append result_Msg to the new file

        // Append data from the input file to the new file
        outputFile << inputFile.rdbuf();

        inputFile.close(); // Close the input file
        outputFile.close(); // Close the output file

    } 
    string removing_element = filename+","+result_Msg+","+result_Details;
    pthread_mutex_lock(&StatusFileLock);
    statusQueue.removeElement(removing_element);
    pthread_mutex_unlock(&StatusFileLock);
    send(client_sockfd,"2",sizeof(int),0);
    if(send_file(client_sockfd,file_to_send.c_str())!=0){
    	perror("Error in sending result to client");
        close(client_sockfd);
    	pthread_exit(nullptr);
    }
    
    pthread_exit(NULL);
}


//function to add a new request file into system and send a message to client that request accepted
void* new_request(void *args){
    int sockfd= *(int*)args;
    string request_id=generateUUID();
    string submit(SUBMISSIONS_DIR);
    string filename=submit+request_id+".cpp";
    	
    if (recv_file(sockfd, filename) != 0)
    {
        close(sockfd);
        pthread_exit(nullptr);
    }
    pthread_mutex_lock(&fileLock);
    fileQueue.push(filename.c_str());
    pthread_cond_signal(&data_available);
    pthread_mutex_unlock(&fileLock);
    string message="Submission accepted for grading , your request id is:"+request_id;
    send(sockfd,request_id.c_str(),100,0);
    send(sockfd,message.c_str(),100,0);
    int n=5;
    while(n--)
    {
        status_check(&sockfd);
    }

    pthread_exit(NULL);

}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "Usage: ./server  <port> <pool-size>\n";
        return -1;
    }
     
    Size=atoi(argv[2]);
    int port = stoi(argv[1]);

    //setting up the socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Socket creation failed");
        return 1;
    }
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    int iSetOption = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&iSetOption, sizeof(iSetOption));
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
    {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }
    if (listen(sockfd, 50) != 0)
    {
        perror("Listen failed");
        close(sockfd);
        return -1;
    }

    cout << "Server listening on port: " << port << "\n";

	// creating the directories
    try
    {
        if (!fs::exists(SUBMISSIONS_DIR))
            fs::create_directory(SUBMISSIONS_DIR);
        if (!fs::exists(EXECUTABLES_DIR))
            fs::create_directory(EXECUTABLES_DIR);
        if (!fs::exists(OUTPUTS_DIR))
            fs::create_directory(OUTPUTS_DIR);
        if (!fs::exists(COMPILER_ERROR_DIR))
            fs::create_directory(COMPILER_ERROR_DIR);
        if (!fs::exists(RUNTIME_ERROR_DIR))
            fs::create_directories(RUNTIME_ERROR_DIR);
        if (!fs::exists(DIFF_ERROR_DIR))
            fs::create_directories(DIFF_ERROR_DIR);
        if (!fs::exists(EXPECTED_OUTPUT))
            fs::create_directories(EXPECTED_OUTPUT);
        
    }
    catch (fs::filesystem_error &e)
    {
        cerr << "Error creating directories: " << e.what() << "\n";
        close(sockfd);
        return -1;
    }
    
    // creating the threadpool
    pthread_t receive_thread[Size];
	for (int i = 0; i < Size; i++) 
	{
        int rc=pthread_create(&receive_thread[i], nullptr, thread_function, nullptr);
        assert(rc==0);
    }


    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // server will run forever
    while (true)
    {
        int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        int size_int=sizeof(int);
        char request_type[size_int];


        //receive request type
    	if (recv(client_sockfd, request_type, sizeof(request_type), 0) == -1)
    	{
        	perror("Error receiving request");
        	return -1;
    	}
    	int type=atoi(request_type);
    	
    	//new request
    	if(type==0){ 

                pthread_t new_req;
                int rc= pthread_create(&new_req,nullptr,new_request,&client_sockfd);
                assert(rc==0);
                pthread_detach(rc);

                
           
    	}
    	
    	// status request
    	else{

            pthread_t status_req;
            int rc= pthread_create(&status_req,nullptr, status_check,&client_sockfd);
            assert(rc==0);
            pthread_detach(rc);

       
    		
    	}        
    }

    close(sockfd);
    return 0;
}
