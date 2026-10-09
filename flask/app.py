
import ctypes
import json
import os
import threading
import time

from flask import Flask, jsonify, request, send_from_directory

from city_data import NODES, EDGES

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(BASE_DIR)
FRONTEND_DIR = os.path.join(ROOT_DIR, "frontend")

LIB_PATH = os.environ.get("DISPATCH_LIB", os.path.join(ROOT_DIR, "dispatch.dll"))

lib = ctypes.CDLL(LIB_PATH)
lib.engine_init.restype = None
lib.engine_reset.restype = None
lib.engine_add_ambulance.argtypes = [ctypes.c_int]
lib.engine_add_ambulance.restype = ctypes.c_int
lib.engine_remove_ambulance.argtypes = [ctypes.c_int]
lib.engine_remove_ambulance.restype = ctypes.c_int
lib.engine_set_ambulance_node.argtypes = [ctypes.c_int, ctypes.c_int]
lib.engine_set_ambulance_node.restype = ctypes.c_int
lib.engine_add_emergency.argtypes = [ctypes.c_int, ctypes.c_int]
lib.engine_add_emergency.restype = ctypes.c_int
lib.engine_tick.argtypes = [ctypes.c_double]
lib.engine_tick.restype = None
lib.engine_get_state.restype = ctypes.c_char_p
lib.engine_ambulance_count.restype = ctypes.c_int
lib.engine_sim_time.restype = ctypes.c_double

engine_lock = threading.Lock()
lib.engine_init()

# 1 real second = 1 simulated minute; ambulances drive at 40 km/h in-sim.
TICK_REAL_SECONDS = 1.0
TICK_SIM_MINUTES = 1.0


def _ticker():
    while True:
        time.sleep(TICK_REAL_SECONDS)
        with engine_lock:
            lib.engine_tick(TICK_SIM_MINUTES)


threading.Thread(target=_ticker, daemon=True).start()

app = Flask(__name__)


def _state():
    with engine_lock:
        raw = lib.engine_get_state()
    return json.loads(raw.decode("utf-8"))


@app.route("/")
def index():
    return send_from_directory(FRONTEND_DIR, "index.html")


@app.route("/<path:filename>")
def frontend_files(filename):
    # serve app.js / style.css next to index.html
    full = os.path.join(FRONTEND_DIR, filename)
    if os.path.isfile(full):
        return send_from_directory(FRONTEND_DIR, filename)
    return jsonify({"error": "not found"}), 404


@app.route("/api/city")
def api_city():
    return jsonify({
        "nodes": NODES,
        "edges": [{"a": a, "b": b, "km": km} for a, b, km in EDGES],
    })


@app.route("/api/state")
def api_state():
    return jsonify(_state())


@app.route("/api/emergency", methods=["POST"])
def api_emergency():
    data = request.get_json(force=True, silent=True) or {}
    node, severity = data.get("node"), data.get("severity")
    if not isinstance(node, int) or not isinstance(severity, int):
        return jsonify({"error": "node and severity must be integers"}), 400
    with engine_lock:
        emg_id = lib.engine_add_emergency(node, severity)
    if emg_id < 0:
        return jsonify({"error": "invalid node (0-14) or severity (1-3)"}), 400
    return jsonify({"emergency_id": emg_id, "state": _state()})


@app.route("/api/ambulances", methods=["POST"])
def api_add_ambulance():
    data = request.get_json(force=True, silent=True) or {}
    node = data.get("node")
    if not isinstance(node, int):
        return jsonify({"error": "node must be an integer"}), 400
    with engine_lock:
        amb_id = lib.engine_add_ambulance(node)
    if amb_id < 0:
        return jsonify({"error": "invalid node or fleet full"}), 400
    return jsonify({"ambulance_id": amb_id, "state": _state()})


@app.route("/api/ambulances/<int:amb_id>", methods=["DELETE"])
def api_remove_ambulance(amb_id):
    with engine_lock:
        ok = lib.engine_remove_ambulance(amb_id)
    if not ok:
        return jsonify({"error": "unknown ambulance, ambulance is busy, or it is the last one"}), 400
    return jsonify({"ok": True, "state": _state()})


@app.route("/api/ambulances/<int:amb_id>/position", methods=["POST"])
def api_move_ambulance(amb_id):
    data = request.get_json(force=True, silent=True) or {}
    node = data.get("node")
    if not isinstance(node, int):
        return jsonify({"error": "node must be an integer"}), 400
    with engine_lock:
        ok = lib.engine_set_ambulance_node(amb_id, node)
    if not ok:
        return jsonify({"error": "unknown ambulance, bad node, or ambulance is busy"}), 400
    return jsonify({"ok": True, "state": _state()})


@app.route("/api/reset", methods=["POST"])
def api_reset():
    with engine_lock:
        lib.engine_reset()
    return jsonify({"ok": True, "state": _state()})


if __name__ == "__main__":
    print("Engine loaded from:", LIB_PATH)
    print("Open http://127.0.0.1:5000/ in your browser")
    app.run(host="127.0.0.1", port=5000, debug=False)
