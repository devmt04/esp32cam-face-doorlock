from telegram import Bot
import asyncio

async def send_message():
    BOT_TOKEN = "8367260642:AAHkXDWMXVsdD1ylVA1KOugSAZKdrMReJq8"
    CHAT_ID = -1003452055354
    bot = Bot(token=BOT_TOKEN)
    await bot.send_message(chat_id=CHAT_ID, text="Alert! Someone else trying to enter your house.")
    print("Alert sent to telegram!")


# asyncio.run(send_message())
