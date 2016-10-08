#define ALPHABET_SIZE 256
typedef struct Node 
{
    bool is_end;
    char ch;
    Node *next[ALPHABET_SIZE];
} Node;

class Trie
{
public:
    Trie() 
    { 
        initialization();
    };
    ~Trie() 
    {
    };
    void initialization()
    {
        head = createNode(' ');
    };
    int search(const char str[], int len)
    {
        Node *p = head;
        for (int i = 0; i < len; i++)
        {
            if (p->next[charToIndex(str[i]) == NULL)
                break;
            p = p->next[charToIndex(str[i]);
        }
        return ((i == len) && (p->is_end == true))
    }

    void insert(const char str[], int len)
    {
        Node *p = head;
        for (int i = 0; i < len; i++)
        {
            int index = charToIndex(str[i]); 
            if (p->next[index] == NULL)
            {
                p->next[index] = createNode(str[i]);
            }
            p = p->next[index];
        }
        p->is_end = true;
    };
private:
   
    Node *createNode(char c)
    {
        Node *node = new Node();
        node->ch = c;
        node->is_end = false;
        for (int i = 0; i < ALPHABET_SIZE; i++)
        {
            node->next[i] = NULL;
        }
        return node;
    };
    int charToIndex(char c)
    {
        return int(c);
    }
    Node *head;
};
