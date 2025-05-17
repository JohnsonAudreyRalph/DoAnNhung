#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "base64.h"  // Thư viện mã hóa Base64
#define CAMERA_MODEL_AI_THINKER 
// ===========================
// Thông tin WiFi
// ===========================
const char* ssid = "BM_CNTT_410";
const char* password = "234567Cn";

// ===========================
// Thông tin MQTT
// ===========================
const char* mqtt_server = "192.168.110.150";  // Thay bằng địa chỉ broker MQTT của bạn
const int mqtt_port = 1883;
const char* mqtt_topic = "Camera_vip";  // Topic MQTT để gửi ảnh

void startCameraServer();
void setupLedFlash(int pin);

WiFiClient espClient;
PubSubClient client(espClient);

void reconnect() {
  while (!client.connected()) {
    Serial.print("Kết nối MQTT...");
    if (client.connect("ESP32-CAM")) {
      Serial.println(" Thành công!");
    } else {
      Serial.print(" Thất bại, mã lỗi: ");
      Serial.print(client.state());
      Serial.println(" -> Thử lại sau 5 giây...");
      delay(5000);
    }
  }
}
void clearallData() {
  Serial.println("🗑 Đang xóa dữ liệu cũ trên MQTT...");

  // Xóa tín hiệu ảnh hoàn tất
  String end_topic = String(mqtt_topic) + "/end";
  client.publish(end_topic.c_str(), "", true);  

  // Xóa toàn bộ chunk cũ 
  for (int i = 1; i <= 1000; i++) {  
    String old_chunk_topic = String(mqtt_topic) + "/chunk_" + i;
    client.publish(old_chunk_topic.c_str(), "", true);
  }

  Serial.println("✅ Đã xóa xong dữ liệu cũ.");
}
int last_chunk_count = 0;  
void clearOldData() {
  Serial.println("🗑 Đang xóa dữ liệu cũ trên MQTT...");

  // Xóa tín hiệu ảnh hoàn tất
  String end_topic = String(mqtt_topic) + "/end";
  client.publish(end_topic.c_str(), "", true);  

  // Xóa toàn bộ chunk cũ 
  for (int i = 1; i <= last_chunk_count; i++) {  
    String old_chunk_topic = String(mqtt_topic) + "/chunk_" + i;
    client.publish(old_chunk_topic.c_str(), "", true);
  }

  Serial.println("✅ Đã xóa xong dữ liệu cũ.");
}

void sendPhoto() {
  Serial.println("📸 Chụp ảnh...");
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Chụp ảnh thất bại!");
    sendPhoto();
  }

  // Xóa dữ liệu cũ trước khi gửi ảnh mới
  clearOldData();

  // Mã hóa ảnh thành Base64
  Serial.println("🔄 Mã hóa ảnh sang Base64...");
  String base64Image = base64::encode(fb->buf, fb->len);
  esp_camera_fb_return(fb); // Giải phóng bộ nhớ ngay sau khi mã hóa

  Serial.println("📤 Gửi dữ liệu ảnh mới qua MQTT...");

  const size_t CHUNK_SIZE = 150;
  size_t total_size = base64Image.length();
  int total_chunks = (total_size + CHUNK_SIZE - 1) / CHUNK_SIZE; // Tính tổng số chunk cần gửi
  size_t sent_bytes = 0;
  int chunk_index = 1;

  while (sent_bytes < total_size) {
    size_t send_size = min(CHUNK_SIZE, total_size - sent_bytes);
    String topic = String(mqtt_topic) + "/chunk_" + chunk_index ;
    String payload = base64Image.substring(sent_bytes, sent_bytes + send_size);

    if (client.publish(topic.c_str(), payload.c_str(), true)) {  // Gửi với retained flag
      Serial.printf("✅ Gửi phần %d/%d thành công! (%d/%d bytes)\n", chunk_index , total_chunks, sent_bytes + send_size, total_size);
    } else {
      Serial.println("❌ Gửi thất bại!");
      break;
    }
    sent_bytes += send_size;
    chunk_index++;
  }

  last_chunk_count = total_chunks; 

  // Gửi tín hiệu báo hiệu ảnh đã gửi hoàn tất
  String end_topic = String(mqtt_topic) + "/end";
  client.publish(end_topic.c_str(), "done", true);

  String Count_chu = String(mqtt_topic) + "/MAX";
  char chunk_count_str[10];  
  itoa(last_chunk_count, chunk_count_str, 10); 
  client.publish(Count_chu.c_str(), chunk_count_str, true);
  Serial.println("🏁 Ảnh đã gửi hoàn tất!");
}


void setup() {
  Serial.begin(115200);
  Serial.println();

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi đã kết nối!");

  // Cấu hình MQTT
  client.setServer(mqtt_server, mqtt_port);

  // ===========================
  // Cấu hình Camera (GIỮ NGUYÊN GPIO CỦA BẠN)
  // ===========================
  #if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#endif

camera_config_t config;
config.ledc_channel = LEDC_CHANNEL_0;
config.ledc_timer = LEDC_TIMER_0;
config.pin_d0 = Y2_GPIO_NUM;
config.pin_d1 = Y3_GPIO_NUM;
config.pin_d2 = Y4_GPIO_NUM;
config.pin_d3 = Y5_GPIO_NUM;
config.pin_d4 = Y6_GPIO_NUM;
config.pin_d5 = Y7_GPIO_NUM;
config.pin_d6 = Y8_GPIO_NUM;
config.pin_d7 = Y9_GPIO_NUM;
config.pin_xclk = XCLK_GPIO_NUM;
config.pin_pclk = PCLK_GPIO_NUM;
config.pin_vsync = VSYNC_GPIO_NUM;
config.pin_href = HREF_GPIO_NUM;
config.pin_sccb_sda = SIOD_GPIO_NUM;
config.pin_sccb_scl = SIOC_GPIO_NUM;
config.pin_pwdn = PWDN_GPIO_NUM;
config.pin_reset = RESET_GPIO_NUM;
config.xclk_freq_hz = 20000000;
config.pixel_format = PIXFORMAT_JPEG; 
config.frame_size = FRAMESIZE_SVGA;
config.jpeg_quality = 20;
config.fb_count = 1;


  if (psramFound()) {
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // Khởi động Camera
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("❌ Camera init thất bại!");
    return;
  }

  Serial.println("📷 Camera đã sẵn sàng!");
  clearallData();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  sendPhoto();  // Gửi ảnh mỗi vòng lặp
  delay(5000); // Gửi mỗi 5 giây
}
