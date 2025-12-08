from telegram import Bot
import asyncio

import cv2

async def send_message():
    cap = cv2.VideoCapture(0)
    ret, frame = cap.read()
    BOT_TOKEN = ""
    CHAT_ID = 0
    bot = Bot(token=BOT_TOKEN)
    
    if ret:
        _, buffer = cv2.imencode(".jpg", frame)
        img_bytes = buffer.tobytes()

        # await bot.send_message(chat_id=CHAT_ID, text="Alert! Someone else trying to enter your house.")
        await bot.send_photo(chat_id=CHAT_ID, photo=img_bytes)
        print("Alert sent to telegram!")
        cap.release()
    else:
        print("Not captured")

asyncio.run(send_message())
