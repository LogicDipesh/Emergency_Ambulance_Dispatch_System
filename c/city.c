#include "city.h"

static const CityNode NODES[CITY_NODES] = {
    { "Govt Doon Medical College Hospital", 1 },  
    { "Max Super Speciality Hospital, Rajpur", 1 },
    { "Graphic Era Institute of Medical Sciences", 1 }, 
    { "Dehradun Railway Station", 0 },          
    { "ISBT Dehradun", 0 },                      
    { "Clock Tower (Ghanta Ghar)", 0 },          
    { "Paltan Bazaar", 0 },                      
    { "Forest Research Institute (FRI)", 0 },    
    { "Robber's Cave (Guchhupani)", 0 },         
    { "Tapkeshwar Temple", 0 },                  
    { "Sahastradhara", 0 },                     
    { "Rajpur", 0 },                             
    { "Clement Town", 0 },                       
    { "Prem Nagar", 0 },                         
    { "Raipur", 0 },                             
};

static const struct { int a, b; double km; } ROADS[CITY_EDGES] = {
    { 3,  5, 2.5 }, { 5,  6, 1.0 }, { 0,  5, 1.5 }, { 0,  6, 1.0 },
    { 3,  4, 5.5 }, { 4,  5, 7.0 }, { 4, 12, 4.5 }, { 5, 11, 4.5 },
    { 11, 1, 2.0 }, { 5, 13, 4.5 }, { 13, 7, 3.0 }, { 7,  2, 3.5 },
    { 7,  5, 7.0 }, { 3,  7, 6.5 }, { 7,  9, 4.0 }, { 3,  9, 8.0 },
    { 9,  8, 5.0 }, { 5,  8, 8.0 }, { 13, 8, 4.0 }, { 8,  2, 6.0 },
    { 5, 14, 7.0 }, { 14,10, 8.0 }, { 12,14, 5.0 }, { 12, 5, 5.0 },
    { 1, 13, 4.0 }, { 0,  3, 2.0 }, { 9,  2, 5.5 }, { 6,  3, 2.0 },
};

const CityNode *city_nodes(void) { return NODES; }

void city_build_roads(Graph *g)
{
    if (!g || g->V != CITY_NODES) return;
    for (int i = 0; i < CITY_EDGES; i++)
        graph_add_edge(g, ROADS[i].a, ROADS[i].b, ROADS[i].km);
}
