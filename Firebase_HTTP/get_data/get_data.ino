#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define WIFI_SSID "IoT"
#define WIFI_PASSWORD "234567Cn"
#define API_KEY "AIzaSyD1zbLwTH6sraUU7wcsJEQLfu7XjIdtGnI"  // Thay API Key của bạn vào đây
#define FIREBASE_HOST "esp8266-178db-default-rtdb.asia-southeast1.firebasedatabase.app"  // Thay bằng URL Firebase
#define DATABASE_PATH "/Topic.json"  // Đường dẫn đến dữ liệu trong Firebase

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Đang kết nối WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWiFi đã kết nối!");
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;  // Sử dụng WiFiClientSecure thay vì WiFiClient
        client.setInsecure();  // Bỏ qua xác thực SSL

        HTTPClient http;
        String url = "https://" + String(FIREBASE_HOST) + DATABASE_PATH + "?auth=" + API_KEY;
        
        // Serial.println("Đang lấy dữ liệu từ: " + url);

        http.begin(client, url);  // Sử dụng WiFiClientSecure
        int httpCode = http.GET();

        if (httpCode == 200) {
            // Serial.printf("HTTP Response code: %d\n", httpCode);
            String payload = http.getString();
            Serial.println("Dữ liệu nhận được: " + payload);
        } else {
            Serial.printf("Lỗi HTTP: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    delay(5000); // Lặp lại sau 5 giây
}