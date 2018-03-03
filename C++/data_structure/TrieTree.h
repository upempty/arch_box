#ifndef _TrieTree_h_
#define _TrieTree_h_

#include <string>
#include <iostream>

class TrieNode
{
private:
    char _value;
    bool _is_word;

public:
   static const int NUMBER = 26;
   TrieNode *branch[NUMBER];

   TrieNode(char c)
   {
       this->set_value(c);
       this->set_is_word(false);
       for (int i = 0; i < TrieNode::NUMBER; i++)
       {
           this->branch[i] = NULL;
       }
   }


   ~TrieNode()
   {
   };

   void set_value(char value)
   {
       this->_value = value;
   };

   char get_value(void)
   {
       return this->_value;
   };

   bool is_word(void)
   {
        return this->_is_word;
   };

   void set_is_word(bool is_word)
   {
        this->_is_word = is_word;
   };
};

class TrieTree
{
private:
    TrieNode *root;

public:
    TrieTree()
    {
        root = new TrieNode(' ');
        for (int i = 0; i < TrieNode::NUMBER; i++)
        {
            root->branch[i] = NULL;
        }
    };

    ~TrieTree() {};

    bool search_word(const std::string words)
    {
        if ((root == NULL) || 
            (words.empty()) ||
            (words.length() == 0))
            return false;

        TrieNode *p = root;
        for (auto const c : words)
        {
            int index;
            if ((c >= 'a') && (c <= 'z'))
                index = c - 'a';
            else if ((c >= 'A') && (c <= 'Z'))
                index = c - 'A';
            else
                return false;

            if (p->branch[index] == NULL)
            {
                return false;
            } 
            p = p->branch[index]; 
        }
 
        if (p->is_word())
        {
            return true;
        }
       return false; 
    };

    void insert_word(const std::string words)
    {
        TrieNode *p = root;
        for (int i = 0; i < words.length(); i++)
        {
            int index;
            char c = words[i];
            if ((c >= 'a') && (c <= 'z'))
                index = c - 'a';
            else if ((c >= 'A') && (c <= 'Z'))
                index = c - 'A';
            else
                return;
            if (p->branch[index] == NULL)
            {
                p->branch[index] = new TrieNode(c);
            }
            p = p->branch[index];
        }
        p->set_is_word(true); 
    };
};

#endif
