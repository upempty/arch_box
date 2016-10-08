#ifndef _CSTRING_H_
#define _CSTRING_H_

#include <iostream>
#include <cstring>

class CString
{
private:
friend std::ostream & operator << (std::ostream &os, const CString &str);
friend std::istream & operator >> (std::istream &is, CString &str);

public:
    CString();
    ~CString();
    CString(const CString &s);
    CString &operator = (const CString &str);
    CString(const char *p);
    char *get_data(void)
    {
        return data;
    }
private:
    char *data;
    int length;
};

CString::CString() : length(1), data(new char [length])
{
    memcpy(data, "", length); 
}

CString::CString(const CString &str) 
: length(str.length),  data (new char [str.length])
{
    memcpy(data, str.data, str.length); 
}

CString &CString::operator = (const CString &str)
{
    char *tmp = new char [str.length];
    length = str.length;
    memcpy(tmp, str.data, length);
    delete [] data;
    data = tmp;
    return *this;
}

CString::CString(const char *p)
{
    length = strlen(p);
    data = new char [length];
    memcpy(data, p, length);
} 

CString::~CString()
{
    delete [] data;
}

std::ostream & operator << (std::ostream &os, const CString &str)
{
    int i = 0;
    for (i = 0; i < str.length; i++)
        os <<str.data[i];
    return os;
}

std::istream & operator >> (std::istream &is, CString &str)
{
    const int BUF_LEN = 256;
    char buf[BUF_LEN];
    memset(buf, 0, BUF_LEN); 
    is.getline(buf, BUF_LEN, '\n');
    str = buf;
    return is;
}
/*
int main(int argc, char *argv[])
{
    const char *s = "abc";
    CString str(s);
    std::cout<<"str "<<str.get_data()<<std::endl;
    CString a = str;
    std::cout<<"a=str "<<a.get_data()<<std::endl;
    CString b(str);
    std::cout<<"b(str) "<<b.get_data()<<std::endl;
    return 0;
}
*/

#endif
