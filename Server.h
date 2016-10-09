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

class Server 
{
public:
    Server();
    void start();
private:
    void runUpdateLoop();
    void parseSystemMessage(const int clientID, char* message);
    void parseIncomingMessage(const int clientID);
    void sendMessageToAllWithoutUser(string message, int clientID);
    bool sendto(int clientID, string message);
    void sendDisconnectedMessage(int clientID);
    void getUserList(int clientID);
private:
    int serverSocket;
    set<int> clients;
    map<int, struct sockaddr_in> ips;
    map<int, string> bufferMessage;
    map<int, string> clientNames;
    map<int, queue<string>> sendQueue;
};

