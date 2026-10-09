#include "graph.h"
#include <stdlib.h>

Graph *graph_create(int V)
{
    Graph *g = (Graph *)malloc(sizeof *g);
    if (!g) return NULL;
    g->V = V;
    g->adj = (Edge **)calloc(V, sizeof(Edge *));
    if (!g->adj) { free(g); return NULL; }
    return g;
}

static void add_directed(Graph *g, int u, int v, double w)
{
    Edge *e = (Edge *)malloc(sizeof *e);
    if (!e) return;
    e->to = v;
    e->w = w;
    e->next = g->adj[u];
    g->adj[u] = e;
}

void graph_add_edge(Graph *g, int u, int v, double w)
{
    if (!g || u < 0 || u >= g->V || v < 0 || v >= g->V || u == v) return;
    add_directed(g, u, v, w);
    add_directed(g, v, u, w);
}

void graph_destroy(Graph *g)
{
    if (!g) return;
    for (int i = 0; i < g->V; i++) {
        Edge *e = g->adj[i];
        while (e) { Edge *n = e->next; free(e); e = n; }
    }
    free(g->adj);
    free(g);
}
