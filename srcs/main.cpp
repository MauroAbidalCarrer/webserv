// #include "Server.hpp"
#include <iostream>
#include <map>
#include <string>
#include "SmartPointer42.hpp"

int main()
{
    SmartPointer42<std::string> test(new std::string("salut ca va"));

    std::cout << *test << std::endl;
    test->append("| appeneded text");
    std::cout << *test << std::endl;
}   
