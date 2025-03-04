#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
// Khai báo chân điều khiển động cơ
int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 14;
// Cấu hình PWM
const int pwmChannel = 0;
const int resolution = 8;  // Độ phân giải 8-bit (từ 0 đến 255)
int dutyCycle = 200;
// Biến lưu trữ status và tốc độ của động cơ
int motorStatus = 0; // 0: Dừng, 1: Đang chạy
int motorSpeed = 0;  // Tốc độ từ 0 đến 100%

// Cấu hình WiFi
const char* ssid = "IoT";
const char* password = "234567Cn";

// Cấu hình MQTT
const char* mqttServer = "192.168.0.101";
const int mqttPort = 1883;               // Cổng MQTT
const char* mqttTopic = "MQTT_DC_DongCo"; // Topic để gửi dữ liệu

WiFiClient espClient;
PubSubClient client(espClient);

// Cấu hình NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // GMT+7 (điều chỉnh múi giờ)

// Hàm di chuyển động cơ về phía trước
void moveForward() {
  Serial.println("Đã chạy");
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  motorStatus = 1;  // Động cơ đang chạy
  startMotor();
}

// Hàm dừng động cơ
void stopMotor() {
  Serial.println("Đã dừng");
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  motorStatus = 0;  // Động cơ dừng
  // Không reset motorSpeed về 0, giữ lại giá trị cũ
  ledcWrite(pwmChannel, 0);
}

// Hàm bắt đầu động cơ với giá trị PWM đã thiết lập
void startMotor() {
  ledcWrite(pwmChannel, dutyCycle);
}

// Hàm cập nhật tốc độ động cơ theo giá trị nhập vào (từ 0 đến 100)
void setSpeed(int speedPercent) {
  dutyCycle = map(speedPercent, 0, 100, 0, 255);  // Chuyển từ % sang giá trị PWM (0 đến 255)
  motorSpeed = speedPercent;  // Cập nhật tốc độ động cơ
  if (motorStatus == 1) {  // Nếu động cơ đang chạy, thì cập nhật tốc độ
    startMotor();  // Cập nhật tốc độ cho động cơ
  }
}

// Hàm kết nối WiFi
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Kết nối tới ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("Địa chỉ IP ESP32: ");
  Serial.println(WiFi.localIP());
}

// Hàm callback khi nhận dữ liệu từ MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nhận TOPIC: ");
  Serial.print(topic);
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  // Phân tích JSON
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("Lỗi!: ");
    Serial.println(error.f_str());
    return;
  }
  // Trích xuất dữ liệu từ JSON
  String receivedDate = doc["date"];
  String receivedTime = doc["time"];
  String status = doc["status"];
  String speedStr = doc["speed"];
  if (status == "1") {
    Serial.println("Đã bật động cơ");
    moveForward();  // Di chuyển về phía trước
  } 
  else if (status == "0") {
    Serial.println("Đã tắt động cơ");
    stopMotor();  // Dừng động cơ
  }
  int speed_int = speedStr.toInt();
  if (speed_int >= 0 && speed_int <= 100) {
    setSpeed(speed_int);  // Cập nhật tốc độ nếu trong khoảng hợp lệ
    Serial.println("Tốc độ mới cập nhật: " + speed_int);
  }
}

// Hàm kết nối MQTT
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Đang kết nối tới MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(mqttTopic);
    } else {
      Serial.print("Lỗi!, rc=");
      Serial.print(client.state());
      Serial.println(" thực hiện kết nối lại sau 5s");
      delay(5000);
    }
  }
}

// Hàm chuyển đổi epoch time thành định dạng ngày dd/mm/yyyy
String formatDate(unsigned long epochTime) {
  int days = epochTime / 86400; // Một ngày có 86400 giây
  int year = 1970;
  // Tính năm
  while (days >= 365) {
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) { // Năm nhuận
      if (days >= 366) {
        days -= 366;
        year++;
      } else {
        break;
      }
    } else {
      days -= 365;
      year++;
    }
  }
  // Tính tháng
  int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
    monthDays[1] = 29; // Năm nhuận, tháng 2 có 29 ngày
  }
  int month = 0;
  while (days >= monthDays[month]) {
    days -= monthDays[month];
    month++;
  }
  month++;
  int day = days + 1;
  // Định dạng ngày
  char dateBuffer[11];
  snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%04d", day, month, year);
  return String(dateBuffer);
}

void setup() {
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  // Cấu hình PWM cho pin enable
  ledcSetup(pwmChannel, 5000, resolution);  // Khởi tạo kênh PWM (5000 Hz và độ phân giải 8-bit)
  ledcAttachPin(enable1Pin, pwmChannel);    // Gắn kênh PWM vào chân enable
  Serial.begin(115200);
  setupWiFi();
  client.setServer(mqttServer, mqttPort); // Thiết lập MQTT với địa chỉ IP
  client.setCallback(callback);
  timeClient.begin(); // Bắt đầu lấy thời gian
}

void loop() {
  int currentStatus = motorStatus;
  int currentSpeed = motorSpeed;
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();
  timeClient.update();
  // Lấy epoch time
  unsigned long epochTime = timeClient.getEpochTime();
  // Định dạng ngày và giờ
  String formattedDate = formatDate(epochTime);       // Định dạng ngày dd/mm/yyyy
  String time = timeClient.getFormattedTime();        // Định dạng giờ HH:MM:SS
  // Gửi dữ liệu qua MQTT mỗi 5 giây
  static unsigned long lastMsg = 0;
   if (millis() - lastMsg > 3000) {
    lastMsg = millis();
    String message = "{\"date\":\"" + formattedDate + "\",\"time\":\"" + time + "\",\"speed\":\"" + String(currentSpeed) + "%\",\"status\":\"" + currentStatus + "\"}";
    Serial.print("Sending message: ");
    Serial.println(message);
    client.publish(mqttTopic, message.c_str());


    // String date_str = String(mqttTopic) + "/date";
    // client.publish(date_str.c_str(), formattedDate.c_str(), true);
    // String time_str = String(mqttTopic) + "/time";
    // client.publish(time_str.c_str(), time.c_str(), true);
    // String speed_str = String(mqttTopic) + "/speed";
    // client.publish(speed_str.c_str(), String(currentSpeed).c_str(), true);
    // String status_str = String(mqttTopic) + "/status";
    // client.publish(status_str.c_str(), String(currentStatus).c_str(), true);
  }
  // delay(2000);
}