#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Thông tin Wi-Fi
#define WIFI_SSID "IoT"
#define WIFI_PASSWORD "234567Cn"

// Thông tin Firebase
#define API_KEY "AIzaSyC71pIn7RIS-_fXf-1v0y9UoA1cQaOrR34"
#define DATABASE_URL "https://test-esp8266-81b75-default-rtdb.firebaseio.com/"

// Đối tượng Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// NTP Client để lấy thời gian
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // UTC+7 cho Việt Nam

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
  Serial.begin(115200);
  
  // Kết nối Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Khởi động NTP Client
  timeClient.begin();

  // Cấu hình Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  // Đăng ký ẩn danh
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase signup OK");
    signupOK = true;
  } else {
    Serial.printf("Signup failed: %s\n", config.signer.signupError.message.c_str());
  }
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Cập nhật thời gian từ NTP
    timeClient.update();

    // Lấy thời gian dạng hh:mm:ss
    String currentTime = timeClient.getFormattedTime(); // Định dạng "hh:mm:ss"

    // Lấy ngày tháng dạng dd/mm/yyyy
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);
    String currentDate = String(ptm->tm_mday) + "/" + String(ptm->tm_mon + 1) + "/" + String(ptm->tm_year + 1900);

    // Tạo số ngẫu nhiên (0-100)
    int randomNumber = random(0, 101);

    String Topic = "/Topic/";

    // Gửi dữ liệu lên Firebase
    Firebase.RTDB.setString(&fbdo, Topic + "time", currentTime);
    Firebase.RTDB.setString(&fbdo, Topic + "date", currentDate);
    Firebase.RTDB.setInt(&fbdo, Topic + "random", randomNumber);


    // if (Firebase.RTDB.setString(&fbdo, "/CamBien/time", currentTime)) {
    //   Serial.println("Sent time: " + currentTime);
    // } else {
    //   Serial.println("Failed to send time: " + fbdo.errorReason());
    // }

    // if (Firebase.RTDB.setString(&fbdo, "/CamBien/date", currentDate)) {
    //   Serial.println("Sent date: " + currentDate);
    // } else {
    //   Serial.println("Failed to send date: " + fbdo.errorReason());
    // }

    // if (Firebase.RTDB.setInt(&fbdo, "/CamBien/random", randomNumber)) {
    //   Serial.println("Sent random: " + String(randomNumber));
    // } else {
    //   Serial.println("Failed to send random: " + fbdo.errorReason());
    // }
  }
}