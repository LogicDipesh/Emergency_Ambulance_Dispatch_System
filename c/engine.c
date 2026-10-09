#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>

#include "engine.h"
#include "graph.h"
#include "city.h"
#include "dijkstra.h"
#include "emerg.h"
#include "ambtable.h"

#define AMB_SPEED_KMPH 40.0
#define MAX_AMBULANCES 32
#define MAX_EVENTS     32
#define STATE_BUF      65536

static Graph    *G = NULL;
static EmergPQ  *Q = NULL;
static AmbTable *AT = NULL;
static int       next_amb_id = 1;
static double    sim_minutes = 0.0;
static int       completed_trips = 0;

static char state_buf[STATE_BUF];
static char events[MAX_EVENTS][192];
static int  ev_start = 0, ev_count = 0;


static void eventf(const char *fmt, ...)
{
    char *dst = events[(ev_start + ev_count) % MAX_EVENTS];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(dst, 192, fmt, ap);
    va_end(ap);
    if (ev_count < MAX_EVENTS) ev_count++;
    else ev_start = (ev_start + 1) % MAX_EVENTS;
}


static double edge_weight(int u, int v)
{
    for (Edge *e = G->adj[u]; e; e = e->next)
        if (e->to == v) return e->w;
    return -1.0;
}

static int *build_path(int src, int dst, const int *prev, int *len_out)
{
    int tmp[CITY_NODES], n = 0, cur = dst;
    while (cur != -1 && n < CITY_NODES) {
        tmp[n++] = cur;
        if (cur == src) break;
        cur = prev[cur];
    }
    if (n == 0 || tmp[n - 1] != src) { *len_out = 0; return NULL; }
    int *path = (int *)malloc(sizeof(int) * n);
    if (!path) { *len_out = 0; return NULL; }
    for (int i = 0; i < n; i++) path[i] = tmp[n - 1 - i];
    *len_out = n;
    return path;
}

static int nearest_hospital(int node)
{
    double dist[CITY_NODES];
    int    prev[CITY_NODES];
    const CityNode *cn = city_nodes();
    dijkstra(G, node, dist, prev);
    int best = -1;
    double bd = DBL_MAX;
    for (int i = 0; i < CITY_NODES; i++)
        if (cn[i].is_hospital && dist[i] < bd) { bd = dist[i]; best = i; }
    return best;
}

typedef struct {
    Ambulance *best;
    double     bestd;
    int        target;
} NearestCtx;

static void nearest_cb(Ambulance *a, void *ctx)
{
    NearestCtx *c = (NearestCtx *)ctx;
    if (a->status != AMB_FREE) return;
    double dist[CITY_NODES];
    int    prev[CITY_NODES];
    dijkstra(G, a->node, dist, prev);
    double d = dist[c->target];
    if (!c->best || d < c->bestd - 1e-9 ||
        (fabs(d - c->bestd) <= 1e-9 && a->id < c->best->id)) {
        c->best = a;
        c->bestd = d;
    }
}

static Ambulance *nearest_free_ambulance(int target, double *dist_out)
{
    NearestCtx c = { NULL, DBL_MAX, target };
    ambtable_foreach(AT, nearest_cb, &c);
    if (dist_out) *dist_out = c.bestd;
    return c.best;
}


static void start_patient_leg(Ambulance *a, Emergency e, double dist_km);

static void assign_ambulance(Ambulance *a, Emergency e)
{
    double d;
    (void)nearest_free_ambulance(e.node, &d); /* distance for the ETA message */
    double dist[CITY_NODES];
    int    prev[CITY_NODES];
    dijkstra(G, a->node, dist, prev);
    start_patient_leg(a, e, dist[e.node]);
}

static void start_patient_leg(Ambulance *a, Emergency e, double dist_km)
{
    double dist[CITY_NODES];
    int    prev[CITY_NODES];
    dijkstra(G, a->node, dist, prev);
    int len = 0;
    int *path = build_path(a->node, e.node, prev, &len);
    free(a->path);
    a->path = path;
    a->path_len = len;
    a->seg = 0;
    a->seg_travelled = 0.0;
    a->status = AMB_TO_PATIENT;
    a->patient_node = e.node;
    a->hospital_node = -1;
    a->emg_id = e.id;
    a->emg_severity = e.severity;
    eventf("Ambulance %d dispatched to %s (severity %d, ETA %.1f min)",
           a->id, city_nodes()[e.node].name, e.severity,
           dist_km / AMB_SPEED_KMPH * 60.0);
}

static void dispatch_from_queue(void)
{
    for (;;) {
        Emergency e;
        if (!emq_peek(Q, &e)) return;            
        Ambulance *a = nearest_free_ambulance(e.node, NULL);
        if (!a) return;                         
        emq_pop(Q, &e);
        assign_ambulance(a, e);
    }
}


static void complete_leg(Ambulance *a)
{
    if (a->status == AMB_TO_PATIENT) {
        int h = nearest_hospital(a->patient_node);
        double dist[CITY_NODES];
        int    prev[CITY_NODES];
        dijkstra(G, a->patient_node, dist, prev);
        int len = 0;
        int *path = build_path(a->patient_node, h, prev, &len);
        free(a->path);
        a->path = path;
        a->path_len = len;
        a->seg = 0;
        a->seg_travelled = 0.0;
        a->node = a->patient_node;
        a->hospital_node = h;
        a->status = AMB_TO_HOSPITAL;
        eventf("Ambulance %d picked up patient, heading to %s",
               a->id, city_nodes()[h].name);
    } else if (a->status == AMB_TO_HOSPITAL) {
        a->node = a->hospital_node;
        a->status = AMB_FREE;
        free(a->path);
        a->path = NULL;
        a->path_len = 0;
        completed_trips++;
        eventf("Ambulance %d completed trip #%d, free at %s",
               a->id, a->emg_id, city_nodes()[a->node].name);
        dispatch_from_queue();
    }
}

static void advance(Ambulance *a, double km)
{
    while (a->status != AMB_FREE) {
        if (a->seg >= a->path_len - 1) { complete_leg(a); continue; }
        if (km <= 1e-9) break;
        int u = a->path[a->seg], v = a->path[a->seg + 1];
        double w = edge_weight(u, v);
        double remain = w - a->seg_travelled;
        double step = km < remain ? km : remain;
        a->seg_travelled += step;
        km -= step;
        if (a->seg_travelled >= w - 1e-9) { a->seg++; a->seg_travelled = 0.0; }
    }
}

typedef struct { double km; } TickCtx;

static void tick_cb(Ambulance *a, void *ctx)
{
    TickCtx *t = (TickCtx *)ctx;
    if (a->status != AMB_FREE) advance(a, t->km);
}


static void engine_teardown(void)
{
    ambtable_destroy(AT); AT = NULL;
    emq_destroy(Q); Q = NULL;
    graph_destroy(G); G = NULL;
}

void engine_init(void)
{
    engine_teardown();
    G = graph_create(CITY_NODES);
    city_build_roads(G);
    Q = emq_create(512);
    AT = ambtable_create(64);
    next_amb_id = 1;
    sim_minutes = 0.0;
    completed_trips = 0;
    ev_start = 0; ev_count = 0;
    engine_add_ambulance(0);  /* Govt Doon Hospital */
    engine_add_ambulance(4);  /* ISBT */
    engine_add_ambulance(5);  /* Clock Tower */
    eventf("System ready: %d locations, %d roads", CITY_NODES, CITY_EDGES);
}

void engine_reset(void) { engine_init(); }

int engine_add_ambulance(int node)
{
    if (!AT || node < 0 || node >= CITY_NODES) return -1;
    if (ambtable_count(AT) >= MAX_AMBULANCES) return -1;
    int id = next_amb_id++;
    if (!ambtable_add(AT, id, node)) return -1;
    eventf("Ambulance %d stationed at %s", id, city_nodes()[node].name);
    return id;
}

int engine_remove_ambulance(int amb_id)
{
    if (!AT) return 0;
    Ambulance *a = ambtable_get(AT, amb_id);
    if (!a) return 0;
    if (a->status != AMB_FREE) return 0;      
    if (ambtable_count(AT) <= 1) return 0;    
    int node = a->node;
    if (!ambtable_remove(AT, amb_id)) return 0;
    eventf("Ambulance %d removed from %s", amb_id, city_nodes()[node].name);
    return 1;
}

int engine_set_ambulance_node(int amb_id, int node)
{
    if (!AT || node < 0 || node >= CITY_NODES) return 0;
    Ambulance *a = ambtable_get(AT, amb_id);
    if (!a || a->status != AMB_FREE) return 0;   
    a->node = node;
    eventf("Ambulance %d repositioned to %s", amb_id, city_nodes()[node].name);
    return 1;
}

int engine_add_emergency(int node, int severity)
{
    if (!G || node < 0 || node >= CITY_NODES || severity < 1 || severity > 3)
        return -1;
    int id = emq_push(Q, node, severity);
    if (id < 0) return -1;
    eventf("Emergency #%d: severity %d at %s", id, severity,
           city_nodes()[node].name);
    dispatch_from_queue();
    return id;
}

void engine_tick(double sim_minutes_dt)
{
    if (sim_minutes_dt <= 0 || !AT) return;
    sim_minutes += sim_minutes_dt;
    TickCtx t = { AMB_SPEED_KMPH * sim_minutes_dt / 60.0 };
    ambtable_foreach(AT, tick_cb, &t);
}

int engine_ambulance_count(void) { return AT ? ambtable_count(AT) : 0; }
double engine_sim_time(void)     { return sim_minutes; }


typedef struct { char *p; char *end; } Buf;

static void buf_printf(Buf *b, const char *fmt, ...)
{
    if (b->p >= b->end) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(b->p, (size_t)(b->end - b->p), fmt, ap);
    va_end(ap);
    if (n > 0) b->p += n;
}

static void amb_json(Ambulance *a, void *ctx)
{
    Buf *b = (Buf *)ctx;
    int from = a->node, to = a->node;
    double frac = 0.0;
    if (a->status != AMB_FREE && a->path && a->seg < a->path_len - 1) {
        from = a->path[a->seg];
        to = a->path[a->seg + 1];
        double w = edge_weight(from, to);
        if (w > 0) frac = a->seg_travelled / w;
    }
    buf_printf(b,
        "{\"id\":%d,\"status\":%d,\"node\":%d,"
        "\"seg_from\":%d,\"seg_to\":%d,\"seg_frac\":%.3f,"
        "\"emg_id\":%d,\"emg_sev\":%d,\"patient\":%d,\"hospital\":%d,\"route\":[",
        a->id, a->status, a->node, from, to, frac,
        a->emg_id, a->emg_severity, a->patient_node, a->hospital_node);
    if (a->status != AMB_FREE && a->path)
        for (int i = a->seg; i < a->path_len; i++)
            buf_printf(b, "%d,", a->path[i]);
    if (b->p > state_buf && *(b->p - 1) == ',') b->p--;
    buf_printf(b, "]},");
}

const char *engine_get_state(void)
{
    Buf b = { state_buf, state_buf + STATE_BUF - 1 };
    buf_printf(&b, "{\"sim_minutes\":%.1f,\"completed\":%d,\"ambulances\":[",
               sim_minutes, completed_trips);
    if (AT) ambtable_foreach(AT, amb_json, &b);
    if (b.p > state_buf && *(b.p - 1) == ',') b.p--;   /* trim trailing comma */
    buf_printf(&b, "],\"queue\":[");
    Emergency snap[64];
    int nq = emq_snapshot(Q, snap, 64);
    for (int i = 0; i < nq; i++)
        buf_printf(&b, "{\"id\":%d,\"node\":%d,\"severity\":%d},",
                   snap[i].id, snap[i].node, snap[i].severity);
    if (nq > 0) b.p--;
    buf_printf(&b, "],\"events\":[");
    for (int i = 0; i < ev_count; i++) {
        const char *e = events[(ev_start + i) % MAX_EVENTS];
        buf_printf(&b, "\"");
        for (const char *c = e; *c && b.p < b.end - 2; c++) {
            if (*c == '"' || *c == '\\') *b.p++ = '\\';
            *b.p++ = *c;
        }
        buf_printf(&b, "\",");
    }
    if (ev_count > 0) b.p--;
    buf_printf(&b, "]}");
    *b.p = '\0';
    return state_buf;
}
