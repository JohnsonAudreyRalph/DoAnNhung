#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>  // Thư viện lấy thời gian

#define WIFI_SSID "IoT"
#define WIFI_PASSWORD "234567Cn"
#define API_KEY "AIzaSyC71pIn7RIS-_fXf-1v0y9UoA1cQaOrR34"  // Thay API Key của bạn vào đây
#define FIREBASE_HOST "test-esp8266-81b75-default-rtdb.firebaseio.com"  // Thay bằng URL Firebase
#define DATABASE_PATH "/New_data.json"  // Đường dẫn dữ liệu

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Đang kết nối WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWiFi đã kết nối!");

    // Cấu hình lấy thời gian từ NTP Server
    configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

String getFormattedTime() {
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);

    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return String(buffer);
}

String getFormattedDate() {
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);

    char buffer[11];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y", timeinfo);
    return String(buffer);
}

void sendDataToFirebase() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;
        client.setInsecure();  // Bỏ qua xác thực SSL

        HTTPClient http;
        String url = "https://" + String(FIREBASE_HOST) + DATABASE_PATH + "?auth=" + API_KEY;

        // Tạo dữ liệu JSON
        String jsonData = "{";
        jsonData += "\"time\":\"" + getFormattedTime() + "\",";
        jsonData += "\"date\":\"" + getFormattedDate() + "\",";
        jsonData += "\"random_number\":" + String(random(1, 100)) + "}";
        
        Serial.println("Đang gửi dữ liệu: " + jsonData);

        http.begin(client, url);
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.PUT(jsonData);

        if (httpResponseCode > 0) {
            Serial.printf("Gửi dữ liệu thành công! HTTP Response: %d\n", httpResponseCode);
        } else {
            Serial.printf("Lỗi gửi dữ liệu: %s\n", http.errorToString(httpResponseCode).c_str());
        }

        http.end();
    }
}

void loop() {
    sendDataToFirebase();
    delay(5000); // Gửi dữ liệu mỗi 5 giây
}
