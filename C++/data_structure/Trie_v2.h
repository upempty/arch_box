#ifndef _TRIE_V2_H_
#define _TRIE_V2_H_

#include <vector>

class TrieNode
{
public:
    TrieNode()
    {
        _key = '\0';
        _valid = false;
    }

    ~TrieNode() {};
    char getKey(void)
    {
        return _key;
    }

    void setKey(char k)
    {
        _key = k;
    }

    void setValid(void)
    {
        _valid = true;
    }
    
    bool getValid(void)
    {
        return _valid;
    }

    void appendChild(TrieNode *child)
    {
        _nodes.push_back(child);
    }

    std::vector<TrieNode *> children(void)
    {
        return _nodes;
    }

    TrieNode *findChild(char ch)
    {
        for(int i = 0; i < _nodes.size(); i++)
        {
            TrieNode *tmp = _nodes.at(i);
            if (tmp->getKey() == ch)
            {
                return tmp;
            }
        }   
        return NULL;
    }

private:
    char _key;
    std::vector<TrieNode *> _nodes;
    bool _valid;
};

class TrieTree
{
public:
    TrieTree()
    {
        _root = new TrieNode();
        _nwords = 0;
    }

    ~TrieTree()
    {
        delete _root;
    }

    void addWord(std::string s)
    {
        TrieNode *node = _root;
        if (s.length() == 0)
            return;
        for(int i = 0; i < s.length(); i++)
        {
            TrieNode *child = node->findChild(s[i]);
            if (child != NULL)
            {
                node = child;
            }
            else
            {
                TrieNode *tmp = new TrieNode(); 
                tmp->setKey(s[i]);
                node->appendChild(tmp);
                node = tmp;
            }
            if (i == (s.length() - 1))
            {
                _nwords++;
                node->setValid();
            }
        }
    }

    bool searchWord(std::string s)
    {
        TrieNode *p = _root;
        if (p == NULL)
            return false;

        for(int i = 0; i < s.length(); i++)
        {
            TrieNode *tmp = p->findChild(s[i]);
            if (tmp == NULL)
                return false;
            p = tmp;
        }
        if (p->getValid())
            return true;
        return false;
    }

    void delWord(std::string s);
    unsigned long getNumWords(void)
    {
        return _nwords;
    }
private:
    TrieNode *_root;
    unsigned long _nwords;

};

#endif
