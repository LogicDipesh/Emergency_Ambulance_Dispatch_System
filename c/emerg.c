#include "emerg.h"
#include "heap.h"
#include <stdlib.h>

struct EmergPQ {
    MinHeap *h;
    int *nodes;       
    int *sevs;
    int  capacity;
    int  next_seq;   
};

EmergPQ *emq_create(int capacity)
{
    EmergPQ *q = (EmergPQ *)malloc(sizeof *q);
    if (!q) return NULL;
    q->h = heap_create(capacity);
    q->nodes = (int *)malloc(sizeof(int) * capacity);
    q->sevs  = (int *)malloc(sizeof(int) * capacity);
    if (!q->h || !q->nodes || !q->sevs) { emq_destroy(q); return NULL; }
    q->capacity = capacity;
    q->next_seq = 1;   
    return q;
}

void emq_destroy(EmergPQ *q)
{
    if (!q) return;
    heap_destroy(q->h);
    free(q->nodes);
    free(q->sevs);
    free(q);
}

int emq_size(const EmergPQ *q) { return heap_size(q->h); }

int emq_push(EmergPQ *q, int node, int severity)
{
    if (severity < 1 || severity > 3) return -1;
    if (q->next_seq >= q->capacity) return -1;
    int id = q->next_seq++;
    q->nodes[id] = node;
    q->sevs[id]  = severity;
    /* key: severity dominates, arrival order breaks ties */
    heap_insert(q->h, severity * 1000000 + id, id);
    return id;
}

static Emergency fill(const EmergPQ *q, int id)
{
    Emergency e;
    e.id = id; e.node = q->nodes[id]; e.severity = q->sevs[id]; e.seq = id;
    return e;
}

int emq_pop(EmergPQ *q, Emergency *out)
{
    int key, id;
    if (!heap_extract_min(q->h, &key, &id)) return 0;
    if (out) *out = fill(q, id);
    return 1;
}

int emq_peek(const EmergPQ *q, Emergency *out)
{
    int keys[1], vals[1];
    if (heap_snapshot(q->h, keys, vals, 1) == 0) return 0;
    if (out) *out = fill(q, vals[0]);
    return 1;
}

int emq_snapshot(const EmergPQ *q, Emergency *out, int max)
{
    int keys[512], vals[512];
    int n = heap_snapshot(q->h, keys, vals, max < 512 ? max : 512);
    for (int i = 0; i < n; i++) out[i] = fill(q, vals[i]);
    return n;
}
