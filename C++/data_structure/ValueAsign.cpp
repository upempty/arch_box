#include <iostream>
class ValueAsign
{
public:
    ValueAsign() {}
    ~ValueAsign() {};

    void passByValue(int *p)
    {
        //Allocate memory for int and store the address in p
        p = new int;
    }

    void passByRef(int *&p)
    {
        p = new int;
    }
};

int main(int argc, char *argv[])
{
    ValueAsign c;
    
    int* p1 = NULL;
    int* p2 = NULL;
    std::cout<<"init p1,p2 = "<<p1<<" "<<p2<<std::endl;

    //p1 will still be NULL
    c.passByValue(p1);

    //p2 is changed to point to the newly allocate memory
    c.passByRef(p2);
    std::cout<<"after passing, p1,p2 = "<<p1<<", "<<p2<<std::endl;

    return 0;
}
