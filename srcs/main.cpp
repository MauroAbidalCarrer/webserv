#include "Server.hpp"
#include <iostream>
#include <map>
#include <string>
#include "SmartPointer42.hpp"

int main()
{
    try
    {
        Server server;
        server.Run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}   
