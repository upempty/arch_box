#include <iostream>
#include "Dijkstra.h"

int main(int argc, char *argv[])
{
    Graph graph(5);
    graph.addEdge(0, 1, 1);
    graph.addEdge(0, 2, 21);
    graph.addEdge(0, 3, 33);
    graph.addEdge(0, 4, 2);
    graph.addEdge(4, 3, 33);
    graph.addEdge(1, 2, 19);

    Dijkstra dijkstra(&graph);
    std::cout<<"Distances from 0"<<std::endl;
    dijkstra.dijkstraPath(0);
    dijkstra.print();
    return 0;
}
