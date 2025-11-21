#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// konfigurasi wifi
const char* ssid = "SMKM-1";
const char* password = "kupattahupadalarang";


#define DHTPIN D4
#define BUZZER D7
#define LEDYELLOW D6
#define LEDRED D1
#define LEDGREEN D2
#define LEDWHITE D3
#define LEDYELLOW2 D8
#define LEDRED2 D5
#define DHTTYPE DHT22

String BASE_URL = "http://192.168.100.13:8000";
int pinLampu[6] = {D6, D1, D2, D3, D8, D5};


DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LEDYELLOW, OUTPUT);
  pinMode(LEDRED, OUTPUT);
  pinMode(LEDGREEN, OUTPUT);
  pinMode(LEDWHITE, OUTPUT);
  pinMode(LEDYELLOW2, OUTPUT);
  pinMode(LEDRED2, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LEDYELLOW, LOW);
  digitalWrite(LEDRED, LOW);
  digitalWrite(LEDGREEN, LOW);
  digitalWrite(LEDWHITE, LOW);
  digitalWrite(LEDYELLOW2, LOW);
  digitalWrite(LEDRED2, LOW);
  digitalWrite(BUZZER, LOW);

  // koneksi ke wifi
  Serial.println();
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Terkoneksi");
  Serial.print("Ip Address : ");
  Serial.println(WiFi.localIP());

  // Initialize device.
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
}

void loop() {
  delay(delayMS);
  lampu();
  sensors_event_t event;

  dht.temperature().getEvent(&event);
  float temperature = event.temperature;

  dht.humidity().getEvent(&event);
  float humidity = event.relative_humidity;

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Gagal baca Sensor DHT");
    return;
  }

  Serial.print("Temperature : ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity : ");
  Serial.print(humidity);
  Serial.println(" %");


  // === 1. KIRIM DATA KE LARAVEL ===
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;

    // endpoint untuk kirim data sensor
    String urlKirim = BASE_URL + "/update-data/";
    urlKirim += String(temperature, 1) + "/" + String(humidity, 1);

    Serial.print("Mengirim data ke : ");
    Serial.println(urlKirim);

    http.begin(client, urlKirim);
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.printf("HTTP Response code : %d\n", httpCode);
      String payload = http.getString();
      Serial.println("Response : ");
      Serial.println(payload);
    } else {
      Serial.printf("Gagal Mengirim Data. Error : %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();

  } else {
    Serial.println("WiFi tidak terkoneksi");
    WiFi.reconnect();
  }


  // === 2. AMBIL DATA DARI SERVER (CEK BUZZER) ===
 /*  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;

    String urlGet = BASE_URL + "/get-latest";
    http.begin(client, urlGet);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("📡 Respon dari Laravel: " + payload);

      if (payload.indexOf("\"buzzer\":1") != -1) {
        Serial.println("🔥 Suhu tinggi! Buzzer ON");
        digitalWrite(BUZZER_PIN, HIGH);
        tone(BUZZER_PIN, 3000);
        delay(1000);
      } else {
        Serial.println("✅ Suhu normal, Buzzer OFF");
        digitalWrite(BUZZER_PIN, LOW);
        noTone(BUZZER_PIN);
      }

    } else {
      Serial.print("❌ Gagal ambil data dari server ");
      Serial.println(httpCode);
    }

    http.end();
  } */

  delay(3000);
}

void lampu() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, BASE_URL + "/get-lampu");
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Data LAMPUU dari server:");
      Serial.println(payload);
      StaticJsonDocument<700> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error){
        for (int i = 0; i < doc.size(); i++) {
          int id = doc[i]["id"];
          String name = doc[i]["name"].as<String>();
          int status = doc[i]["status"];
          
          // ubah lampu 1->index 0
          int index = id - 1;

          int pinLampu[6] = {D6, D1, D2, D3, D8, D5};

          // kontrol LED 
          if (index >= 0 && index < 6) {
            digitalWrite(pinLampu[index], status);

            Serial.print("name : ");
            Serial.print(name);
            Serial.print(" = ");
            
            Serial.println(status);
          }
        }
      }
      
    } else {
      Serial.printf("Gagal ambil LAMPU: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

