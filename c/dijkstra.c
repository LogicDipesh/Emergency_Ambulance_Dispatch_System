#include "dijkstra.h"
#include "heap.h"
#include <float.h>
#include <stdlib.h>
#define INF_HALF 1000000000

void dijkstra(const Graph *g, int src, double *dist, int *prev)
{
    int V = g->V;
    int *d2 = (int *)malloc(sizeof(int) * V);
    MinHeap *h = heap_create(V);

    for (int v = 0; v < V; v++) {
        d2[v] = INF_HALF;
        prev[v] = -1;
        heap_insert(h, INF_HALF, v);
    }
    d2[src] = 0;
    heap_decrease_key(h, src, 0);

    int key, u;
    while (heap_extract_min(h, &key, &u)) {
        if (key >= INF_HALF) break;             
        for (Edge *e = g->adj[u]; e; e = e->next) {
            int nd = d2[u] + (int)(e->w * 2.0 + 0.5);
            if (nd < d2[e->to]) {
                d2[e->to] = nd;
                prev[e->to] = u;
                heap_decrease_key(h, e->to, nd);
            }
        }
    }

    for (int v = 0; v < V; v++)
        dist[v] = (d2[v] >= INF_HALF) ? DBL_MAX : d2[v] / 2.0;

    free(d2);
    heap_destroy(h);
}
