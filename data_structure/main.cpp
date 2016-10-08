#include "TrieTree.h"
#include <iostream>

int main(int argc, char *argv[])
{
    TrieTree tree;
    std::string words[]= {"ab", "abef", "kkK", "Efg"};
    for (auto w : words)
        tree.insert_word(w);
    bool found = tree.search_word("abef");
    std::cout << "found:"<<found<<std::endl;
}
