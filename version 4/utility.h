#ifndef SEND_RECV_UTILITY_H
#define SEND_RECV_UTILITY_H

#include<string>
#include<fstream>
#include <netinet/in.h>
#include <unistd.h>
#include<cstring>
using namespace std;

const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;

int send_file(int sockfd, const char* file_path);
int recv_file(int sockfd, string file_path);



#endif 