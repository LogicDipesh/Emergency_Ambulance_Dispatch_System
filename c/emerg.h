#ifndef EMERG_H
#define EMERG_H

typedef struct {
    int id;
    int node;
    int severity;
    int seq;
} Emergency;

typedef struct EmergPQ EmergPQ;

EmergPQ *emq_create(int capacity);
void     emq_destroy(EmergPQ *q);
int      emq_size(const EmergPQ *q);
/* returns the new emergency id, or -1 when full / bad input */
int      emq_push(EmergPQ *q, int node, int severity);
/* pops the highest-priority emergency; 1 on success, 0 when empty */
int      emq_pop(EmergPQ *q, Emergency *out);
/* peeks at the highest-priority emergency without removing it */
int      emq_peek(const EmergPQ *q, Emergency *out);
int      emq_snapshot(const EmergPQ *q, Emergency *out, int max);

#endif
