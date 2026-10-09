#ifndef GRAPH_H
#define GRAPH_H

typedef struct Edge {
    int          to;
    double       w;
    struct Edge *next;
} Edge;

typedef struct {
    int    V;
    Edge **adj;
} Graph;

Graph *graph_create(int V);
void   graph_add_edge(Graph *g, int u, int v, double w); 
void   graph_destroy(Graph *g);

#endif
