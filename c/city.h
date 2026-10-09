#ifndef CITY_H
#define CITY_H

#include "graph.h"

#define CITY_NODES 15
#define CITY_EDGES 28

typedef struct {
    const char *name;
    int         is_hospital;
} CityNode;

const CityNode *city_nodes(void);   
void            city_build_roads(Graph *g);  

#endif
