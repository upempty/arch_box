#include "Graph.h"

class Dijkstra 
{
private:
    std::map<vertex_t, EdgeList> _graph;
    int _vertex_num;

    std::map<vertex_t, int> _distances; 
    std::list<vertex_t> unvisited_vertices;
public:
    Dijkstra(Graph *graph)
    {
        _graph = graph->get_graph();
        _vertex_num = graph->get_vertex();
        initDistances();
    };

    ~Dijkstra()
    {
     
    };
    void initDistances(void)
    {
        for (int i = 0; i < _vertex_num; i++)
        {
           _distances[i] = -1;
        }
    };

    void initUnvisitVertices(vertex_t src)
    {  
        for (int i = 0; i < _vertex_num; i++)
        {
            //make sure that src is at the front of the list
            if (i == src)
                unvisited_vertices.push_front(i);
            else
                unvisited_vertices.push_back(i);
        } 
    };

    vertex_t popVertexWithDistance(void)
    {
       for(std::list<vertex_t>::iterator it = unvisited_vertices.begin(); 
           it != unvisited_vertices.end(); it++) 
       {
           std::map<vertex_t, int>::iterator it2;
           it2 = _distances.find(*it); 
           if ((it2 !=_distances.end()) && ((it2->second) != -1))
           {
               unvisited_vertices.erase(it);
               return (*it);
           }
       }
       return -1;
    };

    void dijkstraPath(vertex_t src)
    {
        _distances[src] = 0;
        initUnvisitVertices(src);

        while (!unvisited_vertices.empty())
        {
            vertex_t u = popVertexWithDistance();
            if (u == -1)
                return;

            struct Edge *p = _graph[u].head;
            while (p != NULL)// Traversaling edges for u
            {
                vertex_t v = p->dest;
                if (((_distances[u] + p->weight) < _distances[v])||
                    (_distances[v] == -1)) {
                    _distances[v] = _distances[u] + p->weight;
                }
                p = p->next;
            }
        }
        
    }; 

    void print(void)
    {
        std::cout<<"Distances from 0"<<std::endl;
        for(int i = 0; i < _vertex_num; i++)
        {
            std::cout<<_distances[i]<<std::endl;
        }
    };
    
};
