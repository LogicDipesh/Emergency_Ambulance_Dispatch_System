
NODES = [
    {"id": 0,  "name": "Govt Doon Medical College Hospital", "lat": 30.32746, "lon": 78.03077, "hospital": True},
    {"id": 1,  "name": "Max Super Speciality Hospital, Rajpur", "lat": 30.37000, "lon": 78.08600, "hospital": True},
    {"id": 2,  "name": "Graphic Era Institute of Medical Sciences", "lat": 30.34800, "lon": 77.99800, "hospital": True},
    {"id": 3,  "name": "Dehradun Railway Station", "lat": 30.29200, "lon": 78.05900, "hospital": False},
    {"id": 4,  "name": "ISBT Dehradun", "lat": 30.28450, "lon": 78.05250, "hospital": False},
    {"id": 5,  "name": "Clock Tower (Ghanta Ghar)", "lat": 30.32635, "lon": 78.03368, "hospital": False},
    {"id": 6,  "name": "Paltan Bazaar", "lat": 30.32350, "lon": 78.03800, "hospital": False},
    {"id": 7,  "name": "Forest Research Institute (FRI)", "lat": 30.34370, "lon": 77.99970, "hospital": False},
    {"id": 8,  "name": "Robber's Cave (Guchhupani)", "lat": 30.37659, "lon": 78.06125, "hospital": False},
    {"id": 9,  "name": "Tapkeshwar Temple", "lat": 30.35617, "lon": 78.01628, "hospital": False},
    {"id": 10, "name": "Sahastradhara", "lat": 30.38720, "lon": 78.13170, "hospital": False},
    {"id": 11, "name": "Rajpur", "lat": 30.36400, "lon": 78.09500, "hospital": False},
    {"id": 12, "name": "Clement Town", "lat": 30.27600, "lon": 78.06400, "hospital": False},
    {"id": 13, "name": "Prem Nagar", "lat": 30.34000, "lon": 78.01800, "hospital": False},
    {"id": 14, "name": "Raipur", "lat": 30.31000, "lon": 78.09300, "hospital": False},
]

EDGES = [
    (3, 5, 2.5), (5, 6, 1.0), (0, 5, 1.5), (0, 6, 1.0),
    (3, 4, 5.5), (4, 5, 7.0), (4, 12, 4.5), (5, 11, 4.5),
    (11, 1, 2.0), (5, 13, 4.5), (13, 7, 3.0), (7, 2, 3.5),
    (7, 5, 7.0), (3, 7, 6.5), (7, 9, 4.0), (3, 9, 8.0),
    (9, 8, 5.0), (5, 8, 8.0), (13, 8, 4.0), (8, 2, 6.0),
    (5, 14, 7.0), (14, 10, 8.0), (12, 14, 5.0), (12, 5, 5.0),
    (1, 13, 4.0), (0, 3, 2.0), (9, 2, 5.5), (6, 3, 2.0),
]
