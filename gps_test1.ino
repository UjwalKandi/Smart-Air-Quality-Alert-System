#include <M5StickCPlus.h>
#include <TinyGPS++.h>
#include <QubitroMqttClient.h>
#include <WiFi.h>

TinyGPSPlus gps;
HardwareSerial ss(2);
float Lat, Lng;
String lat_str, lng_str;
int sat, battery = 0;
float b, c = 0;

// WiFi Client
WiFiClient wifiClient;
// Qubitro Client
QubitroMqttClient mqttClient(wifiClient);

// Device Parameters
char deviceID[] = "bfc2413b-58f9-418b-91b0-5899d97f71c9";
char deviceToken[] = "dbbXO80Ew4G5vUf-EiZqbF8RWhTeukKfVlafTG7j";

// WiFi Parameters
const char *ssid = "Rio_West_Students";
const char *password = "stove914wooden";

// const char *ssid = "pixel";
// const char *password = "12345678";

// const char *ssid = "utexas-iot";
// const char *password = "52756315611727283607";

void setup()
{
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setSwapBytes(true);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(7, 20, 2);
  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  ss.begin(9600, SERIAL_8N1, 33, 32);
  wifi_init();
  qubitro_init();
}

void loop()
{
  while (ss.available() > 0)
    if (gps.encode(ss.read()))
    {
      if (gps.location.isValid())
      {
        // Display GPS data on the LCD
        M5.Lcd.setTextColor(RED);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.printf("</Qubitro Uplink Done/>");
        Lat = gps.location.lat();
        lat_str = String(Lat, 8); // Increase precision
        Lng = gps.location.lng();
        lng_str = String(Lng, 8); // Increase precision
        sat = gps.satellites.value();
        Serial.print("satellites found:  ");
        Serial.println(sat);
        Serial.print("latitude :  ");
        Serial.println(lat_str);
        M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        M5.Lcd.setCursor(0, 20);
        M5.Lcd.printf("latitude : %s", lat_str);
        Serial.print("longitude: ");
        Serial.println(lng_str);
        //*Just for example
        Serial.print("Alt: ");
        Serial.println(gps.altitude.feet());
        Serial.print("Course: ");
        Serial.println(gps.course.deg());
        Serial.print("Speed: ");
        Serial.println(gps.speed.mph());
        Serial.print("Date: ");
        printDate();
        Serial.print("Time: ");
        printTime();
        M5.Lcd.setCursor(0, 40);
        M5.Lcd.printf("longitude : %s", lng_str);

        // Construct MQTT payload
        String payload = "{\"latitude\": " + String(lat_str) + ",\"longitude\":" + String(lng_str) + "}";

        // Check MQTT connection status
        if (mqttClient.connected())
        {
          // Send payload if MQTT is connected
          mqttClient.beginMessage(deviceID);
          mqttClient.print(payload);
          mqttClient.endMessage();
        }
        else
        {
          // Attempt to reconnect to MQTT
          Serial.print("MQTT Connection Status: Disconnected. Reconnecting...");
          delay(5000); // Wait for 5 seconds before attempting to reconnect
          if (mqttClient.connect("broker.qubitro.com", 1883))
          {
            Serial.println("Reconnected to MQTT.");
            M5.Lcd.setTextColor(RED);
            M5.Lcd.setCursor(0, 25);
            M5.Lcd.printf("Uplink Re-established");
            mqttClient.subscribe(deviceID);
            delay(2000);
            M5.Lcd.fillScreen(BLACK);
          }
          else
          {
            Serial.print("MQTT Reconnection failed. Error code: ");
            Serial.println(mqttClient.connectError());
          }
        }

        // Display satellites count on the LCD
        M5.Lcd.setCursor(0, 60);
        M5.Lcd.printf("satellites : %d", sat);

        // Introduce delay before the next loop iteration
        delay(5000);

        // Clear LCD color
        M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
      }
    }
}


void wifi_init()
{
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  // Disconnect WiFi
  WiFi.disconnect();
  delay(100);
  // Initiate WiFi connection
  WiFi.begin(ssid, password);
  // Print connectivity status to the terminal
  Serial.print("Connecting to WiFi...");
  while (true)
  {
    delay(1000);
    Serial.print(".");
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("");
      Serial.println("WiFi Connected.");
      Serial.print("Local IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("RSSI: ");
      Serial.println(WiFi.RSSI());
      M5.Lcd.setTextColor(RED);
      M5.Lcd.setCursor(0, 8);
      Serial.println("Network Connected");
      break;
    }
  }
}

void qubitro_init()
{
  Serial.println("Connecting to Qubitro...");

  char host[] = "broker.qubitro.com"; // Use a public MQTT broker for testing
  int port = 1883;

  mqttClient.setId(deviceID);
  mqttClient.setDeviceIdToken(deviceID, deviceToken);

  if (!mqttClient.connect(host, port))
  {
    Serial.print("Connection failed. Error code: ");
    Serial.println(mqttClient.connectError());
    Serial.println("Visit docs.qubitro.com or create a new issue on github.com/qubitro");
  }
  else
  {
    Serial.println("Connected to Qubitro.");
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setCursor(0, 25);
    M5.Lcd.printf("Uplink Established");
    mqttClient.subscribe(deviceID);
    delay(2000);
    M5.Lcd.fillScreen(BLACK);
  }
}


void printDate()
{
  Serial.print(gps.date.day());
  Serial.print("/");
  Serial.print(gps.date.month());
  Serial.print("/");
  Serial.println(gps.date.year());
}

void printTime()
{
  Serial.print(gps.time.hour());
  Serial.print(":");
  if (gps.time.minute() < 10)
    Serial.print('0');
  Serial.print(gps.time.minute());
  Serial.print(":");
  if (gps.time.second() < 10)
    Serial.print('0');
  Serial.println(gps.time.second());
}