/* reverse words
I am Fred->
Fred am I
*/
#include <iostream>
#include <string>

#define SPACE ' '

class Words
{
public:
    Words() 
    { 
    }
    
    ~Words() {}

    void asign(const char *p)
    {
        words = p;
    }

    void reverseWordsBySpace(void)
    {
       char *p = &words[0];
       for (int i = 0, j = 0; i< words.length(); i++)
       {
           if (words[i] == ' ')
           {
               reverseWord(p, j, i-1);
               j = i + 1;
           }
           else if (i == (words.length() - 1))
           {
               reverseWord(p, j, i); 
           }
       } 
       reverseWord(p, 0, words.length() -1);
    }

    void print(void)
    {
        std::cout<<words<<std::endl;
    }  

private:    
    void reverseWord(char *str, int i, int j)
    {
        while(i < j)
        {
            char tmp = str[j];
            str[j] = str[i];
            str[i] = tmp;
            i++;
            j--; 
        }
    }

    std::string words;
};

int main(int argc, char *argv[])
{
    Words word;
    const char * p = "abc 22a 33c 44d";
    word.asign(p);

        //words = "abc efg 123 jjk";
    word.print();
    word.reverseWordsBySpace();
    word.print();
    return 0;
}

