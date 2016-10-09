#include <cstdlib>
#include "Client.h"
#include "Server.h"

using namespace std;

int main(int argc, char** argv) 
{
    if(argc > 1 && strcmp("--server", argv[1]) == 0) 
    {
        Server *server = new Server();
        if(server)
        {
            server->start();
            delete server;
        }
        else
        {
            perror("Can't alloc memory for server");
            exit(23);
        }
    }
    else
    {
        Client *client = new Client();
        if(client) 
        {
            client->start();
            delete client;
        }
        else
        {
            perror("Can't alloc memory for client");
            exit(23);
        }
    }
    return 0;
}

