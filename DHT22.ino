#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// konfigurasi wifi
const char* ssid = "SMKM-1";
const char* password = "kupattahupadalarang";


#define DHTPIN D3  // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22  // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);

// pin buzzer
#define BUZZER_PIN D2

uint32_t delayMS;

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  delay(1000);

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
    String urlKirim = "http://192.168.1.35:8000/update-data/";
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
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;

    String urlGet = "http://192.168.1.35:8000/get-latest";
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
  }

  delay(3000);
}

