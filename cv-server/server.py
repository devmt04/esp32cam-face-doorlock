import cv2             # for video capture and image processing
import numpy as np     # for image array operations
import time            # for time-related functions
import pygame          # for displaying video frames
import serial          # for UART communication
from insightface.app import FaceAnalysis # for face detection and recognition
from telegram import Bot # for sending Telegram messages
import asyncio         # for asynchronous operations

pygame.init()          # initialize PyGame
screen = pygame.display.set_mode((320, 240)) # set display size

ESP32_IP = "192.168.4.1"  # IP address of the ESP32-CAM
STREAM_URL = f"http://{ESP32_IP}/stream" # URL for video stream

ser = serial.Serial("/dev/ttyUSB0", 115200, write_timeout=0.5) # initialize UART
ser.dtr = False # disable DTR to prevent reset
ser.rts = False # disable RTS to prevent reset

# Function to display a video frame using PyGame
def show_frame(frame):
    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    surf = pygame.surfarray.make_surface(frame.swapaxes(0,1))
    screen.blit(surf, (0,0))
    pygame.display.update()

# Initialize face analysis model
app = FaceAnalysis(name="buffalo_l", providers=['CPUExecutionProvider'])
app.prepare(ctx_id=0, det_size=(320, 320))
target_embedding = np.load("target_embedding.npy")

# Function to send command via UART
def send_uart(cmd: str):
    ser.write((cmd + "\n").encode('utf-8'))

# Function to open video stream with retry mechanism
def open_stream(url):
    cap = None
    while cap is None or not cap.isOpened():
        print("[INFO] Connecting to stream...")
        cap = cv2.VideoCapture(url)
        if not cap.isOpened():
            print("[WARN] Failed to connect, retrying...")
            time.sleep(2)
        else:
            print("[INFO] Stream connected.")
    return cap

# Function to compute cosine similarity between two embeddings
def cosine_similarity(a, b):
    return np.dot(a, b)

# Asynchronous function to send alert message via Telegram
async def send_message(frame):
    BOT_TOKEN = "" # Bot Token here
    CHAT_ID = 0 #CHAT ID HERE
    bot = Bot(token=BOT_TOKEN)
    _, buffer = cv2.imencode(".jpg", frame)
    img_bytes = buffer.tobytes()

    await bot.send_photo(chat_id=CHAT_ID, photo=img_bytes)
    # await bot.send_message(chat_id=CHAT_ID, text="Alert! Someone else trying to enter your house.")
    print("Alert sent to telegram!")


print(f"[INFO] Connecting to ESP32-CAM stream at {STREAM_URL}")
cap = open_stream(STREAM_URL) # open video stream

if not cap.isOpened():
    print("[ERROR] Cannot open stream.")
    exit(1)

led_on = False
last_sent = 0
FACE_THRESHOLD = 10 
THRESHOLD = 0.40

# Main loop to process video frames
while True:
    ret, frame = cap.read()
    if not ret:
        print("[WARN] Failed to grab frame")
        cap.release()
        cap = open_stream(STREAM_URL)
        continue

    faces = app.get(frame) # detect faces in the frame
    for face in faces:
        emb = face.normed_embedding # get face embedding
        sim = cosine_similarity(emb, target_embedding) # compute similarity

        x1, y1, x2, y2 = face.bbox.astype(int) # get bounding box

        color = (0, 255, 0) if sim > (1 - THRESHOLD) else (0, 0, 255) # green for match, red for unknown

        # Draw bounding box and label
        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
        label = f"{'MATCH' if sim > (1 - THRESHOLD) else 'UNKNOWN'}  {sim:.2f}"
        cv2.rectangle(frame, (x1, y1 - 20), (x1 + 200, y1), color, -1)
        cv2.putText(frame, label, (x1 + 5, y1 - 5),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5,
                    (255, 255, 255), 1)

        # Take actions based on similarity
        if(sim > (1 - THRESHOLD))  and time.time() - last_sent > 10:
            last_sent = time.time()
            print("[ACTION] Face detected → LED ON")
            send_uart("face_detected")
        elif (sim <= (1 - THRESHOLD))  and time.time() - last_sent > 10:
            last_sent = time.time()
            #asyncio.run(send_message(frame))

    show_frame(frame) # display the frame

    # PyGame exit handling
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            exit()
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE:
                pygame.quit()
                exit()


# Cleanup
print("[INFO] Closing stream...")
cap.release()
cv2.destroyAllWindows()
send_uart("led_off")
