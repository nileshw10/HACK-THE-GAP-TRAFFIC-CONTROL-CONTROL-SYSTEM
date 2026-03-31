# HACK-THE-GAP

# 🚦 Smart Traffic Management System

An AI-powered real-time traffic management system that uses **YOLOv8 object detection** to count vehicles across 4 intersection lanes, dynamically allocates green light durations, detects emergency vehicles, and streams all data live to a web dashboard via **Firebase Realtime Database**.

---

## 📸 Demo

<img width="1920" height="1080" alt="Screenshot (1)" src="https://github.com/user-attachments/assets/5a4b7d2e-47ef-4bb4-8d51-f02d75668b94" />

<img width="1920" height="1080" alt="Screenshot (2)" src="https://github.com/user-attachments/assets/cfd44fb8-8720-4d51-9821-f794826fc302" />

<img width="1920" height="1080" alt="Screenshot (3)" src="https://github.com/user-attachments/assets/81a3192e-634b-474a-9230-e2be2f867aa7" />

<img width="1920" height="1080" alt="Screenshot (4)" src="https://github.com/user-attachments/assets/409c51eb-a889-4826-a131-a5047b8e9e49" />


## 🧠 How It Works

1. **Video feeds** from 4 IP cameras (North, South, East, West) are read via OpenCV.
2. **YOLOv8n** runs object detection on each frame, counting vehicles (car, motorcycle, bus, truck).
3. **Vehicle counts** are pushed to Firebase Realtime Database every 2 seconds.
4. **Traffic signal durations** are dynamically allocated — busier lanes get longer green lights.
5. **Emergency vehicle detection** is simulated via keyboard arrow keys; the detected lane gets immediate green light priority.
6. The **React-less frontend** reads Firebase in real time and renders live counts, signal states, camera feeds, and emergency alerts.

---

## 🗂️ Project Structure

```
smart-traffic/
├── backend/
│   ├── python.py              # Main backend — YOLO, Firebase, Flask, signals
│   ├── run.bat                # Windows launcher script
│   ├── serviceAccountKey.json # Firebase Admin SDK credentials (do NOT commit)
│   └── videos/
│       ├── north.mp4
│       ├── south.mp4
│       ├── east.mp4
│       └── west.mp4
├── frontend/
│   ├── index.html             # Dashboard UI
│   ├── script.js              # Firebase listener + signal logic
│   └── style.css              # Dark-theme UI styles
└── yolov8n.pt                 # YOLOv8 nano model weights
```

---

## ⚙️ Tech Stack

| Layer | Technology |
|---|---|
| Object Detection | [YOLOv8n](https://github.com/ultralytics/ultralytics) (Ultralytics) |
| Video Processing | OpenCV (`cv2`) |
| Backend API | Flask + Flask-CORS |
| Database | Firebase Realtime Database (Pyrebase4) |
| Auth / Admin | Firebase Admin SDK (Firestore) |
| Serial (optional) | PySerial — COM3 for hardware signals |
| Frontend | Vanilla HTML/CSS/JS + Firebase JS SDK v10 |
| Keyboard Sim | `pynput` — arrow keys trigger emergency |

---

## 🚀 Getting Started

### Prerequisites

- Python 3.10+
- A Firebase project with Realtime Database enabled
- (Optional) 4 IP cameras streaming MJPEG at `/stream`

### 1. Clone the repository

```bash
git clone https://github.com/your-username/smart-traffic.git
cd smart-traffic
```

### 2. Create and activate a virtual environment

```bash
python -m venv .venv

# Windows
.venv\Scripts\activate

# macOS/Linux
source .venv/bin/activate
```

### 3. Install dependencies

```bash
pip install ultralytics opencv-python pyrebase4 flask flask-cors pynput firebase-admin pyserial
```

### 4. Configure Firebase

#### Backend (`python.py`)

```python
const firebaseConfig = {
  apiKey: "AIzaSyDngaaXb2YXCWsiTGksoxz3WtRmtuIzyrY",
  authDomain: "traffic-system-c8423.firebaseapp.com",
  databaseURL: "https://traffic-system-c8423-default-rtdb.firebaseio.com",
  projectId: "traffic-system-c8423",
  storageBucket: "traffic-system-c8423.firebasestorage.app",
  messagingSenderId: "841697356650",
  appId: "1:841697356650:web:d0592181ce72a855bed313",
  measurementId: "G-0LCEENFQMJ"
};
```

#### Frontend (`frontend/script.js`)
```script.js
const firebaseConfig = {
  apiKey: "AIzaSyDngaaXb2YXCWsiTGksoxz3WtRmtuIzyrY",
  authDomain: "traffic-system-c8423.firebaseapp.com",
  databaseURL: "https://traffic-system-c8423-default-rtdb.firebaseio.com/",
  projectId: "traffic-system-c8423",
};
```
#### Service Account Key

```
backend/serviceAccountKey.json
```


### 5. Configure video sources

In `python.py`, update `VIDEO_PATHS` to point to your local video files or IP camera streams:

```python
VIDEO_PATHS = {
    "LANE_N": "videos/north.mp4",        
    "LANE_S": "videos/south.mp4",
    "LANE_E": "videos/east.mp4",
    "LANE_W": "videos/west.mp4",
}
```

### 6. Run the backend

```bash
# Option A: direct
python backend/python.py

# Option B: Windows batch script
run.bat
```

### 7. Open the dashboard

Open `frontend/index.html` in your browser. The dashboard connects directly to Firebase — no local server needed for the frontend.

---

## 🚨 Emergency Vehicle Simulation

With the backend running and a terminal window focused, use arrow keys to simulate emergency detection:

| Key | Lane |
|-----|------|
| ↑ Up | North |
| ↓ Down | South |
| → Right | East |
| ← Left | West |

The emergency lane immediately receives a **green light override** for 15 seconds, then auto-clears. The dashboard will flash red and show the priority override.

---

## 📡 Firebase Realtime Database Schema

```json
{
  "traffic": {
    "north": 15,
    "south": 12,
    "east": 3,
    "west": 8,
    "total": 28,
    "timestamp": "14:32:01",
    "emergency": {
      "north": false,
      "south": false,
      "east": true,
      "west": false
    }
  }
}
```

---

## 🌐 Flask API Endpoints

| Endpoint | Description |
|---|---|
| `GET /api/stream/north` | MJPEG stream for North lane |
| `GET /api/stream/south` | MJPEG stream for South lane |
| `GET /api/stream/east` | MJPEG stream for East lane |
| `GET /api/stream/west` | MJPEG stream for West lane |

Default: `http://127.0.0.1:5000`

---

## ⚡ Signal Logic

- Green time range: **10s (min) → 60s (max)**
- Each lane's green time is proportional to its vehicle count relative to the total
- Emergency detected → that lane turns **green immediately**, all others **hold red**
- Emergency auto-clears after **15 seconds**

---

## 🔒 Security Notes

- **Do NOT commit** `serviceAccountKey.json` or any API keys to GitHub
- Add the following to your `.gitignore`:

```
.venv/
serviceAccountKey.json
*.pt
__pycache__/
*.pyc
```

- The Firebase API key in `firebaseConfig` is a **client-side key** (safe to expose), but restrict its usage in the Firebase Console under **API restrictions**.
- For production, replace the Flask development server with **Gunicorn** or another WSGI server.

---

## 🐛 Known Issues / Troubleshooting

| Issue | Cause | Fix |
|---|---|---|
| `KeyError: 'storageBucket'` | Missing key in `firebaseConfig` | Add `"storageBucket": "your-project.appspot.com"` |
| `FileNotFoundError: serviceAccountKey.json` | Wrong path | Ensure file is at `backend/serviceAccountKey.json` |
| `could not open port 'COM3'` | No serial device | Safe to ignore if not using hardware signals |
| Camera stream timeout | IP cameras unreachable | Use local `.mp4` files for testing |
| All vehicle counts show `0` | Cameras not returning frames | Check `VIDEO_PATHS` values |

---

## 🤝 Contributing

Pull requests are welcome! For major changes, please open an issue first to discuss what you'd like to change.

---

## 📄 License

MIT License — feel free to use, modify, and distribute.

---

## 🙏 Acknowledgements

- [Ultralytics YOLOv8](https://github.com/ultralytics/ultralytics)
- [Firebase](https://firebase.google.com/)
- [OpenCV](https://opencv.org/)
- [Flask](https://flask.palletsprojects.com/)
