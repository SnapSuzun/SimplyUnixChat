#include "Client.h"

Client::Client() 
{
    this->username = "";
    this->serverSocket = 0;
    this->file = NULL;
    this->recvFile = NULL;
    this->sendFileToUser = -1;
    this->recvFileFromUser = -1;
}

void Client::start()
{
    string server_ip = "127.0.0.1";
    unsigned int server_port = 49152;
    printf("Set server IP: ");
    cin >> server_ip;
    printf("Set server port: ");
    cin >> server_port;
    
    int &sock = this->serverSocket;
    struct sockaddr_in addr;
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("Can't create socket");
        exit(1);
    }
    
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    addr.sin_addr.s_addr = inet_addr(server_ip.data());
    
    if(connect(sock,(struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("Can't connect to server");
        exit(2);
    }
    printf("Connected to server\n");
    string clientName = "";
    while(1)
    {
        printf("Set your name: ");
        cin >> clientName;
        char buffer[1024];
        buffer[0] = (char)messageTypes::SystemMessage;
        buffer[1] = (char)SystemCodes::Connect;
        buffer[2] = (unsigned char)clientName.length();
        string str = string(buffer,3) + clientName;
        if(send(sock, str.c_str(), str.length(), 0) <= 0)
        {
            printf("Send error!!!");
            return;
        }
        memset(buffer, 0, 1024);
        if(recv(sock, buffer,1024,0) <= 0)
        {
            printf("Connection with server lost!\n");
            return;
        }
        
        if((messageTypes)buffer[0] == messageTypes::SystemMessage && (SystemCodes)buffer[1] == SystemCodes::Error)
        {
            printf("Error!! Name = '%s' is used!!\n", clientName.c_str());
            continue;
        }
        printf("Online users:\n");
        this->parseIncomingMessage(buffer);
        break;
    }
    
    this->username = clientName;
    
    fcntl(STDIN_FILENO, F_SETFL,O_NONBLOCK);
    fcntl(sock, F_SETFL,O_NONBLOCK);
    this->runUpdateLoop();
    close(sock);
}

void Client::runUpdateLoop()
{
    fd_set readset;
    fd_set sendset;
    while(1)
    {
        FD_SET(this->serverSocket, &readset);
        FD_SET(STDIN_FILENO, &readset);
        
        if(!this->sendQueue.empty() && !FD_ISSET(this->serverSocket, &sendset))
        {
            FD_SET(this->serverSocket, &sendset);
        }
        
        timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if(select(this->serverSocket + 1, &readset, &sendset, NULL, &timeout) <= 0)
        {
            continue;
        }
        
        if(FD_ISSET(STDIN_FILENO, &readset))
        {
            string str;
            getline(std::cin, str);
            
            if(str.length() > 0)
            {
                if(!this->checkMessageOnSystem(str))
                    this->sendCommonMessage((char*)str.c_str());
            }
        }
        
        if(FD_ISSET(this->serverSocket, &readset))
        {
            char buffer[1024];
            memset(buffer, 0, 1024);
            if(recv(this->serverSocket, buffer,1024,0) <= 0)
            {
                printf("Connection with server lost!\n");
                return;
            }
            FD_CLR(this->serverSocket, &readset);
            this->parseIncomingMessage(buffer);
        }
        
        else if(FD_ISSET(this->serverSocket, &sendset))
        {
            string str = this->sendQueue.front();
            this->sendQueue.pop();
            this->sendFile(str);
            send(this->serverSocket, str.c_str(),str.length(),0);
            usleep(10000);
            FD_CLR(this->serverSocket, &sendset);
        }
    }
}

void Client::sendCommonMessage(char* messageB)
{
    string message(messageB);
    unsigned char messageCount = (message.length() + 1) / (sendBufferSize - sizeof(usint) - 1) + (((message.length() + 1) % (sendBufferSize - sizeof(usint) - 1)) == 0 ? 0 : 1);
    for(int i=0; i < messageCount; i++)
    {
        string buffer;
        char sendBuffer[sendBufferSize];
        memset(sendBuffer, 0, sendBufferSize);
        usint size = 1;
        sendBuffer[0] = (unsigned char)messageTypes::CommonMessage;
        if(i==0)
        {
            buffer = message.substr(0,sendBufferSize - sizeof(size) - 2);
            size += buffer.length() + 1;
            Converter::itoa((usint)(buffer.length() + 1), &sendBuffer[1]);
            sendBuffer[3] = messageCount;
            mempcpy(&sendBuffer[4], buffer.c_str(), buffer.length());
            size += 2;
        }
        else 
        {
            buffer = message.substr(i * (sendBufferSize - sizeof(usint) -1) - 1, (sendBufferSize - sizeof(usint) - 1));
            size += buffer.length();
            Converter::itoa((usint)buffer.length(), &sendBuffer[1]);
            mempcpy(&sendBuffer[3], buffer.c_str(), buffer.length());
            size += 2;
        }
        this->sendQueue.push(string(sendBuffer, size));
    }
}

void Client::sendPrivateMessage(int clientID, string message)
{
    unsigned char messageCount = (message.length() + 1) / (sendBufferSize - sizeof(usint) - 1) + (((message.length() + 1) % (sendBufferSize - sizeof(usint) - 1)) == 0 ? 0 : 1);
    for(int i=0; i < messageCount; i++)
    {
        string buffer;
        char sendBuffer[sendBufferSize+4];
        memset(sendBuffer, 0, sendBufferSize+4);
        usint size = 5;
        sendBuffer[0] = (unsigned char)messageTypes::PrivateMessage;
        Converter::itoa(clientID, &sendBuffer[1]);
        if(i==0)
        {
            buffer = message.substr(0,sendBufferSize - sizeof(size) - 2);
            size += buffer.length() + 1;
            Converter::itoa((usint)(buffer.length() + 1), &sendBuffer[5]);
            sendBuffer[7] = messageCount;
            mempcpy(&sendBuffer[8], buffer.c_str(), buffer.length());
            size += 2;
        }
        else 
        {
            buffer = message.substr(i * (sendBufferSize - sizeof(usint) -1) - 1, (sendBufferSize - sizeof(usint) - 1));
            size += buffer.length();
            Converter::itoa((usint)buffer.length(), &sendBuffer[5]);
            mempcpy(&sendBuffer[7], buffer.c_str(), buffer.length());
            size += 2;
        }
        this->sendQueue.push(string(sendBuffer, size));
    }
}

void Client::parseIncomingMessage(char* message)
{
    messageTypes messageType = (messageTypes)message[0];
    
    switch(messageType)
    {
        case messageTypes::CommonMessage:
        {
            this->parseIncomingCommonMessage(&message[1], false);
            break;
        }
        case messageTypes::PrivateMessage:
        {
            this->parseIncomingCommonMessage(&message[1], true);
            break;
        }
        case messageTypes::SystemMessage:
        {
            this->parseSystemMessage(&message[1]);
            break;
        }
        case messageTypes::FileMessage:
        {
            this->parseFileMessage(&message[1]);
            break;
        }
        default:
            printf("Incoming unknown message with type = %d\n", (int)messageType);
            break;
    }
}

void Client::parseFileMessage(char* message)
{
    if(this->recvFile == NULL) return;
    int fromID = Converter::atoi(message);
    usint bytesCount = Converter::atosi(&message[4]);
    fwrite(&message[6], 1, bytesCount, this->recvFile);
    if(bytesCount != sendFileBufferSize)
    {
        fclose(this->recvFile);
        printf("File downloaded from user: %s\n", this->clientNames[this->recvFileFromUser].c_str());
        this->recvFileFromUser = -1;
        this->recvFile = NULL;
    }
}

void Client::parseIncomingCommonMessage(char* message, bool PM)
{
    int fromID = Converter::atoi(message);
    message = &message[4];
    unsigned short int mesLength = Converter::atosi(message);
    if(this->bufferMessages[fromID].length() != 0)
    {
        if(mesLength != 0)
        {
            this->bufferMessages[fromID]+= &message[2];
        }
        if(mesLength != sendBufferSize - sizeof(mesLength) - 1)
        {
            if(PM) printf("PM from ");
            printf("%s>: %s\n", this->clientNames[fromID].c_str(),this->bufferMessages[fromID].c_str());
            this->bufferMessages[fromID].clear();
        }
        return;
    }
    
    mesLength--;
    unsigned char messageCount = (unsigned char)message[2];
    if(messageCount > 1)
    {
        this->bufferMessages[fromID] = string(&message[3]);
    }
    else
    {
        if(PM) printf("PM from ");
        printf("%s>: %s\n", this->clientNames[fromID].c_str(),&message[3]);
    }
    
}

void Client::parseSystemMessage(char *message)
{
    SystemCodes code = (SystemCodes)message[0];
    message = &message[1];
    switch(code)
    {
        case SystemCodes::Connect:
        {
            int fromID = Converter::atoi(message);
            this->clientNames[fromID] = &message[4];
            printf("New user connected id = %d, name = %s\n", fromID, this->clientNames[fromID].c_str());
            break;
        }
        case SystemCodes::GetUserList:
        {
            int fromID = Converter::atoi(message);
            this->clientNames[fromID] = &message[4];
            printf("User id = %d, name = %s\n", fromID, this->clientNames[fromID].c_str());
            break;
        }
        case SystemCodes::Disconnect:
        {
            int fromID = Converter::atoi(message);
            printf("User with id = %d, name = %s was disconnected!\n", fromID, this->clientNames[fromID].c_str());
            map<int, string>::iterator it = this->clientNames.find(fromID);
            this->clientNames.erase(it);
            break;
        }
        case SystemCodes::SendFile:
        {
            int fromID = Converter::atoi(message);
            usint bytesCount = Converter::atosi(&message[4]);
            if(bytesCount > 0)
            {
                this->lockFd(STDIN_FILENO, true);
                if(this->file == NULL && this->recvFile == NULL)
                {
                    printf("%s want to send to you file with name %s\n", this->clientNames[fromID].c_str(), &message[6]);
                    printf("Do you agree?:");
                    string answer;
                    cin >> answer;
                    if(answer != "yes")
                    {
                        char buffer[6];
                        buffer[0] = (unsigned char)messageTypes::SystemMessage;
                        buffer[1] = (unsigned char)SystemCodes::Error;
                        Converter::itoa(fromID, &buffer[2]);
                        this->sendQueue.push(string(buffer, 6));
                    }
                    else
                    {
                        char buffer[6];
                        buffer[0] = (unsigned char)messageTypes::SystemMessage;
                        buffer[1] = (unsigned char)SystemCodes::SendFile;
                        Converter::itoa(fromID, &buffer[2]);
                        this->sendQueue.push(string(buffer, 6));
                        this->recvFile = fopen(&message[6], "wb");
                        this->recvFileFromUser = fromID;
                    }
                }
                else 
                {
                    if(this->file!= NULL)
                        printf("%s try to send to you file, but now you sending another file\n", this->clientNames[fromID].c_str());
                    else
                    {
                        printf("%s try to send to you file, but now you recieving another file\n", this->clientNames[fromID].c_str());
                    }
                    char buffer[6];
                    buffer[0] = (unsigned char)messageTypes::SystemMessage;
                    buffer[1] = (unsigned char)SystemCodes::Error;
                    Converter::itoa(fromID, &buffer[2]);
                    this->sendQueue.push(string(buffer, 6));
                }
                this->lockFd(STDIN_FILENO, false);
            }
            else
            {
                char buffer[sendFileBufferSize];
                usint bytesCount = fread(buffer, 1, sendFileBufferSize, this->file);
                char header[7];
                header[0] = (unsigned char)messageTypes::FileMessage;
                Converter::itoa(fromID, &header[1]);
                Converter::itoa(bytesCount, &header[5]);
                this->sendQueue.push(string(header,7) + string(buffer, bytesCount));
            }
            
            break;
        }
        case SystemCodes::Error:
        {
            if(this->file!=NULL)
            {
                printf("Error! We can't send file to %s. Please, try again!\n", this->clientNames[this->sendFileToUser].c_str());
                fclose(this->file);
                this->file = NULL;
                this->sendFileToUser = -1;
            }
            else if(this->recvFile != NULL)
            {
                printf("We have some troubles with recieving file from %s. Please, try again!\n", this->clientNames[this->recvFileFromUser].c_str());
                fclose(this->recvFile);
                this->recvFile = NULL;
                this->recvFileFromUser = -1;
            }
            break;
        }
        default:
            printf("Incoming unknown system code = %d\n", (int)code);
            break;
    }
}

bool Client::checkMessageOnSystem(string message)
{
    if(message[0] != '/') 
    {
        return false;
    }
    int param=0;
    switch(this->getSystemCodeFromSystemMessage(message, param))
    {
        case SystemCodes::GetUserList:
        {
            printf("\nOnline users:\n");
            char buffer[2];
            buffer[0] = (unsigned char)messageTypes::SystemMessage;
            buffer[1] = (unsigned char)SystemCodes::GetUserList;
            this->sendQueue.push(string(buffer,2));
            break;
        }
        case SystemCodes::PrivateMessage:
        {
            this->lockFd(STDIN_FILENO, true);
            
            printf("Set UserName: ");
            string username = "Test1";
            cin >> username;
            int toID = -1;
            for(map<int, string>::iterator it = this->clientNames.begin(); it != this->clientNames.end(); it++)
            {
                if(it->second == username)
                {
                    toID = it->first;
                    break;
                }
            }
            if(toID == -1)
            {
                printf("Not found user with name = %s\n", username.c_str());
                this->lockFd(STDIN_FILENO, false);
                return true;
            }
            printf("PM to %s>: ", username.c_str());
            this->lockFd(STDIN_FILENO, true);
            string str;
            std::cin.clear();
            getline(std::cin, str);
            getline(std::cin, str);
            this->sendPrivateMessage(toID, str);
            this->lockFd(STDIN_FILENO, false);
            break;
        }
        case SystemCodes::SendFile:
        {
            this->lockFd(STDIN_FILENO, true);
            if(this->file != NULL)
            {
                printf("Now you are sending a file. Do you want cancel it?:\n");
                string ans;
                cin >> ans;
                if(ans != "yes")
                {
                    this->lockFd(STDIN_FILENO, false);
                    return true;
                }
                else
                    fclose(this->file);
            }
            
            printf("Set UserName: ");
            string username = "Test1";
            cin >> username;
            int toID = -1;
            for(map<int, string>::iterator it = this->clientNames.begin(); it != this->clientNames.end(); it++)
            {
                if(it->second == username)
                {
                    toID = it->first;
                    break;
                }
            }
            if(toID == -1)
            {
                printf("Not found user with name = %s\n", username.c_str());
                this->lockFd(STDIN_FILENO, false);
                return true;
            }
            printf("Set filename: ");
            string filename;
            cin >> filename;
            this->file = fopen(filename.c_str(), "rb");
            this->lockFd(STDIN_FILENO, false);
            if(this->file == NULL)
            {
                printf("Can't found file\n");
                return true;
            }
            char buffer[8];
            buffer[0] = (unsigned char)messageTypes::SystemMessage;
            buffer[1] = (unsigned char)SystemCodes::SendFile;
            Converter::itoa(toID, &buffer[2]);
            Converter::itoa((usint)filename.length(), &buffer[6]);
            this->sendQueue.push(string(buffer,8) + filename);
            this->sendFileToUser = toID;
            break;
        }
        default:
            return false;
    }
    return true;
}

SystemCodes Client::getSystemCodeFromSystemMessage(string message, int &additional_parameter)
{
    if(message.find("user_list") != string::npos) return SystemCodes::GetUserList;
    if(message.find("pm") != string::npos) return SystemCodes::PrivateMessage;
    if(message.find("send_file") != string::npos) return SystemCodes::SendFile;
    printf("SystemMessage not found!\n");
}

bool Client::sendFile(string message)
{
    if(message[0] == (unsigned char)messageTypes::FileMessage)
    {
        char *messageBuffer = (char*)message.c_str();
        usint bytesCount = Converter::atosi(&messageBuffer[5]);
        if(bytesCount == sendFileBufferSize)
        {
            char buffer[sendFileBufferSize];
            bytesCount = fread(buffer, 1, sendFileBufferSize, this->file);
            char header[7];
            header[0] = (unsigned char)messageTypes::FileMessage;
            Converter::itoa(this->sendFileToUser, &header[1]);
            Converter::itoa(bytesCount, &header[5]);
            this->sendQueue.push(string(header,7) + string(buffer, bytesCount));
        }
        else
        {
            printf("File was upload to user: %s!\n", this->clientNames[this->sendFileToUser].c_str());
            fclose(this->file);
            this->sendFileToUser = -1;
            this->file = NULL;
        }
    }
    return true;
}

int Client::lockFd(int fd, int blocking)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}