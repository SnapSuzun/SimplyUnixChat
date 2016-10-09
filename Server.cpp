#include "Server.h"

Server::Server() 
{
    this->serverSocket = -1;
}

void Server::start()
{
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    
    addr.sin_family=AF_INET;
    addr.sin_port=0;
    addr.sin_addr.s_addr=INADDR_ANY;
    this->serverSocket=socket(AF_INET,SOCK_STREAM,0);
    if(this->serverSocket < 0)
    {
        perror("Can't create server socket");
        exit(1);
    }
    fcntl(this->serverSocket, F_SETFL,O_NONBLOCK);
    if(bind(this->serverSocket,(struct sockaddr *)&addr,sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
    }
    int addr_len = sizeof(addr);
    getsockname(this->serverSocket, (struct sockaddr*)&addr, (socklen_t*)&addr_len);  
    printf("Port is %d\n", ntohs(addr.sin_port));
    
    listen(this->serverSocket, 2);
    printf("Server created!\n");
    this->runUpdateLoop();
}

void Server::runUpdateLoop()
{
    while(1)
    {
        fd_set readset;
        fd_set sendset;
        FD_ZERO(&readset);
        FD_ZERO(&sendset);
        FD_SET(this->serverSocket, &readset);
        
        for(set<int>::iterator it = this->clients.begin(); it != this->clients.end(); it++)
        {
            FD_SET(*it, &readset);
            if(!this->sendQueue[*it].empty())
                FD_SET(*it, &sendset);
        }
        timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        
        int mx = max(this->serverSocket, *max_element(this->clients.begin(), this->clients.end()));
        
        if(select(mx+1, &readset, &sendset, NULL, &timeout) <= 0)
        {
            continue;
        }
        
        if(FD_ISSET(this->serverSocket, &readset))
        {
            struct sockaddr_in from;
            unsigned int len = sizeof(from);
            int sock = accept(this->serverSocket, (struct sockaddr *)&from,&len);
            this->ips[sock] = from;
            if(sock < 0)
            {
                perror("Can't accept socket");
                exit(3);
            }
            
            fcntl(sock, F_SETFL, O_NONBLOCK);
            this->clients.insert(sock);
            printf("New user was connected with IP = %s and ID = %d\n", inet_ntoa(from.sin_addr), sock);
        }
        
        for(set<int>::iterator it = this->clients.begin(); it != this->clients.end(); it++)
        {
            if(FD_ISSET(*it, &sendset))
            {
                string data = this->sendQueue[*it].front();
                printf("Send data = %s to %d\n", &data.c_str()[7], *it);
                if(send(*it,data.c_str(), data.length(),0) <= 0)
                {
                    printf("!Send\n");
                } else this->sendQueue[*it].pop();
                
            }
            else if(FD_ISSET(*it, &readset))
            {
                this->parseIncomingMessage(*it);
            }
        }
        
    }
    
    close(this->serverSocket);
}

void Server::parseIncomingMessage(const int clientID)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    if(recv(clientID, &buffer, sizeof(buffer), 0) <= 0)
    {
        this->sendDisconnectedMessage(clientID);
        return;
    }
    printf("Get message\n");

    messageTypes messageType = (messageTypes)buffer[0];

    switch(messageType)
    {
        case messageTypes::CommonMessage:
        {
            printf("CommonMessage from %d\n", clientID);
            unsigned short int bytesCount = Converter::atosi(&buffer[1]);
            char redirMess[bytesCount + 7];
            redirMess[0] = buffer[0];
            Converter::itoa(clientID, &redirMess[1]);
            mempcpy(&redirMess[5], &buffer[1], bytesCount + 2);
            this->sendMessageToAllWithoutUser(string(redirMess, bytesCount + 7), clientID);
            return;
            
            unsigned char messageCount = (unsigned char)buffer[3];
            char mesBuffer[bytesCount - 1];
            memset(mesBuffer, 0, bytesCount - 1);
            mempcpy(mesBuffer, &buffer[4], bytesCount - 1);

            if(messageCount > 1)
            {
                this->bufferMessage[clientID] = mesBuffer;
            }
            else
            {
                printf("%s\n", &buffer[4]);
            }
            break;
        }
        case messageTypes::FileMessage:
        case messageTypes::PrivateMessage:
        {
            int toID = atoi(&buffer[1]);
            printf("PM to %d, from %d\n", toID, clientID);
            Converter::itoa(clientID, &buffer[1]);
            unsigned short int bytesCount = Converter::atosi(&buffer[5]);
            this->sendto(toID, string(buffer, bytesCount + 7));
            break;
        }
        
        case messageTypes::SystemMessage:
        {
            printf("SystemMessage\n");
            this->parseSystemMessage(clientID, &buffer[1]);
            break;
        }
        default:
            printf("Unknown message\n");
            break;
    }
}

void Server::parseSystemMessage(const int clientID, char* message)
{
    SystemCodes type = (SystemCodes)message[0];
    switch(type)
    {
        case SystemCodes::Connect:
        {
            for(map<int, string>::iterator it = this->clientNames.begin(); it != this->clientNames.end(); it++)
            {
                if(it->second == string(&message[2]))
                {
                    printf("Name exist!!");
                    char buffer[2];
                    buffer[0] = (unsigned char)messageTypes::SystemMessage;
                    buffer[1] = (unsigned char)SystemCodes::Error;
                    this->sendto(clientID, string(buffer, 2));
                    return;
                }
            }
            unsigned char bytesCount = message[1];
            this->clientNames[clientID] = &message[2];
            char bufferMessage[6];
            bufferMessage[0] = (unsigned char)messageTypes::SystemMessage;
            bufferMessage[1] = (unsigned char)SystemCodes::Connect;
            Converter::itoa(clientID, &bufferMessage[2]);
            string bufferStr(bufferMessage, 6);
            bufferStr += &message[2];
            this->bufferMessage[1] = (unsigned char)SystemCodes::GetUserList;
            for(set<int>::iterator it = this->clients.begin(); it != this->clients.end(); it++)
            {
                if(*it!=clientID)
                {
                    this->sendto(*it, bufferStr);
                    Converter::itoa(*it, &bufferMessage[2]);
                    this->sendto(clientID, string(bufferMessage,6) + this->clientNames[*it]);
                }
            }
            break;
        }
        case SystemCodes::GetUserList:
        {
            this->getUserList(clientID);
            break;
        }
        case SystemCodes::SendFile:
        {
            char buffer = (unsigned char)messageTypes::SystemMessage;
            int toID = atoi(&message[1]);
            printf("SendFile to %d, from %d, %s\n", toID, clientID, &message[7]);
            Converter::itoa(clientID, &message[1]);
            unsigned short int bytesCount = Converter::atosi(&message[5]);
            this->sendto(toID, buffer + string(message, bytesCount + 7));
            break;
        }
        default:
            printf("Unknown system type\n");
            break;
    }
}

bool Server::sendto(int clientID, string message)
{
    usleep(10000);
    
    int result = send(clientID,message.c_str(), message.length(),0);
    if(result <= 0)
    {
        printf("Send error!!\n");
        return false;
    }
    return true;
}

void Server::sendMessageToAllWithoutUser(string message, int clientID)
{
    for(set<int>::iterator it = this->clients.begin(); it != this->clients.end(); it++)
    {
        if(*it!=clientID)
        {
            this->sendto(*it, message);
        }
    }
}

void Server::getUserList(int clientID)
{
    char bufferMessage[6];
    bufferMessage[0] = (unsigned char)messageTypes::SystemMessage;
    bufferMessage[1] = (unsigned char)SystemCodes::GetUserList;
    
    for(set<int>::iterator it = this->clients.begin(); it != this->clients.end(); it++)
    {
        if(*it!=clientID)
        {
            Converter::itoa(*it, &bufferMessage[2]);
            this->sendto(clientID, string(bufferMessage,6) + this->clientNames[*it]);
        }
    }
}

void Server::sendDisconnectedMessage(int clientID)
{
    printf("User with ip: %s was disconnected\n", inet_ntoa(this->ips[clientID].sin_addr));
    close(clientID);
    this->clients.erase(clientID);
    map<int, string>::iterator it = this->clientNames.find(clientID);
    this->clientNames.erase(it);
    
    char buffer[6];
    buffer[0] = (unsigned char)messageTypes::SystemMessage;
    buffer[1] = (unsigned char)SystemCodes::Disconnect;
    Converter::itoa(clientID, &buffer[2]);
    this->sendMessageToAllWithoutUser(string(buffer,6), 0);
}