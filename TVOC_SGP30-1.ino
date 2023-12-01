#include <M5StickCPlus.h>
#include "Adafruit_SGP30.h"
#include <QubitroMqttClient.h>
#include <WiFi.h>
#include <BLEDevice.h>

Adafruit_SGP30 sgp;

int i = 15;
long last_millis = 0;

// WiFi Client
WiFiClient wifiClient;
// Qubitro Client
QubitroMqttClient mqttClient(wifiClient);

// Device Parameters
char deviceID[] = "828f3f85-7fbb-43fe-8c0f-6f0504372e8f";
char deviceToken[] = "KctoyvLJapEzD9V4lTOGlzuoYtMUFKlcI0g7gtvo";

// WiFi Parameters
const char *ssid = "Rio_West_Students";
const char *password = "stove914wooden";

// const char *ssid = "pixel";
// const char *password = "12345678";

// const char *ssid = "utexas-iot";
// const char *password = "52756315611727283607";

// Sound threshold
const int soundThreshold = 1000;

void header(const char *string, uint16_t color) {
    M5.Lcd.fillScreen(color);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.fillRect(0, 0, 120, 20, TFT_BLACK);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString(string, 120, 2, 4);
}

void setup() {
    M5.begin(true, false, false);
    Wire.begin(32, 33);
    M5.Lcd.setRotation(3);
    header("SGP30 TEST", TFT_BLACK);
    Serial.begin(115200);
    Serial.println("SGP30 test");
    if (!sgp.begin()) {
        Serial.println("Sensor not found :(");
        while (1);
    }

    M5.Lcd.drawString("TVOC:", 40, 42, 4);
    M5.Lcd.drawString("eCO2:", 40, 76, 4);
    Serial.print("Found SGP30 serial #");
    Serial.print(sgp.serialnumber[0], HEX);
    Serial.print(sgp.serialnumber[1], HEX);
    Serial.println(sgp.serialnumber[2], HEX);
    M5.Lcd.drawString("Initialization", 150, 110, 4);

    wifi_init();
    qubitro_init();
}

void loop() {
    mqttClient.poll(); // Handle MQTT communication

    while (i > 0) {
        if (millis() - last_millis > 1000) {
            last_millis = millis();
            i--;
            M5.Lcd.fillRect(45, 110, 30, 25, TFT_BLACK);
            M5.Lcd.drawNumber(i, 60, 110, 4);
        }
    }
    M5.Lcd.fillRect(45, 110, 180, 25, TFT_BLACK);

    if (!sgp.IAQmeasure()) {
        Serial.println("Measurement failed");
        return;
    }
    M5.Lcd.fillRect(85, 42, 75, 68, TFT_BLACK);
    M5.Lcd.drawNumber(sgp.TVOC, 120, 42, 4);
    M5.Lcd.drawString("ppb", 200, 42, 4);
    M5.Lcd.drawNumber(sgp.eCO2, 120, 76, 4);
    M5.Lcd.drawString("ppm", 200, 76, 4);

    Serial.print("TVOC ");
    Serial.print(sgp.TVOC);
    Serial.print(" ppb\t");
    Serial.print("eCO2 ");
    Serial.print(sgp.eCO2);
    Serial.println(" ppm");

    // Send data to Qubitro
    String payload = "{\"temp\":" + String(sgp.TVOC) + ", \"Key2\":" + String(sgp.eCO2) + "}";
    mqttClient.beginMessage(deviceID);
    mqttClient.print(payload);
    mqttClient.endMessage();

    // Sound alert if TVOC level is beyond the threshold
    if (sgp.TVOC > soundThreshold) {
        M5.Lcd.setTextColor(RED);
        M5.Lcd.setCursor(0, 100);
        M5.Lcd.printf("ALERT: TVOC exceeds threshold");
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.setCursor(0, 110);
        M5.Lcd.printf("                     "); // Clear previous text
        M5.Lcd.setCursor(0, 120);
        M5.Lcd.printf("                     "); // Clear previous text

        // Turn on the speaker
        M5.Beep.tone(4000);
    } else {
        // Turn off the speaker
        M5.Beep.mute();
    }
    delay(1000); // Adjust based on the SGP30 measurement frequency
}

void wifi_init() {
    // Set WiFi mode
    WiFi.mode(WIFI_STA);
    // Disconnect WiFi
    WiFi.disconnect();
    delay(100);
    // Initiate WiFi connection
    WiFi.begin(ssid, password);
    // Print connectivity status to the terminal
    Serial.print("Connecting to WiFi...");
    while (true) {
        delay(1000);
        Serial.print(".");
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("");
            Serial.println("WiFi Connected.");
            Serial.print("Local IP: ");
            Serial.println(WiFi.localIP());
            Serial.print("RSSI: ");
            Serial.println(WiFi.RSSI());
            M5.Lcd.setTextColor(RED);
            M5.Lcd.setCursor(0, 8);
            M5.Lcd.printf("Network Connected");
            break;
        }
    }
}

void qubitro_init() {
    char host[] = "broker.qubitro.com";
    int port = 1883;

    mqttClient.setId(deviceID);
    mqttClient.setDeviceIdToken(deviceID, deviceToken);

    Serial.println("Connecting to Qubitro...");
    
    if (!mqttClient.connect(host, port)) {
        Serial.print("Connection failed. Error code: ");
        Serial.println(mqttClient.connectError());
        Serial.println("Visit docs.qubitro.com or create a new issue on github.com/qubitro");
    } else {
        Serial.println("Connected to Qubitro.");
        M5.Lcd.setTextColor(RED);
        M5.Lcd.setCursor(0, 25);
        M5.Lcd.printf("Uplink Established");

        // Subscribe to device topics (if needed)
        mqttClient.subscribe(deviceID);
    }
    
    // Wait for a moment before proceeding
    delay(2000);
    M5.Lcd.fillScreen(BLACK);
}
