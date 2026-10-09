# Emergency Ambulance Dispatch System — Dehradun

PBL Phase 2 project. A simulated city of 15 Dehradun locations (3 of them
hospitals) connected by 28 weighted roads. Emergency requests are ordered by
medical severity in a priority queue; the closest free ambulance is found with
Dijkstra's shortest path and dispatched; the ambulance drives to the patient,
then to the nearest hospital, drops the patient, and becomes free again at
that hospital.

## Architecture (3 layers, as per the synopsis)

| Layer | Tech | Files |
|---|---|---|
| 1. C99 core engine | adjacency-list graph, hand-rolled binary min-heap with position array + decrease-key, Dijkstra, emergency priority queue (heap), ambulance hash table (separate chaining) | `c/` |
| 2. Python middleware | Flask + ctypes bridge to `dispatch.dll`, background ticker advancing the sim clock | `flask/` |
| 3. Frontend | HTML/CSS/JS, Leaflet map over real Dehradun geography (OpenStreetMap tiles, Karyon-style dark UI): roads drawn along real streets with km labels, hospital badges that stay on top, numbered severity badges for waiting emergencies, status-coloured ambulance badges, orange polylines for active routes | `frontend/` |

## Trip loop

`dispatch → drive to patient → pick up → drive to nearest hospital → drop → free at hospital → next emergency`

Same-severity emergencies are served FIFO. While no ambulance is free, requests
wait in the priority queue. Busy ambulances can neither be repositioned nor
removed from the UI. A free ambulance can be removed with the × button on its
fleet row — except the last remaining one, which the engine refuses to remove
so the fleet can never be emptied by accident.

## Fleet

The simulation starts with 3 ambulances stationed at:

- Govt Doon Medical College Hospital (node 0)
- ISBT Dehradun (node 4)
- Clock Tower (node 5)

More ambulances (up to 32) can be stationed at any node from the dashboard's
Fleet Setup panel.

## Build & run (Windows)

Requirements: MinGW-w64 `gcc` on PATH and Python 3. `gcc` and Python must have
the **same bitness** (e.g. both 64-bit) or Python cannot load the DLL
(`WinError 193`). `build.bat` detects both, adapts with `-m32`/`-m64` when
needed, and verifies at the end that Python can actually load the DLL it just
built.

```bat
build.bat        :: produces dispatch.dll + text_main.exe
launcher.bat     :: starts Flask and opens http://127.0.0.1:5000/
text_main.exe    :: console-only demo (no Python needed)
```

The launcher installs Flask automatically if it is missing. `text_main.exe`
is a scripted console demo: it stations 2 extra ambulances, fires 5
mixed-severity emergencies, runs the clock for 120 simulated minutes, and
prints the engine's JSON state — no Python involved.

The ticker advances **1 simulated minute per real second**; ambulances drive
at 40 km/h in-sim, so a 10 km trip takes ~15 real seconds to watch.

## Real road geometry (free, no API key)

The map draws **real roads**, not straight lines. All 28 edge geometries were
fetched once from the free OSRM public demo server
(`router.project-osrm.org`, no key needed) and baked into
`frontend/road_geometries.json` (124 KB). The app never contacts OSRM at
runtime, so there are no rate limits or keys to manage during the demo — only
the Leaflet library and map tiles load from the internet.

Honest caveat for the viva: the C engine (Dijkstra) still runs on the
synopsis's fixed simulated weights; real road distances differ (e.g. edge
3–5, Railway Station → Clock Tower, is 2.5 km simulated vs 5.15 km by real
road). Hovering any road on the map shows both numbers. Total: 127.5 km
simulated vs 157.7 km real.

## REST API

| Method | Endpoint | Body | Notes |
|---|---|---|---|
| GET | `/api/city` | — | 15 nodes (id, name, lat/lon, hospital flag) + 28 edges |
| GET | `/api/state` | — | sim clock, trips completed, ambulances (incl. live segment + fraction + remaining route), queue, event log (last 32 events) |
| POST | `/api/emergency` | `{"node": 0-14, "severity": 1-3}` | returns the emergency id and the full state |
| POST | `/api/ambulances` | `{"node": 0-14}` | add ambulance at a user-defined position (max 32) |
| DELETE | `/api/ambulances/<id>` | — | remove a *free* ambulance (busy ones and the last ambulance are protected) |
| POST | `/api/ambulances/<id>/position` | `{"node": 0-14}` | reposition a *free* ambulance |
| POST | `/api/reset` | — | reset simulation |

Invalid input (unknown node, severity outside 1–3, unknown or busy
ambulance) is rejected with HTTP 400 and a plain error message; the
simulation state is left untouched.

## Files

```
ambulance-dispatch/
├── build.bat            :: Windows build (MinGW-w64) — dispatch.dll + text_main.exe
├── launcher.bat         :: one-click run (cmd / Explorer)
├── README.md
├── c/
│   ├── heap.h / heap.c          :: binary min-heap + position array (decrease-key)
│   ├── graph.h / graph.c        :: adjacency list
│   ├── city.h / city.c          :: Dehradun data (15 nodes, 28 roads, 3 hospitals)
│   ├── dijkstra.h / dijkstra.c  :: shortest paths via the heap
│   ├── emerg.h / emerg.c        :: emergency priority queue (severity-ordered)
│   ├── ambtable.h / ambtable.c  :: ambulance hash table (separate chaining)
│   ├── engine.h / engine.c      :: dispatch logic, movement, JSON state (DLL API)
│   └── text_main.c              :: console demo driver
├── flask/
│   ├── app.py                   :: ctypes bridge + ticker + REST API
│   └── city_data.py             :: node lat/lon + edges for the frontend
└── frontend/
    ├── index.html
    ├── style.css
    ├── app.js                   :: Leaflet map, polls /api/state every 800 ms, controls
    └── road_geometries.json     :: real street shapes for the 28 roads (from OSRM)
```
