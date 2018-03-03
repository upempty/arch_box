#include <iostream>
#include "RingBuffer.h"

int main(int argc, char *argv[])
{
    CircleQueue q;
    q.push('a');
    q.push('b'); 
    char a = q.pop();
    char b = q.pop();
    std::cout<<"a b = "<<a<<" "<<b<<" "<<q.pop()<<std::endl;
    return 0;
}
