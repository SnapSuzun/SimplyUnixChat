#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <vector>
#include <pthread.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <set>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <queue>
#include "helpers.h"
#include "Converter.h"
using namespace std;

class Client 
{
public:
    Client();
    void start();
private:
    void runUpdateLoop();
    void sendCommonMessage(char* messageB);
    void parseIncomingMessage(char *message);
    void sendPrivateMessage(int clientID, string message);
    void parseFileMessage(char* message);
    void parseIncomingCommonMessage(char* message, bool PM);
    void parseSystemMessage(char *message);
    bool checkMessageOnSystem(string message);
    SystemCodes getSystemCodeFromSystemMessage(string message, int &additional_parameter);
    bool sendFile(string message);
    int lockFd(int fd, int blocking);
private:
    string username;
    int serverSocket;
    map<int, string> clientNames;
    map<int, string> bufferMessages;
    queue<string> sendQueue;
    FILE *file;
    FILE *recvFile;
    int sendFileToUser;
    int recvFileFromUser;
};

