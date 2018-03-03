#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

class LinkedList
{
private:
typedef struct Node
{
    int v;
    Node *next;
} Node;

public:
    LinkedList() 
    {
        head = NULL;
    }

    virtual ~LinkedList()
    {
        while(head != NULL)
        {
            Node *tmp = head->next;
            delete head;
            head = tmp;
        }
    }

    void addValue(int x)
    {
        Node *n = new Node();
        n->v = x;
        n->next = head;
        head = n;
    }
    
    int popValue(void)
    {
        Node *p = head; 
        if (p == NULL)
            return -1;
        head = head->next; 
        int v = p->v;
        delete p;
        return v;
    }
    
    void reverseList(void)
    {
        if ((head == NULL) || (head->next == NULL))
            return;
        Node *p1 = head, *p2 = head->next;
        p1->next = NULL;
        while((p1 != NULL) && (p2 != NULL))
        {
            Node *tmp = p2->next; 
            p2->next = p1;
            p1 = p2;
            p2 = tmp;
        }
        head = p1;
    }

private:
    Node *head;
};

#endif
