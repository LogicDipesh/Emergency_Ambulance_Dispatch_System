#ifndef HEAP_H
#define HEAP_H


typedef struct MinHeap MinHeap;

MinHeap *heap_create(int capacity);
void     heap_destroy(MinHeap *h);
int      heap_is_empty(const MinHeap *h);
int      heap_size(const MinHeap *h);
void     heap_insert(MinHeap *h, int key, int value);
int      heap_contains(const MinHeap *h, int value);
void     heap_decrease_key(MinHeap *h, int value, int new_key);
int      heap_extract_min(MinHeap *h, int *key_out, int *value_out);
int      heap_snapshot(const MinHeap *h, int *keys_out, int *vals_out, int max);

#endif
