// =====================================================
// 🔥 FINAL FRONTEND SCRIPT (FIREBASE + VIDEO + SIGNAL + EMERGENCY)
// =====================================================

// ================= FIREBASE CONFIG =================
const firebaseConfig = {
  apiKey: "AIzaSyDngaaXb2YXCWsiTGksoxz3WtRmtuIzyrY",
  authDomain: "traffic-system-c8423.firebaseapp.com",
  databaseURL: "https://traffic-system-c8423-default-rtdb.firebaseio.com/",
  projectId: "traffic-system-c8423",
};

firebase.initializeApp(firebaseConfig);
const db = firebase.database();

// ================= VIDEO STREAM =================
const API_BASE = "http://127.0.0.1:5000";

const CAMERA_STREAMS = {
  north: `${API_BASE}/api/stream/north`,
  south: `${API_BASE}/api/stream/south`,
  east:  `${API_BASE}/api/stream/east`,
  west:  `${API_BASE}/api/stream/west`,
};

const DIRS = ["north", "south", "east", "west"];
const SIG_MAP = { north: "n", south: "s", east: "e", west: "w" };

// ================= EMERGENCY STATE =================
let emergencyState = { north: false, south: false, east: false, west: false };

// ================= CAMERA INIT =================
function initCameraFeeds() {
  DIRS.forEach((dir) => {
    const feedEl = document.getElementById(`feed-${dir}`);
    const placeholder = document.getElementById(`placeholder-${dir}`);
    if (!feedEl) return;

    const img = document.createElement("img");
    img.src = CAMERA_STREAMS[dir];
    img.style.cssText = "width:100%;height:100%;object-fit:cover;position:absolute;inset:0;";
    img.onload = () => { if (placeholder) placeholder.style.display = "none"; };
    img.onerror = () => {
      if (placeholder) {
        placeholder.style.display = "flex";
        placeholder.innerHTML = "⚠ Loading video...";
      }
    };
    feedEl.insertBefore(img, feedEl.firstChild);
  });
}

// ================= CLOCK =================
function updateClock() {
  document.getElementById("clock").textContent = new Date().toLocaleTimeString("en-IN");
}
setInterval(updateClock, 1000);

// ================= UPDATE COUNTS =================
function updateCounts(c) {
  const values = DIRS.map(d => c[d] || 0);
  const max = Math.max(...values, 1);

  DIRS.forEach((dir, i) => {
    const val = values[i];
    document.getElementById(`count-${dir}`).textContent = val;

    const pct = Math.round((val / max) * 100);
    const bar = document.getElementById(`bar-${dir}`);
    if (bar) {
      bar.style.width = pct + "%";
      bar.style.background = pct > 75 ? "red" : pct > 50 ? "orange" : "green";
    }
    document.getElementById(`overlay-${dir}`).textContent = val + " vehicles";
  });
}

// ================= UPDATE STATS =================
function updateStats(c) {
  document.getElementById("stat-total").textContent = c.total;

  let max = Math.max(c.north, c.south, c.east, c.west);
  let busiest = ["north","south","east","west"].find(k => c[k] === max) || "-";
  document.getElementById("stat-busiest").textContent = busiest.toUpperCase();

  document.getElementById("stat-congestion").textContent =
    c.total > 120 ? "HIGH" : c.total > 60 ? "MEDIUM" : "LOW";

  document.getElementById("stat-update").textContent =
    c.timestamp || new Date().toLocaleTimeString();
}

// ================= SIGNAL SYSTEM =================
const SIGNAL_MIN = 10;
const SIGNAL_MAX = 60;

let current = 0;
let timer = 10;
let greenTimes = [10, 10, 10, 10];

function updateSignals(c) {
  const values = DIRS.map(d => c[d] || 0);
  const total = values.reduce((a, b) => a + b, 1);
  greenTimes = values.map(v =>
    Math.round(SIGNAL_MIN + (v / total) * (SIGNAL_MAX - SIGNAL_MIN))
  );
}

function signalLoop() {
  // Check if any emergency is active — override to green for that lane
  const emergencyDir = DIRS.find(d => emergencyState[d]);

  DIRS.forEach((dir, i) => {
    const d = SIG_MAP[dir];
    ["red", "green", "amber"].forEach(color => {
      document.getElementById(`sig-${d}-${color}`)?.classList.remove("active");
    });

    if (emergencyDir) {
      // Emergency override: green for emergency lane, red for all others
      if (dir === emergencyDir) {
        document.getElementById(`sig-${d}-green`)?.classList.add("active");
        document.getElementById(`sig-${d}-timer`).textContent = "🚨 EMERGENCY";
      } else {
        document.getElementById(`sig-${d}-red`)?.classList.add("active");
        document.getElementById(`sig-${d}-timer`).textContent = "🔴 HOLD";
      }
    } else {
      // Normal signal rotation
      if (i === current) {
        document.getElementById(`sig-${d}-green`)?.classList.add("active");
        document.getElementById(`sig-${d}-timer`).textContent = "🟢 " + timer + "s";
      } else {
        document.getElementById(`sig-${d}-red`)?.classList.add("active");
        document.getElementById(`sig-${d}-timer`).textContent = "--";
      }
    }
  });

  // Only advance the normal timer when no emergency
  if (!emergencyDir) {
    timer--;
    if (timer <= 0) {
      current = (current + 1) % 4;
      timer = greenTimes[current];
    }
  }
}

setInterval(signalLoop, 1000);

// ================= EMERGENCY PANEL UPDATE =================
function updateEmergencyPanel(emergencyData) {
  if (!emergencyData) return;

  emergencyState = emergencyData;

  const anyActive = Object.values(emergencyData).some(Boolean);

  // Update overall banner
  const banner = document.getElementById("emergency-banner");
  const bannerTitle = document.getElementById("emergency-title");
  const bannerStatus = document.getElementById("emergency-status");

  if (banner) {
    if (anyActive) {
      banner.classList.add("active");
      const activeDirs = DIRS.filter(d => emergencyData[d]).map(d => d.toUpperCase()).join(", ");
      bannerTitle.textContent = `🚨 Emergency Vehicle Detected — ${activeDirs}`;
      bannerStatus.textContent = "PRIORITY OVERRIDE ACTIVE";
      bannerStatus.className = "emergency-global-status detected";
      document.body.classList.add("emergency");
    } else {
      banner.classList.remove("active");
      bannerTitle.textContent = "All Clear — No Emergency Vehicles";
      bannerStatus.textContent = "NORMAL OPERATION";
      bannerStatus.className = "emergency-global-status clear";
      document.body.classList.remove("emergency");
    }
  }

  // Update per-direction emergency indicators
  DIRS.forEach(dir => {
    const indicator = document.getElementById(`emg-${dir}`);
    const card = document.getElementById(`dir-card-${dir}`);

    if (indicator) {
      if (emergencyData[dir]) {
        indicator.innerHTML = `<span class="emg-badge detected">🚨 EMERGENCY</span>`;
      } else {
        indicator.innerHTML = `<span class="emg-badge clear">✓ CLEAR</span>`;
      }
    }

    if (card) {
      card.classList.toggle("emergency-active", !!emergencyData[dir]);
    }
  });
}

// ================= FIREBASE LISTENER =================
db.ref("traffic").on("value", (snapshot) => {
  const data = snapshot.val();
  if (!data) return;

  updateCounts(data);
  updateStats(data);
  updateSignals(data);

  if (data.emergency) {
    updateEmergencyPanel(data.emergency);
  }

  document.getElementById("api-status").className = "status-badge online";
  document.getElementById("api-status-text").textContent = "FIREBASE LIVE";
});

// ================= START =================
initCameraFeeds();

console.log("🔥 Traffic Dashboard Running (Firebase + Emergency Mode)");
console.log("🚨 Emergency keys (backend): ↑=North ↓=South →=East ←=West");
