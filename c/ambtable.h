#ifndef AMBTABLE_H
#define AMBTABLE_H

#define AMB_FREE       0
#define AMB_TO_PATIENT 1
#define AMB_TO_HOSPITAL 2

typedef struct Ambulance {
    int id;
    int node;              
    int status;
    int *path;             
    int  path_len;
    int  seg;              
    double seg_travelled;  
    int patient_node;
    int hospital_node;
    int emg_id;
    int emg_severity;
    struct Ambulance *next; 
} Ambulance;

typedef struct AmbTable AmbTable;

AmbTable  *ambtable_create(int buckets);
void       ambtable_destroy(AmbTable *t);
Ambulance *ambtable_add(AmbTable *t, int id, int node); 
Ambulance *ambtable_get(AmbTable *t, int id);
int        ambtable_remove(AmbTable *t, int id); 
int        ambtable_count(const AmbTable *t);
void       ambtable_foreach(AmbTable *t, void (*fn)(Ambulance *, void *), void *ctx);

#endif
