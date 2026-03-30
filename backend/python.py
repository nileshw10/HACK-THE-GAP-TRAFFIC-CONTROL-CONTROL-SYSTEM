import cv2
import time
import threading
from ultralytics import YOLO
import pyrebase
from flask import Flask, Response
from flask_cors import CORS
from pynput import keyboard
 
app = Flask(__name__)
CORS(app)


# =========================
# 🔥 FIREBASE CONFIG
# =========================

firebaseConfig = {
    "apiKey": "AIzaSyDngaaXb2YXCWsiTGksoxz3WtRmtuIzyrY",
    "authDomain": "traffic-system-c8423.firebaseapp.com",
    "databaseURL": "https://traffic-system-c8423-default-rtdb.firebaseio.com/",
    "projectId": "traffic-system-c8423",
    "storageBucket": "traffic-system-c8423.appspot.com",
    "messagingSenderId": "000000000000",
    "appId": "1:000000000000:web:0000000000000000000000"
}

firebase = pyrebase.initialize_app(firebaseConfig)
db = firebase.database()

# =========================
# VIDEO CONFIG
# =========================

VIDEO_PATHS = {
    "LANE_N": "D:\\hack2\\backend\\videos\\north.mp4",
    "LANE_S": "D:\\hack2\\backend\\videos\\south.mp4",
    "LANE_E": "D:\\hack2\\backend\\videos\\east.mp4",
    "LANE_W": "D:\\hack2\\backend\\videos\\west.mp4",
}

LANES = ["LANE_N", "LANE_S", "LANE_E", "LANE_W"]

# =========================
# LOAD YOLO
# =========================

print("Loading YOLO...")
model = YOLO("yolov8n.pt")
print("YOLO Ready!")

VEHICLES = {"car", "motorcycle", "bus", "truck"}

# ─── Box colours per vehicle class ───────────────────────────────────────────
BOX_COLORS = {
    "car":        (0, 255, 0),    # green
    "motorcycle": (0, 200, 255),  # cyan-yellow
    "bus":        (255, 128, 0),  # orange
    "truck":      (0, 128, 255),  # blue
}
DEFAULT_COLOR = (200, 200, 200)   # grey fallback

# =========================
# GLOBAL DATA
# =========================

frames           = {lane: None for lane in LANES}   # raw frames from video reader
annotated_frames = {lane: None for lane in LANES}   # frames with YOLO boxes drawn
counts           = {lane: 0    for lane in LANES}
locks            = {lane: threading.Lock() for lane in LANES}
ann_locks        = {lane: threading.Lock() for lane in LANES}

# =========================
# 🚨 EMERGENCY VEHICLE STATE
# =========================

emergency = {
    "LANE_N": False,
    "LANE_S": False,
    "LANE_E": False,
    "LANE_W": False,
}

EMERGENCY_DURATION = 15  # seconds
emergency_timers = {}

def clear_emergency_direct(lane):
    emergency[lane] = False
    emergency_timers.pop(lane, None)
    print(f"🟢 Emergency auto-cleared for {lane}")

def trigger_emergency(lane):
    """Trigger emergency for a lane, restart timer if already active."""
    emergency[lane] = True
    direction_name = lane.replace("LANE_", "")
    print(f"🚨 EMERGENCY VEHICLE DETECTED — {direction_name} lane! (auto-clears in {EMERGENCY_DURATION}s)")

    if lane in emergency_timers:
        emergency_timers[lane].cancel()

    t = threading.Timer(EMERGENCY_DURATION, lambda: clear_emergency_direct(lane))
    t.daemon = True
    t.start()
    emergency_timers[lane] = t

# =========================
# ⌨️ KEYBOARD LISTENER
# =========================

ARROW_TO_LANE = {
    keyboard.Key.up:    "LANE_N",
    keyboard.Key.down:  "LANE_S",
    keyboard.Key.right: "LANE_E",
    keyboard.Key.left:  "LANE_W",
}

def on_key_press(key):
    if key in ARROW_TO_LANE:
        trigger_emergency(ARROW_TO_LANE[key])

def start_keyboard_listener():
    print("⌨️  Keyboard listener active:")
    print("    ↑ = NORTH  ↓ = SOUTH  → = EAST  ← = WEST")
    with keyboard.Listener(on_press=on_key_press) as listener:
        listener.join()

# =========================
# VIDEO READER
# =========================

def video_reader(lane, path):
    cap = cv2.VideoCapture(path)

    while True:
        ret, frame = cap.read()

        if not ret:
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
            continue

        with locks[lane]:
            frames[lane] = frame

        time.sleep(0.03)

# =========================
# YOLO DETECTION  ← draws bounding boxes here
# =========================

def draw_boxes(frame, results):
    """
    Draw colour-coded bounding boxes and labels on *frame* for every
    vehicle detected in *results*.  Returns the annotated copy.
    """
    annotated = frame.copy()

    for box in results[0].boxes:
        cls   = int(box.cls[0])
        label = model.names[cls]
        conf  = float(box.conf[0])

        if label not in VEHICLES:
            continue

        # Bounding box coordinates (pixel integers)
        x1, y1, x2, y2 = map(int, box.xyxy[0])

        color = BOX_COLORS.get(label, DEFAULT_COLOR)

        # Draw rectangle
        cv2.rectangle(annotated, (x1, y1), (x2, y2), color, thickness=2)

        # Label background + text
        text      = f"{label} {conf:.2f}"
        font      = cv2.FONT_HERSHEY_SIMPLEX
        font_scale = 0.55
        thickness  = 1
        (tw, th), baseline = cv2.getTextSize(text, font, font_scale, thickness)

        # Filled rectangle behind text so it's always readable
        cv2.rectangle(annotated,
                       (x1, y1 - th - baseline - 4),
                       (x1 + tw + 4, y1),
                       color, cv2.FILLED)

        # White text on coloured background
        cv2.putText(annotated, text,
                    (x1 + 2, y1 - baseline - 2),
                    font, font_scale, (255, 255, 255), thickness, cv2.LINE_AA)

    return annotated


def detection_loop():
    while True:
        for lane in LANES:
            with locks[lane]:
                frame = frames[lane]

            if frame is None:
                continue

            results = model(frame, conf=0.4, verbose=False)

            # ── Count vehicles ────────────────────────────────────────────
            count = sum(
                1 for box in results[0].boxes
                if model.names[int(box.cls[0])] in VEHICLES
            )
            counts[lane] = count

            # ── Draw boxes and store the annotated frame ──────────────────
            annotated = draw_boxes(frame, results)

            # Overlay lane name + vehicle count (top-left corner)
            lane_label = lane.replace("LANE_", "Lane ")
            cv2.putText(annotated,
                        f"{lane_label}  |  {count} vehicle{'s' if count != 1 else ''}",
                        (10, 28), cv2.FONT_HERSHEY_SIMPLEX,
                        0.7, (255, 255, 255), 2, cv2.LINE_AA)

            with ann_locks[lane]:
                annotated_frames[lane] = annotated

        time.sleep(0.1)

# =========================
# FIREBASE PUSH
# =========================

def push_to_firebase():
    while True:
        total = sum(counts.values())

        data = {
            "north": counts["LANE_N"],
            "south": counts["LANE_S"],
            "east":  counts["LANE_E"],
            "west":  counts["LANE_W"],
            "total": total,
            "timestamp": time.strftime("%H:%M:%S"),
            "emergency": {
                "north": emergency["LANE_N"],
                "south": emergency["LANE_S"],
                "east":  emergency["LANE_E"],
                "west":  emergency["LANE_W"],
            }
        }

        db.child("traffic").set(data)

        print(f"🔥 Firebase Updated: N={data['north']} S={data['south']} "
              f"E={data['east']} W={data['west']} | "
              f"Emergency: N={emergency['LANE_N']} S={emergency['LANE_S']} "
              f"E={emergency['LANE_E']} W={emergency['LANE_W']}")

        time.sleep(2)

# =========================
# FLASK API — serves ANNOTATED frames
# =========================

def generate_frames(lane):
    """Yield MJPEG stream using the annotated (boxed) frame for *lane*."""
    while True:
        # Prefer annotated frame; fall back to raw if detection hasn't run yet
        with ann_locks[lane]:
            frame = annotated_frames[lane]

        if frame is None:
            # Fall back to raw frame while YOLO hasn't processed first frame yet
            with locks[lane]:
                frame = frames[lane]

        if frame is None:
            time.sleep(0.1)
            continue

        ret, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
        if not ret:
            continue

        frame_bytes = buffer.tobytes()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')
        time.sleep(0.05)

@app.route('/api/stream/<direction>')
def video_feed(direction):
    lane_map = {
        'north': 'LANE_N',
        'south': 'LANE_S',
        'east':  'LANE_E',
        'west':  'LANE_W'
    }
    lane = lane_map.get(direction)
    if not lane:
        return "Invalid direction", 400

    return Response(generate_frames(lane),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

# =========================
# START SYSTEM
# =========================

if __name__ == "__main__":
    print("🚀 Starting system...")

    for lane, path in VIDEO_PATHS.items():
        threading.Thread(target=video_reader, args=(lane, path), daemon=True).start()

    threading.Thread(target=detection_loop, daemon=True).start()
    threading.Thread(target=push_to_firebase, daemon=True).start()
    threading.Thread(target=start_keyboard_listener, daemon=True).start()

    print("🌐 Starting Flask Server on http://0.0.0.0:5000 ...")
    app.run(host="0.0.0.0", port=5000, debug=False, use_reloader=False)
