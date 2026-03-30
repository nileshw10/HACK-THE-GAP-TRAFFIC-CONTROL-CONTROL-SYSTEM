"""
SMART TRAFFIC MANAGEMENT — VEHICLE COUNT ONLY
4 ESP32-CAM → YOLO Vehicle Detection → Arduino Mega
"""

import cv2
import time
import threading
import numpy as np
import serial
import requests
import torch
import os
from ultralytics import YOLO

# ═══════════════════════════════════════════════
# CONFIGURATION
# ═══════════════════════════════════════════════

CAMERA_URLS = {
    "LANE_N": "http://10.81.49.239/stream",
    "LANE_E": "http://10.81.49.142/stream",
    "LANE_S": "http://10.81.49.153/stream",
    "LANE_W": "http://10.81.49.110/stream ",
}

LANE_ORDER = ["LANE_N","LANE_E","LANE_S","LANE_W"]
LANE_CODES = {"LANE_N":"N","LANE_E":"E","LANE_S":"S","LANE_W":"W"}

SERIAL_PORT="COM3"
SERIAL_BAUD=9600

YOLO_MODEL="yolov8n.pt"

YOLO_IMGSZ=320
YOLO_CONF=0.45
YOLO_IOU=0.45

VEHICLE_CLASS_IDS=[2,3,5,7]
VEHICLE_CLASS_NAMES={"car","motorcycle","bus","truck"}

PANEL_W=480
PANEL_H=360

SEND_INTERVAL=2.0


# ═══════════════════════════════════════════════
# DEVICE
# ═══════════════════════════════════════════════

DEVICE="cuda:0" if torch.cuda.is_available() else "cpu"
print("Device:",DEVICE)


# ═══════════════════════════════════════════════
# LOAD YOLO MODEL
# ═══════════════════════════════════════════════

print("Loading YOLO model...")
vehicle_model=YOLO(YOLO_MODEL)
vehicle_model.fuse()

if DEVICE!="cpu":
    vehicle_model.to(DEVICE)

print("Model ready")


# ═══════════════════════════════════════════════
# SHARED STATE
# ═══════════════════════════════════════════════

latest_frames={lane:None for lane in LANE_ORDER}
annotated_frames={lane:None for lane in LANE_ORDER}
counts={lane:0 for lane in LANE_ORDER}

frame_locks={lane:threading.Lock() for lane in LANE_ORDER}

display_lock=threading.Lock()
result_lock=threading.Lock()
serial_lock=threading.Lock()


# ═══════════════════════════════════════════════
# SERIAL
# ═══════════════════════════════════════════════

try:
    ser=serial.Serial(SERIAL_PORT,SERIAL_BAUD,timeout=1)
    time.sleep(2)
    print("Arduino connected")
except:
    ser=None
    print("Serial not connected")


def send_batch(commands):
    if ser and ser.is_open:
        payload="".join(f"<{c}>\n" for c in commands)
        with serial_lock:
            ser.write(payload.encode())


# ═══════════════════════════════════════════════
# CAMERA THREAD
# ═══════════════════════════════════════════════

def mjpeg_reader(lane,url):

    while True:

        try:
            r=requests.get(url,stream=True,timeout=10)

            buf=bytearray()

            for chunk in r.iter_content(8192):

                buf.extend(chunk)

                a=buf.find(b'\xff\xd8')
                b=buf.find(b'\xff\xd9')

                if a!=-1 and b!=-1:

                    jpg=bytes(buf[a:b+2])
                    del buf[:b+2]

                    frame=cv2.imdecode(
                        np.frombuffer(jpg,np.uint8),
                        cv2.IMREAD_COLOR)

                    if frame is not None:

                        with frame_locks[lane]:
                            latest_frames[lane]=frame

        except:
            time.sleep(2)


# ═══════════════════════════════════════════════
# DETECTION THREAD
# ═══════════════════════════════════════════════

def detection_worker():

    while True:

        batch_frames=[]
        batch_lanes=[]

        for lane in LANE_ORDER:

            with frame_locks[lane]:
                f=latest_frames[lane]

            if f is None:
                continue

            batch_frames.append(f)
            batch_lanes.append(lane)

        if not batch_frames:
            time.sleep(0.05)
            continue

        results=vehicle_model(
            batch_frames,
            imgsz=YOLO_IMGSZ,
            conf=YOLO_CONF,
            iou=YOLO_IOU,
            classes=VEHICLE_CLASS_IDS,
            device=DEVICE,
            verbose=False)

        for i,(lane,frame) in enumerate(zip(batch_lanes,batch_frames)):

            vehicle_count=0

            for box in results[i].boxes:

                cls=int(box.cls[0])
                label=vehicle_model.names[cls]

                if label in VEHICLE_CLASS_NAMES:
                    vehicle_count+=1

            annotated=frame.copy()

            for box in results[i].boxes:

                cls=int(box.cls[0])
                label=vehicle_model.names[cls]

                if label in VEHICLE_CLASS_NAMES:

                    x1,y1,x2,y2=map(int,box.xyxy[0])
                    conf=float(box.conf[0])

                    cv2.rectangle(
                        annotated,
                        (x1,y1),
                        (x2,y2),
                        (0,255,0),
                        2)

                    cv2.putText(
                        annotated,
                        f"{label} {conf:.2f}",
                        (x1,y1-5),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        0.5,
                        (0,255,0),
                        1)

            cv2.putText(
                annotated,
                f"{lane} Count: {vehicle_count}",
                (10,30),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.8,
                (0,255,128),
                2)

            with display_lock:
                annotated_frames[lane]=annotated

            with result_lock:
                counts[lane]=vehicle_count


# ═══════════════════════════════════════════════
# TRAFFIC CONTROLLER
# ═══════════════════════════════════════════════

def traffic_controller():

    while True:

        time.sleep(SEND_INTERVAL)

        with result_lock:
            c=dict(counts)

        commands=[]

        for lane,lcode in LANE_CODES.items():
            commands.append(f"COUNT:{lcode}:{c[lane]}")

        commands.append(f"DENSITY:{sum(c.values())}")

        send_batch(commands)


# ═══════════════════════════════════════════════
# DISPLAY
# ═══════════════════════════════════════════════

def display_loop():

    while True:

        with display_lock:
            frames=dict(annotated_frames)

        grid=np.zeros((PANEL_H*2,PANEL_W*2,3),dtype=np.uint8)

        for i,lane in enumerate(LANE_ORDER):

            row=(i//2)*PANEL_H
            col=(i%2)*PANEL_W

            f=frames[lane]

            if f is None:
                panel=np.zeros((PANEL_H,PANEL_W,3),dtype=np.uint8)
            else:
                panel=cv2.resize(f,(PANEL_W,PANEL_H))

            grid[row:row+PANEL_H,col:col+PANEL_W]=panel

        cv2.imshow("Traffic Monitor",grid)

        if cv2.waitKey(30)==27:
            break

    cv2.destroyAllWindows()


# ═══════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════

if __name__=="__main__":

    for lane,url in CAMERA_URLS.items():

        threading.Thread(
            target=mjpeg_reader,
            args=(lane,url),
            daemon=True).start()

    threading.Thread(target=detection_worker,daemon=True).start()
    threading.Thread(target=traffic_controller,daemon=True).start()

    display_loop()