import base64
from flask import Flask, render_template, send_from_directory, jsonify
import paho.mqtt.client as mqtt
import os
from datetime import datetime
import threading

# Các thông số MQTT
MQTT_BROKER = "192.168.0.100"
MQTT_PORT = 1883
MQTT_TOPIC = "esp32/cam/image"

# Dữ liệu buffer để lưu ảnh
image_data = ""
total_parts = 0
received_parts = 0
UPLOAD_FOLDER = "./uploads"  # Thư mục lưu ảnh
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

# Biến lưu tên ảnh hiện tại để hiển thị trong web
current_image = ""

def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    global image_data, total_parts, received_parts, current_image
    message = msg.payload.decode()

    if message == "end":
        print("End of image transmission received.")
        print("Final concatenated image data (Base64):")
        print(image_data)

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        file_name = f"image_{timestamp}.jpg"
        file_path = os.path.join(UPLOAD_FOLDER, file_name)

        if not image_data:
            print("No image data to decode!")
            return
        try:
            image_bytes = base64.b64decode(image_data)
            with open(file_path, "wb") as img_file:
                img_file.write(image_bytes)
            print(f"Image saved as {file_name}")
            current_image = file_name
        except Exception as e:
            print(f"Error decoding base64 data: {e}")
        image_data = ""
        received_parts = 0
        total_parts = 0
    else:
        try:
            if '/' in message:
                index, rest = message.split('/', 1)
                index = int(index)
                part_data = rest.split(':', 1)[1]
                if received_parts == 0:
                    total_parts = int(rest.split('/', 1)[0].split(":")[0])
                image_data += part_data
                received_parts += 1
                # print(f"Received part {index}/{total_parts}")
            else:
                print(f"Received invalid message: {message}")
        except Exception as e:
            print(f"Error processing message: {e}")

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/uploads/<filename>')
def uploaded_file(filename):
    return send_from_directory(UPLOAD_FOLDER, filename)

@app.route('/current-image')
def current_image_route():
    return jsonify({'image_name': current_image})

def mqtt_loop():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()

if __name__ == "__main__":
    mqtt_thread = threading.Thread(target=mqtt_loop)
    mqtt_thread.start()

    app.run(host="0.0.0.0", port=5000, debug=True)