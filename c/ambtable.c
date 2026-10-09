#include "ambtable.h"
#include <stdlib.h>

struct AmbTable {
    Ambulance **buckets;
    int nbuckets;
    int count;
};

AmbTable *ambtable_create(int buckets)
{
    AmbTable *t = (AmbTable *)malloc(sizeof *t);
    if (!t) return NULL;
    t->buckets = (Ambulance **)calloc(buckets, sizeof(Ambulance *));
    if (!t->buckets) { free(t); return NULL; }
    t->nbuckets = buckets;
    t->count = 0;
    return t;
}

void ambtable_destroy(AmbTable *t)
{
    if (!t) return;
    for (int i = 0; i < t->nbuckets; i++) {
        Ambulance *a = t->buckets[i];
        while (a) {
            Ambulance *n = a->next;
            free(a->path);
            free(a);
            a = n;
        }
    }
    free(t->buckets);
    free(t);
}

static int hash_id(const AmbTable *t, int id)
{
    unsigned u = (unsigned)id;
    u = (u ^ 61) ^ (u >> 16);
    return (int)(u % (unsigned)t->nbuckets);
}

Ambulance *ambtable_add(AmbTable *t, int id, int node)
{
    if (ambtable_get(t, id)) return NULL;
    Ambulance *a = (Ambulance *)calloc(1, sizeof *a);
    if (!a) return NULL;
    a->id = id;
    a->node = node;
    a->status = AMB_FREE;
    int b = hash_id(t, id);
    a->next = t->buckets[b];
    t->buckets[b] = a;
    t->count++;
    return a;
}

Ambulance *ambtable_get(AmbTable *t, int id)
{
    int b = hash_id(t, id);
    for (Ambulance *a = t->buckets[b]; a; a = a->next)
        if (a->id == id) return a;
    return NULL;
}

int ambtable_remove(AmbTable *t, int id)
{
    int b = hash_id(t, id);
    Ambulance *prev = NULL;
    for (Ambulance *a = t->buckets[b]; a; prev = a, a = a->next) {
        if (a->id == id) {
            if (prev) prev->next = a->next;
            else t->buckets[b] = a->next;
            free(a->path);
            free(a);
            t->count--;
            return 1;
        }
    }
    return 0;
}

int ambtable_count(const AmbTable *t) { return t->count; }

void ambtable_foreach(AmbTable *t, void (*fn)(Ambulance *, void *), void *ctx)
{
    for (int i = 0; i < t->nbuckets; i++)
        for (Ambulance *a = t->buckets[i]; a; a = a->next)
            fn(a, ctx);
}
