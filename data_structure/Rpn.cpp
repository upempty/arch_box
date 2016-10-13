/* Reverse Polish Notation 
expression:  "3 9 - 6 * 6 /"
result: -6
*/
#include<vector>
#include<string>
#include<sstream>
#include<iostream>

const static std::string OPERATORS = "+-*/";

class Rpn
{
public:
  Rpn()
  {

  }

  ~Rpn()
  {

  }

  int calc(std::string exp)
  {
    std::vector<std::string> tokens = extractExp(exp); 
    for (auto s : tokens)
    {
      if (OPERATORS.find(s) == std::string::npos)
      {
        operands.push_back(s);       
      }
      else
      {
        int a = std::stoi(operands.back()); 
        operands.pop_back();
        int b = std::stoi(operands.back());
        operands.pop_back();
        int res;
        switch(s[0])
        {
        case '+':
          res = a + b;
          operands.push_back(std::to_string(res)); 
          break;
        case '-':
          res = b - a;
          operands.push_back(std::to_string(res)); 
          break;
        case '*':
          res = a * b;
          operands.push_back(std::to_string(res)); 
          break;
        case '/':
          res = b / a;
          operands.push_back(std::to_string(res)); 
          break;
 
        }
      }
    }
    int ret = std::stoi(operands.back());
    operands.pop_back();
    return ret;
  }

private:
  std::vector<std::string> extractExp(std::string exp)
  { 
    std::vector<std::string> tokens;
    std::string word;
    std::stringstream ss(exp);
    while(ss >> word)
    {
      tokens.push_back(word);
    } 
    return tokens;
  }  

  std::vector<std::string> operands;

};

int main(int argc, char *argv[])
{

//#define EXPRESSION "3 5 - 6 *"
  const char *EXPRESSION = "3 9 - 6 * 6 /";
  Rpn rpn;
  int ret = rpn.calc(EXPRESSION);
  std::cout<<"result = "<<ret<<std::endl;
  return 0;
}
