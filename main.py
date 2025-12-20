from fastapi import FastAPI
from pydantic import BaseModel
import threading
import paho.mqtt.client as mqtt
import os
from dotenv import load_dotenv

load_dotenv()

MQTT_HOST = os.getenv("MQTT_HOST", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
DEVICE_ID = "esp32-room-node"

app = FastAPI(title="IoT Smart Gateway API")


device_state = {
    DEVICE_ID: {
        "temperature": None,
        "temperatureF": None,
        "humidity": None,
        "status": "unknown"
    }
}


def on_connect(client, userdata, flags, rc):
    print("MQTT connected:", rc)
    client.subscribe("home/esp32/temperature")
    client.subscribe("home/esp32/temperatureF")
    client.subscribe("home/esp32/humidity")
    client.subscribe("home/esp32/status")

def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    print("MQTT >", msg.topic, payload)

    if msg.topic == "home/esp32/temperature":
        device_state[DEVICE_ID]["temperature"] = float(payload)
    elif msg.topic == "home/esp32/temperatureF":
        device_state[DEVICE_ID]["temperatureF"] = float(payload)
    elif msg.topic == "home/esp32/humidity":
        device_state[DEVICE_ID]["humidity"] = float(payload)
    elif msg.topic == "home/esp32/status":
        device_state[DEVICE_ID]["status"] = payload

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

def mqtt_loop():
    mqtt_client.connect(MQTT_HOST, MQTT_PORT, 60)
    mqtt_client.loop_forever()

threading.Thread(target=mqtt_loop, daemon=True).start()


class Command(BaseModel):
    action: str
    value: str | None = None


@app.get("/devices")
def list_devices():
    return device_state

@app.get("/devices/{device_id}")
def get_device(device_id: str):
    return device_state.get(device_id, {"error": "device not found"})

@app.post("/devices/{device_id}/command")
def send_command(device_id: str, cmd: Command):
    if device_id != DEVICE_ID:
        return {"error": "unknown device"}

    if cmd.action == "led":
        mqtt_client.publish("home/esp32/led", cmd.value.upper())
        return {"sent": cmd.value}

    return {"error": "unsupported action"}
