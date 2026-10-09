#include "heap.h"
#include <stdlib.h>

struct MinHeap {
    int *keys;
    int *vals;
    int *pos;      
    int  size;
    int  capacity;
};

MinHeap *heap_create(int capacity)
{
    MinHeap *h = (MinHeap *)malloc(sizeof *h);
    if (!h) return NULL;
    h->keys = (int *)malloc(sizeof(int) * capacity);
    h->vals = (int *)malloc(sizeof(int) * capacity);
    h->pos  = (int *)malloc(sizeof(int) * capacity);
    if (!h->keys || !h->vals || !h->pos) { heap_destroy(h); return NULL; }
    for (int i = 0; i < capacity; i++) h->pos[i] = -1;
    h->size = 0;
    h->capacity = capacity;
    return h;
}

void heap_destroy(MinHeap *h)
{
    if (!h) return;
    free(h->keys);
    free(h->vals);
    free(h->pos);
    free(h);
}

int heap_is_empty(const MinHeap *h) { return h->size == 0; }
int heap_size(const MinHeap *h)     { return h->size; }

static void swap_at(MinHeap *h, int i, int j)
{
    int tk = h->keys[i]; h->keys[i] = h->keys[j]; h->keys[j] = tk;
    int tv = h->vals[i]; h->vals[i] = h->vals[j]; h->vals[j] = tv;
    h->pos[h->vals[i]] = i;
    h->pos[h->vals[j]] = j;
}

static void sift_up(MinHeap *h, int i)
{
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->keys[i] < h->keys[p]) { swap_at(h, i, p); i = p; }
        else break;
    }
}

static void sift_down(MinHeap *h, int i)
{
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, m = i;
        if (l < h->size && h->keys[l] < h->keys[m]) m = l;
        if (r < h->size && h->keys[r] < h->keys[m]) m = r;
        if (m == i) break;
        swap_at(h, i, m);
        i = m;
    }
}

void heap_insert(MinHeap *h, int key, int value)
{
    if (h->size >= h->capacity || value < 0 || value >= h->capacity) return;
    if (h->pos[value] != -1) return;              
    int i = h->size++;
    h->keys[i] = key;
    h->vals[i] = value;
    h->pos[value] = i;
    sift_up(h, i);
}

int heap_contains(const MinHeap *h, int value)
{
    return value >= 0 && value < h->capacity && h->pos[value] != -1;
}

void heap_decrease_key(MinHeap *h, int value, int new_key)
{
    if (value < 0 || value >= h->capacity) return;
    int i = h->pos[value];
    if (i == -1 || new_key >= h->keys[i]) return;
    h->keys[i] = new_key;
    sift_up(h, i);
}

int heap_extract_min(MinHeap *h, int *key_out, int *value_out)
{
    if (h->size == 0) return 0;
    if (key_out)   *key_out   = h->keys[0];
    if (value_out) *value_out = h->vals[0];
    h->pos[h->vals[0]] = -1;
    h->size--;
    if (h->size > 0) {
        h->keys[0] = h->keys[h->size];
        h->vals[0] = h->vals[h->size];
        h->pos[h->vals[0]] = 0;
        sift_down(h, 0);
    }
    return 1;
}

int heap_snapshot(const MinHeap *h, int *keys_out, int *vals_out, int max)
{
    int n = h->size < max ? h->size : max;
    for (int i = 0; i < n; i++) {
        keys_out[i] = h->keys[i];
        vals_out[i] = h->vals[i];
    }
    return n;
}
