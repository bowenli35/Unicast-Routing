
#ifndef Dijkstra_h
#define Dijkstra_h

#define MAX 256
#define INF 1000000000

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern int neighborList[256];

struct graph {
    int size;
    int *nodes;
    int **adjacency;
};

typedef struct graph *Graph;

Graph initGraph() {
    Graph graph = (Graph) calloc (1, sizeof(struct graph));
    graph -> size = 0;
    graph -> nodes = (int*) calloc (MAX, sizeof(int));
    graph -> adjacency = (int**) calloc (MAX, sizeof(int*));
    for (int i = 0; i < MAX; i++) {
        graph -> adjacency[i] = (int*) calloc (MAX, sizeof(int));
        for (int j = 0; j < MAX; j++) {
            graph -> adjacency[i][j] = 0;
        }
    }
    return graph;
}

void delGraph(Graph graph) {
    for (int i = 0; i < MAX; i++) {
        free(graph -> adjacency[i]);
    }
    free(graph -> adjacency);
    free(graph -> nodes);
    free(graph);
}

void addNode(Graph graph, int idx) {
    for (int i = 0; i < graph -> size; i++) {
        if (graph -> nodes[i] == idx) return; // already in the nodes.
    }
    graph -> nodes[graph -> size++] = idx;
}

void addEdge(Graph graph, int a, int b, int cost) {
    graph -> adjacency[a][b] = cost;
    graph -> adjacency[b][a] = cost;
}

int findNodeIdx(Graph graph, int idx) {
    for (int i = 0; i < graph -> size; i++) {
        if (graph -> nodes[i] == idx) return i;
    }
    printf("node idx not found! idx = %d\n", idx);
    return -1;
}

void printNodes(Graph graph) {
    for (int i = 0; i < graph -> size; i++) {
        fprintf(stdout, "%d ", graph -> nodes[i]);
    }
    fprintf(stdout, "\n");
}

void dijkstra(Graph graph, int src, int *predecessor) {
    int distance[graph -> size];
    int frontier[graph -> size];
    for (int i = 0; i < graph -> size; i++) {
        distance[i] = INF;
        frontier[i] = graph -> nodes[i];
    }
    distance[findNodeIdx(graph, src)] = 0;
    int cnt = graph -> size;
    while (cnt != 0) {
        int idx = 0; // the index of the node in frontier which have min distance to src
        int minDistance = INF; // the min distance to src
        for (int i = 0; i < graph -> size; i++) {
            // find the node which has the min distance to src
            if (frontier[i] != -1 && distance[i] < minDistance) {
                minDistance = distance[i];
                idx = i;
            }
        }

        int node = graph -> nodes[idx]; // the id of the node
        for (int i = 0; i < MAX; i++) {
            // find all neighbors of the node
            if (graph -> adjacency[node][i] != 0) {
                // one of neighbor
                int newDistance = minDistance + graph -> adjacency[node][i];
                int neighbor = findNodeIdx(graph, i); // the index of the neighor in nodes
                if (newDistance < distance[neighbor]) {
                    distance[neighbor] = newDistance;
                    predecessor[i] = node;
                } else if (newDistance == distance[neighbor]) {
                    int currHop = node;
                    int nextHop = predecessor[currHop];
                    while (nextHop != src) {
                        currHop = nextHop;
                        nextHop = predecessor[currHop];
                    }
                    int ans1 = currHop;
                    currHop = i;
                    nextHop = predecessor[currHop];
                    while (nextHop != src) {
                        currHop = nextHop;
                        nextHop = predecessor[currHop];
                    }
                    int ans2 = currHop;
                    if (ans1 < ans2) {
                        predecessor[i] = node;
                    }
                }
            }
        }
        frontier[idx] = -1; // marked explored.
        cnt--;
    }
}

#endif /* Dijkstra_h */
