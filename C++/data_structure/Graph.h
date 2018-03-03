#include <cstddef>
#include <map>

typedef int vertex_t;
struct Edge 
{
    vertex_t dest;
    int weight;
    struct Edge *next;
};

struct EdgeList
{
    struct Edge *head;
};

class Graph
{
public:
    Graph(int v)
    {
        initGraph(v);
    };

    ~Graph()
    {
        for (int i = 0; i < _vertex_num; i++)
        {
            Edge *p = _graph[i].head;
            Edge *tmp = p;
            while (p != NULL)
            {
                tmp = p->next;
                delete p;
                p = tmp;
            }
        }
    };

    struct Edge *createEdge(int dest, int weight)
    {
        struct Edge *edge = new Edge();
        edge->dest = dest;
        edge->weight = weight;
        edge->next = NULL;
        return edge;
    };

    void *addEdge(int src, int dest, int weight)
    {
        struct Edge *edge = createEdge(dest, weight);
        edge->next = _graph[src].head;
        _graph[src].head  = edge;
    };

    void initGraph(int v)
    {
        this->_vertex_num = v; 
        for (int i = 0; i < v; i++)
        {
            _graph[i].head = NULL;
        }
    };

    vertex_t get_vertex(void)
    {
        return _vertex_num;
    };

    std::map<vertex_t, EdgeList> get_graph(void)
    {
        return _graph;
    };

private:
    int _vertex_num;
    std::map<vertex_t, EdgeList> _graph;
};
