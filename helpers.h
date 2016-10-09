#pragma once
#define sendBufferSize 16
#define sendFileBufferSize 16

typedef unsigned short int usint;

enum class messageTypes
{
    SystemMessage,
    CommonMessage,
    PrivateMessage,
    FileMessage,
};

enum class SystemCodes
{
    Disconnect,
    Connect,
    GetUserList,
    Error,
    PrivateMessage,
    SendFile
};