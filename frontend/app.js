/* Ambulance Dispatch frontend — Karyon-style coordination dashboard.
 * Leaflet map over real Dehradun geography; polls /api/state every 800 ms. */

const SHORT = {
  0: "Doon Hospital", 1: "Max Hospital", 2: "GEIMS", 3: "Railway Stn",
  4: "ISBT", 5: "Clock Tower", 6: "Paltan Bzr", 7: "FRI",
  8: "Robber's Cave", 9: "Tapkeshwar", 10: "Sahastradhara",
  11: "Rajpur", 12: "Clement Town", 13: "Prem Nagar", 14: "Raipur",
};
const SEV_NAME = { 1: "Critical", 2: "Serious", 3: "Minor" };
const SEV_COLOR = { 1: "#e74c3c", 2: "#f39c12", 3: "#2ecc71" };
const AMB_STATUS = { 0: "Available", 1: "To patient", 2: "To hospital" };
const AMB_COLOR = { 0: "#2ecc71", 1: "#f39c12", 2: "#f39c12" };

let city = null;
let nodeById = {};
let roadGeom = {};   // "a,b" (a<b) -> {geometry: [[lat,lon],...], km_sim, km_real}
let map = null;
let roadLayer, routeLayer, nodeLayer, emgLayer, ambLayer;
let nodeMarkers = {};      // node id -> circleMarker
let ambMarkers = {};       // ambulance id -> marker
let selectedNode = null;
let highlightedEmg = null; // emergency id highlighted from the incident list
let fleetOpen = false;

function esc(s) {
  return String(s).replace(/[&<>"]/g, c =>
    ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
}

function edgeKey(a, b) { return a < b ? `${a},${b}` : `${b},${a}`; }

/* geometry oriented from u to v (stored geometry may run either way) */
function edgeGeometry(u, v) {
  const g = roadGeom[edgeKey(u, v)];
  if (!g || !g.geometry || g.geometry.length < 2) return null;
  const pts = g.geometry;
  const first = pts[0], last = pts[pts.length - 1];
  const nu = nodeById[u];
  const dFirst = Math.abs(first[0] - nu.lat) + Math.abs(first[1] - nu.lon);
  const dLast = Math.abs(last[0] - nu.lat) + Math.abs(last[1] - nu.lon);
  return dFirst <= dLast ? pts : [...pts].reverse();
}

function segLenKm(p, q) {
  const R = 6371, d2r = Math.PI / 180;
  const dLat = (q[0] - p[0]) * d2r, dLon = (q[1] - p[1]) * d2r;
  const mLat = ((p[0] + q[0]) / 2) * d2r;
  return R * Math.sqrt(dLat * dLat + Math.pow(Math.cos(mLat) * dLon, 2));
}

/* points of edge u->v from fraction `frac` along it to v (real road shape) */
function edgeRemainder(u, v, frac) {
  const pts = edgeGeometry(u, v);
  const nv = nodeById[v];
  if (!pts) return [[nv.lat, nv.lon]];
  let total = 0;
  const cum = [0];
  for (let i = 1; i < pts.length; i++) {
    total += segLenKm(pts[i - 1], pts[i]);
    cum.push(total);
  }
  const target = Math.min(Math.max(frac, 0), 1) * total;
  let i = 1;
  while (i < cum.length - 1 && cum[i] < target) i++;
  const segLen = (cum[i] - cum[i - 1]) || 1;
  const f = (target - cum[i - 1]) / segLen;
  const pos = [
    pts[i - 1][0] + (pts[i][0] - pts[i - 1][0]) * f,
    pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * f,
  ];
  return [pos, ...pts.slice(i)];
}

async function init() {
  const [cityRes, geomRes] = await Promise.all([
    fetch("/api/city"),
    fetch("road_geometries.json"),
  ]);
  city = await cityRes.json();
  nodeById = Object.fromEntries(city.nodes.map(n => [n.id, n]));
  const geoms = await geomRes.json();
  for (const g of geoms) roadGeom[edgeKey(g.a, g.b)] = g;
  initMap();
  buildRoads();
  buildNodes();
  bindControls();
  await refresh();
  setInterval(refresh, 800);
}

/* ---------------- map ---------------- */

function initMap() {
  map = L.map("map", { zoomControl: true, attributionControl: true });
  // Explicit view first: Leaflet's renderer needs a real view before
  // path layers are added, otherwise rendering aborts.
  map.setView([30.33, 78.055], 12);

  L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
    attribution: '&copy; <a href="https://openstreetmap.org/copyright">OpenStreetMap</a>',
    maxZoom: 18,
  }).addTo(map);

  roadLayer = L.layerGroup().addTo(map);
  routeLayer = L.layerGroup().addTo(map);
  nodeLayer = L.layerGroup().addTo(map);
  emgLayer = L.layerGroup().addTo(map);
  ambLayer = L.layerGroup().addTo(map);
}

function buildRoads() {
  for (const e of city.edges) {
    const g = roadGeom[edgeKey(e.a, e.b)];
    const pts = g && g.geometry
      ? g.geometry
      : [[nodeById[e.a].lat, nodeById[e.a].lon], [nodeById[e.b].lat, nodeById[e.b].lon]];
    const line = L.polyline(pts, { color: "#34495e", weight: 3, opacity: 0.75 });
    const realTxt = g && g.km_real
      ? `<br>Real road distance: ${g.km_real} km`
      : "";
    line.bindTooltip(`<b>${e.km} km</b>`, {
      permanent: true, direction: "center", className: "road-label",
    });
    line.bindPopup(
      `<strong>${esc(nodeById[e.a].name)}</strong> &harr; <strong>${esc(nodeById[e.b].name)}</strong><br>` +
      `Simulated weight: ${e.km} km${realTxt}`
    );
    line.addTo(roadLayer);
  }
  applyRoadToggles();
}

function applyRoadToggles() {
  const showRoads = document.getElementById("toggle-roads").checked;
  const showLabels = document.getElementById("toggle-labels").checked;
  roadLayer.eachLayer(line => {
    const el = line.getElement && line.getElement();
    if (el) el.style.display = showRoads ? "" : "none";
    const tip = line.getTooltip();
    if (tip) {
      const tipEl = tip.getElement && tip.getElement();
      if (tipEl) tipEl.style.display = (showRoads && showLabels) ? "" : "none";
    }
  });
}

function buildNodes() {
  for (const n of city.nodes) {
    let m;
    if (n.hospital) {
      // big unmissable badge, always drawn above ambulance markers
      m = L.marker([n.lat, n.lon], { icon: hospitalIcon(false), zIndexOffset: 1000 });
    } else {
      m = L.circleMarker([n.lat, n.lon], {
        radius: 7,
        fillColor: "#1a2733",
        color: "#8899aa",
        weight: 2,
        fillOpacity: 0.95,
      });
    }
    m.bindTooltip(n.name + (n.hospital ? " (hospital)" : ""), {
      direction: "top", offset: [0, -8], className: "node-tip",
    });
    m.bindPopup(
      `<strong>${esc(n.name)}</strong><br>` +
      (n.hospital ? "Hospital — trip destination" : "Emergency location") +
      `<br><span style="color:#8899aa">Click to select for a new emergency</span>`
    );
    m.on("click", () => selectNode(n.id));
    m.addTo(nodeLayer);
    nodeMarkers[n.id] = m;
  }
  const bounds = city.nodes.map(n => [n.lat, n.lon]);
  map.fitBounds(bounds, { padding: [30, 30] });
}

function hospitalIcon(selected) {
  const border = selected ? "#ffeb3b" : "#3498db";
  return L.divIcon({
    html: `<div class="hosp-divicon" style="border-color:${border};">H</div>`,
    className: "",
    iconSize: [32, 32],
    iconAnchor: [16, 16],
  });
}

function selectNode(id) {
  selectedNode = id;
  for (const [nid, m] of Object.entries(nodeMarkers)) {
    const nidInt = parseInt(nid, 10);
    const n = nodeById[nidInt];
    const isSel = nidInt === id;
    if (n.hospital) m.setIcon(hospitalIcon(isSel));
    else m.setStyle({ color: isSel ? "#ffeb3b" : "#8899aa", weight: isSel ? 4 : 2 });
  }
  document.getElementById("selNode").textContent = nodeById[id].name;
}

function ambLatLng(a) {
  // position along the REAL road shape of the current edge
  const [lat, lon] = edgeRemainder(a.seg_from, a.seg_to, a.seg_frac)[0];
  if (a.status === 0) {
    // nudge parked ambulances off the node so hospital badges stay visible
    return [lat + 0.0016, lon + 0.0019];
  }
  return [lat, lon];
}

function renderMarkers(state) {
  // waiting emergencies -> numbered badges (Karyon incident-marker style)
  emgLayer.clearLayers();
  const queue = [...state.queue].sort((x, y) => x.severity - y.severity || x.id - y.id);
  queue.forEach((q, i) => {
    const n = nodeById[q.node];
    const color = SEV_COLOR[q.severity];
    const border = (highlightedEmg === q.id) ? "3px solid #ffeb3b" : "2px solid #fff";
    const icon = L.divIcon({
      html: `<div class="emg-divicon" style="background-color:${color};border:${border};width:26px;height:26px;">${i + 1}</div>`,
      className: "",
      iconSize: [26, 26],
      iconAnchor: [13, 13],
    });
    const m = L.marker([n.lat, n.lon], { icon, zIndexOffset: 500 });
    m.bindPopup(
      `<strong>Emergency #${q.id}</strong><br>` +
      `${esc(n.name)}<br>Severity ${q.severity} · ${SEV_NAME[q.severity]} — awaiting ambulance`
    );
    m.addTo(emgLayer);
  });

  // ambulances -> status-coloured badges
  const seen = new Set();
  for (const a of state.ambulances) {
    seen.add(a.id);
    const [lat, lon] = ambLatLng(a);
    const color = AMB_COLOR[a.status];
    const icon = L.divIcon({
      html: `<div class="amb-divicon" style="background-color:${color};width:26px;height:26px;">A${a.id}</div>`,
      className: "",
      iconSize: [26, 26],
      iconAnchor: [13, 13],
    });
    const detail = a.status === 0
      ? `at ${nodeById[a.node].name}`
      : (a.status === 1
        ? `to patient #${a.emg_id} (${SHORT[a.seg_from]} → ${SHORT[a.seg_to]})`
        : `to ${nodeById[a.hospital].name} (#${a.emg_id})`);
    if (ambMarkers[a.id]) {
      ambMarkers[a.id].setLatLng([lat, lon]);
      ambMarkers[a.id].setIcon(icon);
      ambMarkers[a.id].setPopupContent(`<strong>Ambulance ${a.id}</strong><br>${AMB_STATUS[a.status]}<br>${esc(detail)}`);
    } else {
      const m = L.marker([lat, lon], { icon, zIndexOffset: 400 });
      m.bindPopup(`<strong>Ambulance ${a.id}</strong><br>${AMB_STATUS[a.status]}<br>${esc(detail)}`);
      m.addTo(ambLayer);
      ambMarkers[a.id] = m;
    }
  }
  for (const id of Object.keys(ambMarkers)) {
    if (!seen.has(parseInt(id, 10))) {
      ambLayer.removeLayer(ambMarkers[id]);
      delete ambMarkers[id];
    }
  }
}

function renderRoutes(state) {
  routeLayer.clearLayers();
  if (!document.getElementById("toggle-routes").checked) return;
  for (const a of state.ambulances) {
    if (a.status === 0 || !a.route || a.route.length < 2) continue;
    // stitch real road geometries along the engine's node path
    let pts = edgeRemainder(a.route[0], a.route[1], a.seg_frac);
    for (let i = 2; i < a.route.length; i++) {
      const g = edgeGeometry(a.route[i - 1], a.route[i]);
      if (g) {
        pts = pts.concat(g.slice(1));
      } else {
        const n = nodeById[a.route[i]];
        pts.push([n.lat, n.lon]);
      }
    }
    L.polyline(pts, { color: "#f39c12", weight: 4, opacity: 0.85 }).addTo(routeLayer);
  }
}

/* ---------------- panel ---------------- */

function renderIncidents(state) {
  const box = document.getElementById("incident-list");
  box.innerHTML = "";
  const queue = [...state.queue].sort((x, y) => x.severity - y.severity || x.id - y.id);
  if (!queue.length) {
    box.innerHTML = `<p class="empty-note">No waiting emergencies — the queue is clear.</p>`;
    return;
  }
  queue.forEach((q, i) => {
    const card = document.createElement("div");
    card.className = "incident-card" + (highlightedEmg === q.id ? " active" : "");
    card.innerHTML =
      `<div class="incident-header">` +
      `<span class="incident-rank">${i + 1}</span>` +
      `<div class="incident-info">` +
      `<div class="incident-name">${esc(nodeById[q.node].name)}</div>` +
      `<div class="incident-desc">Emergency #${q.id} · Severity ${q.severity} · ${SEV_NAME[q.severity]} · awaiting ambulance</div>` +
      `</div>` +
      `<span class="incident-score score-s${q.severity}">S${q.severity}</span>` +
      `</div>`;
    card.addEventListener("click", () => {
      highlightedEmg = (highlightedEmg === q.id) ? null : q.id;
      renderIncidents(state);
      renderMarkers(state);
      const n = nodeById[q.node];
      map.panTo([n.lat, n.lon]);
    });
    box.appendChild(card);
  });
}

function renderFleet(state) {
  const box = document.getElementById("resource-status");
  box.innerHTML = "";
  const ambs = [...state.ambulances].sort((x, y) => x.id - y.id);
  const free = ambs.filter(a => a.status === 0).length;
  const busy = ambs.length - free;

  const row = document.createElement("div");
  row.className = "resource-type-row";
  row.innerHTML =
    `<span class="resource-type-label">Ambulances</span>` +
    `<span class="resource-counts">` +
    `<span class="count-badge count-available">${free} avail</span>` +
    `<span class="count-badge count-busy">${busy} busy</span>` +
    `<span class="resource-expand-icon">▾</span>` +
    `</span>`;
  const drill = document.createElement("div");
  drill.className = "resource-drilldown" + (fleetOpen ? " expanded" : "");
  const content = document.createElement("div");
  content.className = "drilldown-content";
  for (const a of ambs) {
    const unit = document.createElement("div");
    unit.className = "resource-unit-row";
    let detail;
    if (a.status === 0) {
      detail = `at ${nodeById[a.node].name}`;
    } else if (a.status === 1) {
      detail = `→ patient #${a.emg_id} (${SHORT[a.seg_from]} → ${SHORT[a.seg_to]})`;
    } else {
      detail = `→ ${nodeById[a.hospital].name} (#${a.emg_id})`;
    }
    unit.innerHTML =
      `<span class="unit-id">A${a.id}</span>` +
      `<span class="unit-status unit-status-${a.status === 0 ? "available" : "busy"}">${AMB_STATUS[a.status]}</span>` +
      `<span class="unit-assignment">${esc(detail)}</span>`;
    if (a.status === 0) {
      const x = document.createElement("button");
      x.className = "unit-remove";
      x.textContent = "×";
      x.title = "Remove this ambulance from the fleet";
      x.addEventListener("click", async (ev) => {
        ev.stopPropagation();
        if (!confirm(`Remove Ambulance A${a.id} from the fleet?`)) return;
        const r = await fetch(`/api/ambulances/${a.id}`, { method: "DELETE" });
        if (!r.ok) {
          const err = await r.json().catch(() => ({}));
          alert(err.error || "Could not remove ambulance.");
        }
        await refresh();
      });
      unit.appendChild(x);
    }
    unit.addEventListener("click", () => {
      const [lat, lon] = ambLatLng(a);
      map.panTo([lat, lon]);
      if (ambMarkers[a.id]) ambMarkers[a.id].openPopup();
    });
    content.appendChild(unit);
  }
  drill.appendChild(content);
  row.addEventListener("click", () => {
    fleetOpen = !fleetOpen;
    drill.classList.toggle("expanded", fleetOpen);
  });
  box.appendChild(row);
  box.appendChild(drill);

  const freeAmbs = ambs.filter(a => a.status === 0);
  document.getElementById("moveAmbId").innerHTML = freeAmbs.length
    ? freeAmbs.map(a => `<option value="${a.id}">A${a.id} — ${esc(nodeById[a.node].name)}</option>`).join("")
    : `<option value="">(none free)</option>`;
}

function renderLog(state) {
  const log = document.getElementById("changes-display");
  log.innerHTML = "";
  for (const e of state.events) {
    const div = document.createElement("div");
    div.textContent = e;
    log.appendChild(div);
  }
  log.scrollTop = log.scrollHeight;
}

async function refresh() {
  let s;
  try {
    s = await (await fetch("/api/state")).json();
  } catch (e) {
    return;
  }
  document.getElementById("sim-clock").textContent = s.sim_minutes.toFixed(1);
  document.getElementById("sim-completed").textContent = s.completed;
  renderIncidents(s);
  renderFleet(s);
  renderMarkers(s);
  renderRoutes(s);
  renderLog(s);
}

/* ---------------- controls ---------------- */

function bindControls() {
  document.querySelectorAll(".sev-buttons .action-btn").forEach(btn => {
    btn.addEventListener("click", async () => {
      if (selectedNode === null) {
        alert("Click a location node on the map first.");
        return;
      }
      const r = await fetch("/api/emergency", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ node: selectedNode, severity: parseInt(btn.dataset.sev, 10) }),
      });
      if (!r.ok) alert("Could not add emergency.");
      await refresh();
    });
  });

  const nodeOpts = city.nodes
    .map(n => `<option value="${n.id}">${n.id} · ${esc(n.name)}</option>`).join("");
  document.getElementById("addAmbNode").innerHTML = nodeOpts;
  document.getElementById("moveAmbNode").innerHTML = nodeOpts;

  document.getElementById("fleet-btn").addEventListener("click", () => {
    document.getElementById("fleet-picker").classList.toggle("hidden");
  });

  document.getElementById("addAmbBtn").addEventListener("click", async () => {
    const node = parseInt(document.getElementById("addAmbNode").value, 10);
    const r = await fetch("/api/ambulances", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ node }),
    });
    if (!r.ok) alert("Could not add ambulance (fleet may be full).");
    await refresh();
  });

  document.getElementById("moveAmbBtn").addEventListener("click", async () => {
    const ambId = parseInt(document.getElementById("moveAmbId").value, 10);
    if (!ambId) return;
    const node = parseInt(document.getElementById("moveAmbNode").value, 10);
    const r = await fetch(`/api/ambulances/${ambId}/position`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ node }),
    });
    if (!r.ok) alert("Reposition failed — ambulance may be busy.");
    await refresh();
  });

  document.getElementById("reset-btn").addEventListener("click", async () => {
    if (!confirm("Total reset of the simulation?")) return;
    await fetch("/api/reset", { method: "POST" });
    highlightedEmg = null;
    selectedNode = null;
    await refresh();
  });

  document.getElementById("toggle-routes").addEventListener("change", refresh);
  document.getElementById("toggle-roads").addEventListener("change", applyRoadToggles);
  document.getElementById("toggle-labels").addEventListener("change", applyRoadToggles);
}

init();
